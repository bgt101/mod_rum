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



#include "LuaServerRec.H"
#include "lua.hpp"
#include "StrPtrMap.H"
#include "FuncFieldHandler.H"
#include "VirtualFieldHandler.H"
#include "MemberFieldHandler.H"
#include "util_lua.H"
#include "httpd.h"



#define RUM_REGISTER_SERVER_REC_FIELD(map, name, type) \
    RUM_REGISTER_MEMBER_FIELD(server_rec, false, map, name, type)



namespace rum
{

    // server_rec methods
    static const struct luaL_Reg serverRecRecMethods[] =
    {
        {0, 0}
    };



    // server_rec metamethods
    static const struct luaL_Reg serverRecMetaMethods[] =
    {
        {"__index", LuaServerRec::serverRecIndex},
        {"__newindex", LuaServerRec::serverRecNewindex},
        {0, 0}
    };



    // server_rec virtual field methods
    static const struct VirtualFieldHandler::fieldReg serverRecRecVFMethods[] =
    {
        {0, 0, 0}
    };



    // called when a new Lua interpreter is created
    void LuaServerRec::luaPrepEnv(lua_State *L, Logger *logger__,
                                apr_pool_t *pool__)
    {
        RUM_LOG_COND(logger__, APLOG_DEBUG,
                     "creating field handler map for server_rec");

        StrPtrMap<FieldHandler *> *fhm =
            new (pool__) StrPtrMap<FieldHandler *>(0);
        fhm->destroyWithPool();

        // register (char *) fields of server_rec
        RUM_REGISTER_SERVER_REC_FIELD(fhm, defn_name, CharPtr);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, server_admin, CharPtr);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, server_hostname, CharPtr);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, error_fname, CharPtr);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, path, CharPtr);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, server_scheme, CharPtr);

        // register (int) fields of server_rec
        RUM_REGISTER_SERVER_REC_FIELD(fhm, defn_line_number, Int);
#if RUM_AP22
        RUM_REGISTER_SERVER_REC_FIELD(fhm, loglevel, Int);
#endif
        RUM_REGISTER_SERVER_REC_FIELD(fhm, is_virtual, Int);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, keep_alive_max, Int);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, keep_alive, Int);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, pathlen, Int);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, limit_req_line, Int);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, limit_req_fieldsize, Int);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, limit_req_fields, Int);

        // register (apr_uint16_t) fields of server_rec
        RUM_REGISTER_SERVER_REC_FIELD(fhm, port, UInt16);

        // register (apr_int64_t) fields of server_rec
        RUM_REGISTER_SERVER_REC_FIELD(fhm, timeout, Int64);
        RUM_REGISTER_SERVER_REC_FIELD(fhm, keep_alive_timeout, Int64);

        // register (server_rec *) fields of server_rec
        RUM_REGISTER_SERVER_REC_FIELD(fhm, next, ServerRec);

        // register functions of server_rec
        const luaL_Reg *funcReg = serverRecRecMethods;
        while (funcReg && funcReg->name)
        {
            fhm->insert(funcReg->name,
                        new (fhm->pool()) FuncFieldHandler(0, funcReg->name,
                                                           funcReg->func));
            funcReg++;
        }

        // register virtual field functions of server_rec
        const VirtualFieldHandler::fieldReg *vfReg = serverRecRecVFMethods;
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

        // create metatable for (server_rec *), add it to the Lua
        // registry, and register the metamethods
        luaL_newmetatable(L, RUM_PTR_REGISTRY_NAME(server_rec));
        luaL_register(L, 0, serverRecMetaMethods);
        lua_pop(L, 1);
    }



    int LuaServerRec::serverRecIndex(lua_State *L)
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
            luaL_error(L, "invalid server_rec field: %s", key);
        }

        return rc;
    }



    int LuaServerRec::serverRecNewindex(lua_State *L)
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
            luaL_error(L, "invalid server_rec field: %s", key);
        }

        return rc;
    }



    char LuaServerRec::fhmRegKey_useMyAddress_;
}
