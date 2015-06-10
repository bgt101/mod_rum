// Copyright 2015 CBS Interactive Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// CBS Interactive accepts contributions to software products and free
// and open-source projects owned, licensed, managed, or maintained by
// CBS Interactive submitted under the terms of the CBS Interactive
// Contribution License Agreement (the "Contribution Agreement"); you may
// not submit software to CBS Interactive for inclusion in a CBS
// Interactive product or project unless you agree to the terms of the
// CBS Interactive Contribution License Agreement or have executed a
// separate agreement with CBS Interactive governing the use of such
// submission. A copy of the Contribution Agreement should have been
// included with the software. You may also obtain a copy of the
// Contribution Agreement at
// http://www.cbsinteractive.com/cbs-interactive-software-grant-and-contribution-license-agreement/.



#include "string.h"
#include "apr_lib.h"
#include "lua.hpp"
#include "rum_errno.H"
#include "util_lua.H"
#include "httpd.h"
#include "LuaAction.H"
#include "ReqCtx.H"
#include "ActionCtx.H"
#include "Config.H"
#include "LuaManager.H"



namespace rum
{

    ActionCtx* LuaAction::getActionCtx(lua_State *L)
    {
        int tos = lua_gettop(L);
        lua_pushlightuserdata(L, rum::LuaAction::sandboxEnvRegKey());
        lua_gettable(L, LUA_REGISTRYINDEX);
        lua_getfield(L, -1, "rum");

        ActionCtx *actionCtx = NULL;
        lua_pushlightuserdata(L, actionCtxRumKey());
        lua_gettable(L, -2);
        if (lua_islightuserdata(L, -1))
        {
            actionCtx = static_cast<ActionCtx *>(lua_touserdata(L, -1));
        }
        if (!actionCtx)
        {
            luaL_error(L, "unable to access actionCtx");
        }
        lua_pop(L, lua_gettop(L) - tos);

        return actionCtx;
    }



    int LuaAction::setRumFields(lua_State *L, ActionCtx *actionCtx)
    {
        // set the fields of the "rum" table which is at the top of
        // the Lua stack

        int tos = lua_gettop(L);
        ReqCtx *reqCtx = actionCtx->reqCtx();
        apr_status_t aStatus;

        // store address of ActionCtx object as light userdata
        lua_pushlightuserdata(L, actionCtxRumKey());
        lua_pushlightuserdata(L, actionCtx);
        lua_settable(L, -3);

        // for each of the condition modules create a sub-table
        StrPtrMap<CondModule *>::ConstIterator
            it(reqCtx->pool(), reqCtx->config().condModulesMap());
        while (it.next())
        {
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                           "creating subtable for "
                           "condition module: " << it.key());
            aStatus = it.val()->prepLuaAction(L, actionCtx);
            if (aStatus != APR_SUCCESS)
            {
                RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                               "failed to create Lua subtable for "
                               "condition module");
                return APR_EGENERAL;
            }

            // in case extra stuff was left on stack
            lua_pop(L, lua_gettop(L) - tos);
        }

        return APR_SUCCESS;
    }



    apr_status_t LuaAction::run(ActionCtx *actionCtx) const
    {
        RUM_PTRC_ACTION(actionCtx->reqCtx()->pool(),
                       "LuaAction::run(ActionCtx *actionCtx), "
                       "actionCtx: " << (void *)actionCtx);


        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                       "LuaAction: executing chunk");


        int lStatus;
        apr_status_t aStatus;
        ReqCtx *reqCtx = actionCtx->reqCtx();
        lua_State *L = reqCtx->luaState();
        if (L == 0)
        {
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                           "aborting Lua action due to errors");
            return APR_EGENERAL;
        }
        lua_pop(L, lua_gettop(L));

        // if custom error handler is defined, push it onto the stack,
        // otherwise use debug.traceback
        if (reqCtx->config().errHandlerChunk())
        {
            const Blob *chunk = reqCtx->config().errHandlerChunk();
            lStatus = luaL_loadbuffer(L,
                                      static_cast<const char *>(chunk->data()),
                                      chunk->size(),
                                      "<ErrorHandler>");
            if (lStatus) {
                const char *msg = lua_tostring(L, -1);
                RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                               "failed to load ErrorHandler Lua chunk: "
                               << msg);
                return APR_EGENERAL;
            }
        }
        else
        {
            // push debug.traceback onto stack
            lua_getglobal(L, "debug");
            lua_getfield(L, -1, "traceback");
            lua_remove(L, -2);
        }

        // create Lua sandbox environment which inherits from main environment
        lua_newtable(L);
        lua_newtable(L);
        lua_pushliteral(L, "__index");
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        lua_settable(L, -3);
        lua_setmetatable(L, -2);
        int envTableIdx = lua_gettop(L);

        // make sure scripts can't access real global _G
        lua_pushliteral(L, "_G");
        lua_pushvalue(L, -2);
        lua_settable(L, -3);

        // add sandbox env to registry so that interactive_lua() can access it
        lua_pushlightuserdata(L, sandboxEnvRegKey());
        lua_pushvalue(L, envTableIdx);
        lua_settable(L, LUA_REGISTRYINDEX);


        // create "rum" table in sandbox environment
        lua_pushvalue(L, envTableIdx);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        int tos = lua_gettop(L);
        aStatus = setRumFields(L, actionCtx);
        if (aStatus != APR_SUCCESS)
        {
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                           "aborting Lua action due to errors");
            return APR_EGENERAL;
        }
        lua_pop(L, lua_gettop(L) - tos);
        lua_setfield(L, -3, "rum");
        tos = lua_gettop(L);
        // the "rum" table is now at the top of the stack


        // have the global "rum" proxy table inherit from the real
        // "rum" table in the sandbox environment we just created
        lua_getglobal(L, "rum");
        lua_getmetatable(L, -1);
        lua_pushvalue(L, -3);
        lua_setfield(L, -2, "__index");
        lua_pop(L, 2);


        // load and run CommonPreAction Lua chunks in sandbox
        StaticStrBuffer<32> nameBuf;
        nameBuf << "ruleIdx:" << actionCtx->ruleIdx();
        apr_ssize_t i;
        apr_ssize_t numCPA = reqCtx->config().commonPreActions().size();
        for (i = 0; i < numCPA; i++)
        {
            const Blob *chunk = reqCtx->config().commonPreActions()[i];
            lStatus = luaL_loadbuffer(L,
                                      static_cast<const char *>(chunk->data()),
                                      chunk->size(),
                                      nameBuf.asStr());
            if (lStatus) {
                const char *msg = lua_tostring(L, -1);
                RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                               "failed to load CommonPreAction Lua chunk: "
                               << msg);
                return APR_EGENERAL;
            }
            lua_pushvalue(L, envTableIdx);
            lua_setfenv(L, -2);
            lStatus = lua_pcall(L, 0, 0, 1);
            if (lStatus) {
                const char *msg = lua_tostring(L, -1);
                RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                               "failed to execute CommonPreAction Lua chunk: "
                               << msg);
                return APR_EGENERAL;
            }
        }


        // load and run Lua chunk in same sandbox as CommonPreAction
        lStatus = luaL_loadbuffer(L,
                                  static_cast<const char *>(chunk_->data()),
                                  chunk_->size(),
                                  nameBuf.asStr());
        if (lStatus) {
            const char *msg = lua_tostring(L, -1);
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                           "failed to load Lua chunk: " << msg);
            return APR_EGENERAL;
        }
        lua_pushvalue(L, envTableIdx);
        lua_setfenv(L, -2);
        lStatus = lua_pcall(L, 0, 1, 1);
        if (lStatus) {
            const char *msg = lua_tostring(L, -1);
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                           "failed to execute Lua chunk: " << msg);
            return APR_EGENERAL;
        }

        // process optionally returned value
        aStatus = RUM_DEFAULT;
        if (lua_type(L, -1) == LUA_TNUMBER)
        {
            aStatus = static_cast<apr_status_t>(lua_tointeger(L, -1));
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                           "integer return code from action: " << aStatus);
            if ((aStatus != RUM_OK) &&
                (aStatus != RUM_DELAYED_OK) &&
                (aStatus != RUM_RELOOKUP) &&
                (aStatus != RUM_DECLINED))
            {
                RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                               "invalid return code from action: " << aStatus);
                return APR_EGENERAL;
            }
        }
        else if (lua_type(L, -1) == LUA_TNIL)
        {
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                           "nil return code from action");
        }
        else
        {
            RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR,
                           "invalid non-integer return code from action");
            return APR_EGENERAL;
        }
        lua_pop(L, 1);


#if 0 //bgt -- eliminate in favor of return codes
        // check status flags
        if (actionCtx->lastAction())
        {
            aStatus = RUM_LAST_ACTION;
        }
        else if (actionCtx->relookup())
        {
            aStatus = RUM_RELOOKUP;
        }
#endif


        return aStatus;
    }



    StrBuffer& LuaAction::write(StrBuffer& sb) const
    {
        return sb << "Lua chunk size: " << chunk_->size();
    }



    const char *LuaAction::actionName_ = "Lua";

    char LuaAction::sandboxEnvRegKey_useMyAddress_;
    char LuaAction::actionCtxRumKey_useMyAddress_;
}
