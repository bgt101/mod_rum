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



#include "LuaConnRec.H"
#include "lua.hpp"
#include "StrPtrMap.H"
#include "FuncFieldHandler.H"
#include "VirtualFieldHandler.H"
#include "MemberFieldHandler.H"
#include "util_lua.H"



#define RUM_REGISTER_CONN_FIELD(map, name, type) \
    RUM_REGISTER_MEMBER_FIELD(conn_rec, true, map, name, type)



namespace rum
{

    // connection methods
    static const struct luaL_Reg connRecMethods[] =
    {
        {0, 0}
    };



    // connection metamethods
    static const struct luaL_Reg connMetaMethods[] =
    {
        {"__index", LuaConnRec::connIndex},
        {"__newindex", LuaConnRec::connNewindex},
        {0, 0}
    };



    // connection virtual field methods
    static const struct VirtualFieldHandler::fieldReg connRecVFMethods[] =
    {
        {"aborted",
         LuaConnRec::connGetAborted,
         LuaConnRec::connSetAborted},
        {"double_reverse",
         LuaConnRec::connGetDoubleReverse,
         LuaConnRec::connSetDoubleReverse},
        {0, 0, 0}
    };



    // called when a new Lua interpreter is created
    void LuaConnRec::luaPrepEnv(lua_State *L, Logger *logger__,
                                apr_pool_t *pool__)
    {
        RUM_LOG_COND(logger__, APLOG_DEBUG,
                     "creating field handler map for conn_rec");

        StrPtrMap<FieldHandler *> *fhm =
            new (pool__) StrPtrMap<FieldHandler *>(0);
        fhm->destroyWithPool();

        // register (char *) fields of conn_rec
#if RUM_AP22
        RUM_REGISTER_CONN_FIELD(fhm, remote_ip, CharPtr);
#else
        RUM_REGISTER_CONN_FIELD(fhm, client_ip, CharPtr);
#endif
        RUM_REGISTER_CONN_FIELD(fhm, remote_host, CharPtr);
        RUM_REGISTER_CONN_FIELD(fhm, remote_logname, CharPtr);
        RUM_REGISTER_CONN_FIELD(fhm, local_ip, CharPtr);
        RUM_REGISTER_CONN_FIELD(fhm, local_host, CharPtr);

        // register (int) fields of conn_rec
        RUM_REGISTER_CONN_FIELD(fhm, keepalives, Int);
        RUM_REGISTER_CONN_FIELD(fhm, data_in_input_filters, Int);
#if RUM_AP22
        RUM_REGISTER_CONN_FIELD(fhm, clogging_input_filters, Int);
#endif

        // register (long) fields of conn_rec
        RUM_REGISTER_CONN_FIELD(fhm, id, Long);

        // register APR table fields of conn_rec
        RUM_REGISTER_CONN_FIELD(fhm, notes, Table);

        // register apr_sockaddr_t fields of conn_rec
        RUM_REGISTER_CONN_FIELD(fhm, local_addr, SockAddr);
#if RUM_AP22
        RUM_REGISTER_CONN_FIELD(fhm, remote_addr, SockAddr);
#else
        RUM_REGISTER_CONN_FIELD(fhm, client_addr, SockAddr);
#endif

        // register functions of conn
        const luaL_Reg *funcReg = connRecMethods;
        while (funcReg && funcReg->name)
        {
            fhm->insert(funcReg->name,
                        new (fhm->pool()) FuncFieldHandler(0, funcReg->name,
                                                           funcReg->func));
            funcReg++;
        }

        // register virtual field functions of conn
        const VirtualFieldHandler::fieldReg *vfReg = connRecVFMethods;
        while (vfReg && vfReg->name)
        {
            fhm->insert(vfReg->name,
                        new (fhm->pool()) VirtualFieldHandler(0, vfReg->name,
                                                              vfReg->getFunc,
                                                              vfReg->setFunc));
            vfReg++;
        }

        // add field handler map to Lua registry
        lua_pushlightuserdata(L, fhmRegKey());
        lua_pushlightuserdata(L, fhm);
        lua_settable(L, LUA_REGISTRYINDEX);

        // create metatable for (conn_rec *), add it to the Lua
        // registry, and register the metamethods
        luaL_newmetatable(L, RUM_PTR_REGISTRY_NAME(conn_rec));
        luaL_register(L, 0, connMetaMethods);
        lua_pop(L, 1);
    }



    int LuaConnRec::connGetAborted(lua_State *L)
    {
        conn_rec *c = RUM_CHECK_NON_NULL_PTR(conn_rec, L, 1);
        lua_pushinteger(L, c->aborted);
        return 1;
    }



    int LuaConnRec::connSetAborted(lua_State *L)
    {
        conn_rec *c = RUM_CHECK_NON_NULL_PTR(conn_rec, L, 1);
        c->aborted = luaL_checkint(L, 3);
        return 0;
    }



    int LuaConnRec::connGetDoubleReverse(lua_State *L)
    {
        conn_rec *c = RUM_CHECK_NON_NULL_PTR(conn_rec, L, 1);
        lua_pushinteger(L, c->double_reverse);
        return 1;
    }



    int LuaConnRec::connSetDoubleReverse(lua_State *L)
    {
        conn_rec *c = RUM_CHECK_NON_NULL_PTR(conn_rec, L, 1);
        c->double_reverse = luaL_checkint(L, 3);
        return 0;
    }



    int LuaConnRec::connIndex(lua_State *L)
    {
        // get field handler map from Lua registry
        StrPtrMap<FieldHandler *> *fhm = NULL;
        lua_pushlightuserdata(L, fhmRegKey());
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (lua_islightuserdata(L, -1))
        {
            fhm = static_cast<StrPtrMap<FieldHandler *> *>
                  (lua_touserdata(L, -1));
        }
        if (!fhm)
        {
            luaL_error(L, "unable to retrieve field handler map");
        }
        lua_pop(L, 1);

        // call appropriate field handler
        int rc = 0;
        const char *key = luaL_checkstring(L, 2);
        const FieldHandler *fh = fhm->find(key);
        if (fh)
        {
            rc = fh->get(L);
        }
        else
        {
            luaL_error(L, "invalid conn field: %s", key);
        }

        return rc;
    }



    int LuaConnRec::connNewindex(lua_State *L)
    {
        // get field handler map from Lua registry
        StrPtrMap<FieldHandler *> *fhm = NULL;
        lua_pushlightuserdata(L, fhmRegKey());
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (lua_islightuserdata(L, -1))
        {
            fhm = static_cast<StrPtrMap<FieldHandler *> *>
                  (lua_touserdata(L, -1));
        }
        if (!fhm)
        {
            luaL_error(L, "unable to retrieve field handler map");
        }
        lua_pop(L, 1);

        // call appropriate field handler
        int rc = 0;
        const char *key = luaL_checkstring(L, 2);
        const FieldHandler *fh = fhm->find(key);
        if (fh)
        {
            rc = fh->set(L);
        }
        else
        {
            luaL_error(L, "invalid conn field: %s", key);
        }

        return rc;
    }



    char LuaConnRec::fhmRegKey_useMyAddress_;
}
