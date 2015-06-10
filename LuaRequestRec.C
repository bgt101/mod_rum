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



#include "LuaRequestRec.H"
#include "lua.hpp"
#include "StrPtrMap.H"
#include "FuncFieldHandler.H"
#include "VirtualFieldHandler.H"
#include "MemberFieldHandler.H"
#include "util_lua.H"
#include "http_core.h"
#include "http_protocol.h"
#include "http_request.h"
#include "httpd.h"



#define RUM_REGISTER_REQUEST_FIELD(map, name, type) \
    RUM_REGISTER_MEMBER_FIELD(request_rec, true, map, name, type)



namespace rum
{
    // request methods
    static const struct luaL_Reg requestRecMethods[] =
    {
        {"is_internal_redirect", LuaRequestRec::requestIsInternalRedirect},
        {"is_subrequest", LuaRequestRec::requestIsSubrequest},
        {"get_remote_logname", LuaRequestRec::getRemoteLogname},
        {"get_server_name", LuaRequestRec::getServerName},
        {"get_server_port", LuaRequestRec::getServerPort},
#if RUM_AP22
        {"default_type", LuaRequestRec::defaultType},
#endif
        {"set_content_type", LuaRequestRec::setContentType},
        {"rputs", LuaRequestRec::rputs},
        {"is_initial_req", LuaRequestRec::isInitialReq},
        {"http_scheme", LuaRequestRec::httpScheme},
        {"outermost_request", LuaRequestRec::requestOutermostRequest},
        {0, 0}
    };



    // request metamethods
    static const struct luaL_Reg requestMetaMethods[] =
    {
        {"__index", LuaRequestRec::requestIndex},
        {"__newindex", LuaRequestRec::requestNewindex},
        {0, 0}
    };



    // request virtual field methods
    static const struct VirtualFieldHandler::fieldReg requestRecVFMethods[] =
    {
        {0, 0, 0}
    };



    // called when a new Lua interpreter is created
    void LuaRequestRec::luaPrepEnv(lua_State *L, Logger *logger__,
                                   apr_pool_t *pool__)
    {
        RUM_LOG_COND(logger__, APLOG_DEBUG,
                     "creating field handler map for request_rec");

        StrPtrMap<FieldHandler *> *fhm =
            new (pool__) StrPtrMap<FieldHandler *>(0);
        fhm->destroyWithPool();


        // register (char *) fields of request_rec
        RUM_REGISTER_REQUEST_FIELD(fhm, the_request, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, protocol, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, hostname, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, status_line, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, method, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, range, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, content_type, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, handler, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, content_encoding, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, vlist_validator, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, user, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, ap_auth_type, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, unparsed_uri, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, uri, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, filename, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, canonical_filename, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, path_info, CharPtr);
        RUM_REGISTER_REQUEST_FIELD(fhm, args, CharPtr);

        // register (int) fields of request_rec
        RUM_REGISTER_REQUEST_FIELD(fhm, assbackwards, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, proxyreq, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, header_only, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, proto_num, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, status, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, method_number, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, chunked, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, read_body, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, read_chunked, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, expecting_100, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, no_cache, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, no_local_copy, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, used_path_info, Int);
        RUM_REGISTER_REQUEST_FIELD(fhm, eos_sent, Int);

        // register various pointer fields of request_rec
        RUM_REGISTER_REQUEST_FIELD(fhm, next, RequestRec);
        RUM_REGISTER_REQUEST_FIELD(fhm, prev, RequestRec);
        RUM_REGISTER_REQUEST_FIELD(fhm, main, RequestRec);
        RUM_REGISTER_REQUEST_FIELD(fhm, connection, ConnRec);
        RUM_REGISTER_REQUEST_FIELD(fhm, server, ServerRec);

        // register APR table fields of request_rec
        RUM_REGISTER_REQUEST_FIELD(fhm, headers_in, Table);
        RUM_REGISTER_REQUEST_FIELD(fhm, headers_out, Table);
        RUM_REGISTER_REQUEST_FIELD(fhm, err_headers_out, Table);
        RUM_REGISTER_REQUEST_FIELD(fhm, subprocess_env, Table);
        RUM_REGISTER_REQUEST_FIELD(fhm, notes, Table);

        // register functions of request
        const luaL_Reg *funcReg = requestRecMethods;
        while (funcReg && funcReg->name)
        {
            fhm->insert(funcReg->name,
                        new (fhm->pool()) FuncFieldHandler(0, funcReg->name,
                                                           funcReg->func));
            funcReg++;
        }

        // register virtual field functions of request
        const VirtualFieldHandler::fieldReg *vfReg = requestRecVFMethods;
        while (vfReg && vfReg->name)
        {
            fhm->insert(vfReg->name,
                        new (fhm->pool()) VirtualFieldHandler(0,
                                                              vfReg->name,
                                                              vfReg->getFunc,
                                                              vfReg->setFunc));
            vfReg++;
        }

        // add field handler map to Lua registry
        lua_pushlightuserdata(L, fhmRegKey());
        lua_pushlightuserdata(L, fhm);
        lua_settable(L, LUA_REGISTRYINDEX);

        // create metatable for (request_rec *), add it to the Lua
        // registry, and register the metamethods
        luaL_newmetatable(L, RUM_PTR_REGISTRY_NAME(request_rec));
        luaL_register(L, 0, requestMetaMethods);
        lua_pop(L, 1);
    }



    int LuaRequestRec::requestIsInternalRedirect(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushboolean(L, (r->prev) ? 1 : 0);
        return 1;
    }



    int LuaRequestRec::requestIsSubrequest(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushboolean(L, (r->main) ? 1 : 0);
        return 1;
    }



    int LuaRequestRec::getRemoteLogname(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushstring(L, ap_get_remote_logname(r));
        return 1;
    }



    int LuaRequestRec::getServerName(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushstring(L, ap_get_server_name(r));
        return 1;
    }



    int LuaRequestRec::getServerPort(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushinteger(L, ap_get_server_port(r));
        return 1;
    }



#if RUM_AP22
    int LuaRequestRec::defaultType(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushstring(L, ap_default_type(r));
        return 1;
    }
#endif



    int LuaRequestRec::setContentType(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        const char *ct = apr_pstrdup(r->pool, luaL_checkstring(L, 2));
        ap_set_content_type(r, ct);
        return 0;
    }



    int LuaRequestRec::rputs(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        const char *str = apr_pstrdup(r->pool, luaL_checkstring(L, 2));
        ap_rputs(str, r);
        return 0;
    }



    int LuaRequestRec::isInitialReq(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushboolean(L, ap_is_initial_req(r));
        return 1;
    }



    int LuaRequestRec::httpScheme(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        lua_pushstring(L, ap_http_scheme(r));
        return 1;
    }



    int LuaRequestRec::requestOutermostRequest(lua_State *L)
    {
        request_rec *r = RUM_CHECK_NON_NULL_PTR(request_rec, L, 1);
        while (r->main)
        {
            if (r == r->main)
            {
                luaL_error(L, "infinite loop averted: r == r->main");
            }
            r = r->main;
        }
        RUM_PUSH_PTR(request_rec, L, r);
        return 1;
    }



    int LuaRequestRec::requestIndex(lua_State *L)
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
            luaL_error(L, "invalid request field: %s", key);
        }

        return rc;
    }



    int LuaRequestRec::requestNewindex(lua_State *L)
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
            luaL_error(L, "invalid request field: %s", key);
        }

        return rc;
    }



    char LuaRequestRec::fhmRegKey_useMyAddress_;
}
