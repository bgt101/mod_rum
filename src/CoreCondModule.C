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



#include "http_request.h"
#include "http_protocol.h"

#include "interactive_lua.H"
#include "CoreCondModule.H"
#include "DummyReqData.H"
#include "StrBuffer.H"
#include "SizeVec.H"
#include "apr_xml.h"
#include "get_cdata.H"
#include "ReqCtx.H"
#include "ActionCtx.H"
#include "LuaAction.H"
#include "lua.hpp"
#include "Config.H"
#include "string.h"
#include "CoreServerNameFiltCond.H"



#define RUM_API_VERSION_MAJOR 2
#define RUM_API_VERSION_MINOR 0



namespace rum
{

    // methods within "rum.core"
    static const struct luaL_Reg rumCoreMethods[] = {
        {"internal_redirect", CoreCondModule::coreInternalRedirect},
        {"external_redirect", CoreCondModule::coreExternalRedirect},
        {"log_emerg", CoreCondModule::coreLogEmerg},
        {"log_alert", CoreCondModule::coreLogAlert},
        {"log_crit", CoreCondModule::coreLogCrit},
        {"log_err", CoreCondModule::coreLogError},
        {"log_error", CoreCondModule::coreLogError},
        {"log_warn", CoreCondModule::coreLogWarn},
        {"log_warning", CoreCondModule::coreLogWarn},
        {"log_notice", CoreCondModule::coreLogNotice},
        {"log_info", CoreCondModule::coreLogInfo},
        {"log_debug", CoreCondModule::coreLogDebug},
        {"set_content_type", CoreCondModule::coreSetContentType},
        {"proxy", CoreCondModule::coreProxy},
        {"puts", CoreCondModule::corePuts},
        {"rflush", CoreCondModule::coreRflush},
        {"rputs", CoreCondModule::coreRputs},
        {"rwrite", CoreCondModule::coreRwrite},
        {"interactive_lua", CoreCondModule::coreInteractiveLua},
        {"api_version_major", CoreCondModule::coreAPIVersionMajor},
        {"api_version_minor", CoreCondModule::coreAPIVersionMinor},
        {"state", CoreCondModule::coreState},
        {0, 0}
    };



    // metamethods within "rum.core"
    static const struct luaL_Reg rumCoreMetaMethods[] = {
        {"__index", CoreCondModule::coreIndex},
        {"__newindex", CoreCondModule::coreNewindex},
        {0, 0}
    };



    int CoreCondModule::coreInternalRedirect(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        const char *uri = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "internal redirect: " << uri);
        r->filename = apr_pstrcat(r->pool, "redirect:", uri, NULL);
        r->handler = "rum-int-redirect";

        return 0;
    }



    int CoreCondModule::coreExternalRedirect(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        const char *url = luaL_checkstring(L, 1);
        int http_status = luaL_optint(L, 2, HTTP_MOVED_PERMANENTLY);
        apr_table_setn(r->headers_out, "Location", apr_pstrdup(r->pool, url));
        r->status = http_status;
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "external redirect (" << http_status << "): " << url);

        return 0;
    }



    int CoreCondModule::coreLogEmerg(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_EMERG, msg);

        return 0;
    }



    int CoreCondModule::coreLogAlert(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ALERT, msg);

        return 0;
    }



    int CoreCondModule::coreLogCrit(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_CRIT, msg);

        return 0;
    }



    int CoreCondModule::coreLogError(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_ERR, msg);

        return 0;
    }



    int CoreCondModule::coreLogWarn(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_WARNING, msg);

        return 0;
    }



    int CoreCondModule::coreLogNotice(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_NOTICE, msg);

        return 0;
    }



    int CoreCondModule::coreLogInfo(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO, msg);

        return 0;
    }



    int CoreCondModule::coreLogDebug(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *msg = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG, msg);

        return 0;
    }



    int CoreCondModule::coreSetContentType(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        const char *ct = apr_pstrdup(r->pool, luaL_checkstring(L, 1));
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "setting content-type to " << ct);
        ap_set_content_type(r, ct);

        return 0;
    }



    int CoreCondModule::coreProxy(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        const char *url = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "proxying to URL: " << url);
        r->handler = "proxy-server";
        r->proxyreq = PROXYREQ_REVERSE;
        const char *url_sans_qa;
        const char *qm = strchr(url, '?');
        if (qm)
        {
            url_sans_qa = apr_pstrndup(r->pool, url, qm - url);
            const char *qa = qm + 1;
            if (*qa)
            {
                r->args = apr_pstrdup(r->pool, qa);
            }
            else
            {
                r->args = NULL;
            }
        }
        else
        {
            url_sans_qa = url;
            r->args = NULL;
        }
        r->filename = apr_pstrcat(r->pool, "proxy:", url_sans_qa, NULL);

        return 0;
    }



    int CoreCondModule::corePuts(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        const char *str = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                       "puts called");
        ap_rputs(str, r);
        ap_rputs("\n", r);
        actionCtx->reqCtx()->doHandleReq();

        return 0;
    }



    int CoreCondModule::coreRflush(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                       "rflush called");
        ap_rflush(r);
        actionCtx->reqCtx()->doHandleReq();

        return 0;
    }



    int CoreCondModule::coreRputs(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        const char *str = luaL_checkstring(L, 1);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                       "rputs called");
        ap_rputs(str, r);
        actionCtx->reqCtx()->doHandleReq();

        return 0;
    }



    int CoreCondModule::coreRwrite(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        request_rec *r = actionCtx->reqCtx()->req();
        size_t len;
        const char *str = luaL_checklstring(L, 1, &len);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                       "rwrite called");
        ap_rwrite(str, len, r);
        actionCtx->reqCtx()->doHandleReq();

        return 0;
    }



    int CoreCondModule::coreInteractiveLua(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "interactive_lua called");
        interactive_lua(L);

        return 0;
    }



    int CoreCondModule::coreAPIVersionMajor(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "api_version_major called");
        lua_pushinteger(L, RUM_API_VERSION_MAJOR);

        return 1;
    }



    int CoreCondModule::coreAPIVersionMinor(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "api_version_minor called");
        lua_pushinteger(L, RUM_API_VERSION_MINOR);

        return 1;
    }



    int CoreCondModule::coreState(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_INFO,
                       "state called");

        StrBuffer buf(actionCtx->reqCtx()->pool());
        buf << actionCtx->reqCtx()->config();
        lua_pushstring(L, buf);

        return 1;
    }



    int CoreCondModule::coreIndex(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                       "__index called on core");

        const char *key = luaL_checkstring(L, 2);
        if (strcmp(key, "last_action") == 0)
        {
            lua_pushboolean(L, actionCtx->lastAction());
        }
        else if (strcmp(key, "relookup") == 0)
        {
            lua_pushboolean(L, actionCtx->relookup());
        }
        else if (strcmp(key, "DECLINED") == 0)
        {
            lua_pushinteger(L, RUM_DECLINED);
        }
        else if (strcmp(key, "OK") == 0)
        {
            lua_pushinteger(L, RUM_OK);
        }
        else if (strcmp(key, "DELAYED_OK") == 0)
        {
            lua_pushinteger(L, RUM_DELAYED_OK);
        }
        else if (strcmp(key, "RELOOKUP") == 0)
        {
            lua_pushinteger(L, RUM_RELOOKUP);
        }
        else
        {
            luaL_error(L, "invalid core field: %s", key);
        }

        return 1;
    }



    int CoreCondModule::coreNewindex(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_LOG_ACTION(actionCtx->reqCtx()->logger(), APLOG_DEBUG,
                       "__newindex called on core");

        const char *key = luaL_checkstring(L, 2);
        luaL_checkany(L, 3);
        if (strcmp(key, "last_action") == 0)
        {
            actionCtx->setLastAction(lua_toboolean(L, 3));
        }
        else if (strcmp(key, "relookup") == 0)
        {
            actionCtx->setRelookup(lua_toboolean(L, 3));
        }
        else
        {
            luaL_error(L, "invalid core field: %s", key);
        }

        return 0;
    }



    StrBuffer& CoreCondModule::write(StrBuffer& sb) const
    {
        return sb;
    }



    apr_status_t
    CoreCondModule::parseXMLCond(apr_pool_t *pTmp,
                                 const apr_xml_elem *elem,
                                 apr_ssize_t ruleIdx,
                                 Phases::Phase phase,
                                 PtrVec<FiltCond *> *filtConds,
                                 SizeVec *filtCondIdxs,
                                 BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        RUM_PTRC_COND(pool(), "CoreCondModule::parseXMLCond("
                      "const apr_xml_elem *elem, "
                      "apr_ssize_t ruleIdx"
                      "PtrVec<FiltCond *> filtConds, "
                      "SizeVec *filtCondIdxs), "
                      << "elem: " << elem->name
                      << "ruleIdx: " << ruleIdx
                      << ", filtConds: " << (void *)filtConds
                      << ", filtCondIdxs: " << (void *)filtCondIdxs);

        apr_xml_elem *childElem;
        for (childElem = elem->first_child;
             childElem != NULL;
             childElem = childElem->next) {

            if (strcmp(childElem->name, "ServerName") == 0)
            {
                const char *cdata = rum_xml_get_cdata(childElem, pTmp, 1);
                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing Core/" << childElem->name
                             << ": " << cdata);

                // create filtering condition which matches given server name
                CoreServerNameFiltCond coreFiltCond(pTmp, logger(),
                                                    phase, id(), cdata);
                storeUniqueFiltCond(coreFiltCond, filtConds, filtCondIdxs,
                                    filtCondIdxMap);
            }
            else
            {
                RUM_LOG_COND(logger(), APLOG_ERR, "unrecognized element in "
                             << condName()
                             << " condition: " << childElem->name);
                continue;
            }
        }

        // no narrowing conditions; match all rules during narrowing lookup
        matchAllReqs(ruleIdx, phase);

        return APR_SUCCESS;
    }



    ReqData *CoreCondModule::reqData(ReqCtx *reqCtx) const
    {
        return new (reqCtx->pool()) DummyReqData(0, reqCtx);
    }



    apr_status_t CoreCondModule::prepLuaAction(lua_State *L,
                                               ActionCtx *actionCtx) const
    {
        // create "rum.core" table; "rum" is on top of the stack
        lua_pushliteral(L, "core");
        lua_newtable(L);

        // register the methods in "rum.core"
        luaL_register(L, 0, rumCoreMethods);

        // create metatable
        lua_newtable(L);
        luaL_register(L, 0, rumCoreMetaMethods);
        lua_setmetatable(L, -2);

        // set newly created table
        lua_settable(L, -3);

        return APR_SUCCESS;
    }



    const char *CoreCondModule::condName_ = "Core";
}
