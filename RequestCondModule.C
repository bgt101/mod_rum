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
#include "util_lua.H"
#include "RequestCondModule.H"
#include "DummyReqData.H"
#include "RequestSubreqFiltCond.H"
#include "RequestIntRedirFiltCond.H"
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
#include "LuaRequestRec.H"
#include "LuaServerRec.H"
#include "LuaConnRec.H"
#include "LuaSockAddr.H"
#include "LuaTable.H"



namespace rum
{
    RequestCondModule::RequestCondModule(apr_pool_t *p, Logger *l,
                                         apr_ssize_t cmID)
        : CondModule(p, l, cmID)
    { }



    StrBuffer& RequestCondModule::write(StrBuffer& sb) const
    {
        return sb;
    }



    apr_status_t
    RequestCondModule::parseXMLCond(apr_pool_t *pTmp,
                                    const apr_xml_elem *elem,
                                    apr_ssize_t ruleIdx,
                                    Phases::Phase phase,
                                    PtrVec<FiltCond *> *filtConds,
                                    SizeVec *filtCondIdxs,
                                    BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        RUM_PTRC_COND(pool(), "RequestCondModule::parseXMLCond("
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

            if (strcmp(childElem->name, "IsSubrequest") == 0)
            {
                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing Request/" << childElem->name);

                // create a filtering condition matching only subrequests
                RequestSubreqFiltCond requestFiltCond(pTmp, logger(),
                                                      phase, id(), true);
                storeUniqueFiltCond(requestFiltCond, filtConds, filtCondIdxs,
                                    filtCondIdxMap);
            }
            else if (strcmp(childElem->name, "NotSubrequest") == 0)
            {
                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing Request/" << childElem->name);

                // create a filtering condition matching non-subrequests
                RequestSubreqFiltCond requestFiltCond(pTmp, logger(),
                                                      phase, id(), false);
                storeUniqueFiltCond(requestFiltCond, filtConds, filtCondIdxs,
                                    filtCondIdxMap);
            }
            else if (strcmp(childElem->name, "IsInternalRedirect") == 0)
            {
                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing Request/" << childElem->name);

                // create a filtering condition matching only
                // internal-redirects
                RequestIntRedirFiltCond requestFiltCond(pTmp, logger(),
                                                        phase, id(), true);
                storeUniqueFiltCond(requestFiltCond, filtConds, filtCondIdxs,
                                    filtCondIdxMap);
            }
            else if (strcmp(childElem->name, "NotInternalRedirect") == 0)
            {
                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing Request/" << childElem->name);

                // create a filtering condition matching non-internal-redirects
                RequestIntRedirFiltCond requestFiltCond(pTmp, logger(),
                                                        phase, id(), false);
                storeUniqueFiltCond(requestFiltCond, filtConds, filtCondIdxs,
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



    ReqData *RequestCondModule::reqData(ReqCtx *reqCtx) const
    {
        return new (reqCtx->pool()) DummyReqData(0, reqCtx);
    }



    void RequestCondModule::luaPrepEnvCB(lua_State *L, Logger *logger,
                                         apr_pool_t *pool__)
    {
        RUM_LOG_COND(logger, APLOG_DEBUG, "prepping Lua env");

        LuaRequestRec::luaPrepEnv(L, logger, pool__);
        LuaServerRec::luaPrepEnv(L, logger, pool__);
        LuaConnRec::luaPrepEnv(L, logger, pool__);
        LuaSockAddr::luaPrepEnv(L, logger, pool__);
        LuaTable::luaPrepEnv(L, logger, pool__);
    }



    apr_status_t RequestCondModule::prepLuaAction(lua_State *L,
                                                  ActionCtx *actionCtx) const
    {
        // create "rum.request"; "rum" is on top of the stack
        lua_pushliteral(L, "request");
        RUM_PUSH_PTR(request_rec, L, actionCtx->reqCtx()->req());
        lua_settable(L, -3);

        return APR_SUCCESS;
    }



    const char *RequestCondModule::condName_ = "Request";
}
