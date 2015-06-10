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



#include "QueryArgCondModule.H"
#include "QueryArgNameFiltCond.H"
#include "QueryArgNameValFiltCond.H"
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
#include "MatchedIdxs.H"



namespace rum
{

    // methods within "rum.queryarg"
    static const struct luaL_Reg qaMethods[] = {
        {"get", QueryArgCondModule::qaGet},
        {"raw_get", QueryArgCondModule::qaRawGet},
        {"mget", QueryArgCondModule::qaMget},
        {"raw_mget", QueryArgCondModule::qaRawMget},
        {"keys", QueryArgCondModule::qaKeys},
        {"reset", QueryArgCondModule::reset},
        {0, 0}
    };



    int QueryArgCondModule::getVal(lua_State *L, bool raw)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_PTRC_ACTION(actionCtx->reqCtx()->pool(),
                        "LuaAction::getVal(lua_State *L, bool raw)");

        const char *key = luaL_checkstring(L, 1);

        QueryArgReqData *qard =
            static_cast<QueryArgReqData *>
            (actionCtx->reqCtx()->reqData(condName_));
        if (qard == 0)
        {
            luaL_error(L, "ReqData is nil in getVal");
        }

        const StrVec *valVec;
        if (raw)
        {
            valVec = qard->queryArgRawValVec(key);
        }
        else
        {
            valVec = qard->queryArgValVec(key);
        }
        if (valVec)
        {
            // if value is NULL, push a boolean true
            const char *val = (valVec->size() > 0) ? (*valVec)[0] : "";
            if (val)
            {
                lua_pushstring(L, val);
            }
            else
            {
                lua_pushboolean(L, 1);
            }
        }
        else
        {
            // key doesn't exist, so return nil
            lua_pushnil(L);
        }

        return 1;
    }



    int QueryArgCondModule::qaGet(lua_State *L)
    {
        return getVal(L, false);
    }



    int QueryArgCondModule::qaRawGet(lua_State *L)
    {
        return getVal(L, true);
    }



    int QueryArgCondModule::getVals(lua_State *L, bool raw)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_PTRC_ACTION(actionCtx->reqCtx()->pool(),
                        "LuaAction::getVals(lua_State *L, bool raw)");

        const char *key = luaL_checkstring(L, 1);

        QueryArgReqData *qard =
            static_cast<QueryArgReqData *>
            (actionCtx->reqCtx()->reqData(condName_));
        if (qard == 0)
        {
            luaL_error(L, "ReqData is nil in getVals");
        }

        // return an array only if key exists, nil otherwise
        const StrVec *valVec;
        if (raw)
        {
            valVec = qard->queryArgRawValVec(key);
        }
        else
        {
            valVec = qard->queryArgValVec(key);
        }
        if (valVec)
        {
            int sz = static_cast<int>(valVec->size());
            lua_createtable(L, sz, 0);
            int i;
            for (i = 0; i < sz; i++)
            {
                // Lua indices start at 1
                lua_pushinteger(L, i + 1);

                // if value is NULL, push a boolean true
                const char *val = (*valVec)[i];
                if (val)
                {
                    lua_pushstring(L, val);
                }
                else
                {
                    lua_pushboolean(L, 1);
                }

                lua_settable(L, -3);
            }
        }
        else
        {
            lua_pushnil(L);
        }

        return 1;
    }



    int QueryArgCondModule::qaMget(lua_State *L)
    {
        return getVals(L, false);
    }



    int QueryArgCondModule::qaRawMget(lua_State *L)
    {
        return getVals(L, true);
    }



    int QueryArgCondModule::qaKeys(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_PTRC_ACTION(actionCtx->reqCtx()->pool(),
                        "LuaAction::qaKeys(lua_State *L)");

        QueryArgReqData *qard =
            static_cast<QueryArgReqData *>
            (actionCtx->reqCtx()->reqData(condName_));
        if (qard == 0)
        {
            luaL_error(L, "ReqData is nil in qaGet");
        }

        const StrPtrMap<StrVec *>& qavvm = qard->queryArgAnyValVecMap();
        int sz = static_cast<int>(qavvm.size());
        lua_createtable(L, sz, 0);
        StrPtrMap<StrVec *>::ConstIterator it(actionCtx->pool(), qavvm);
        int i = 1;
        while (it.next())
        {
            lua_pushinteger(L, i);
            lua_pushstring(L, it.key());
            lua_settable(L, -3);
            i++;
        }

        return 1;
    }



    int QueryArgCondModule::reset(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        RUM_PTRC_ACTION(actionCtx->reqCtx()->pool(),
                        "LuaAction::reset(lua_State *L)");

        QueryArgReqData *qard =
            static_cast<QueryArgReqData *>
            (actionCtx->reqCtx()->reqData(condName_));
        if (qard == 0)
        {
            luaL_error(L, "ReqData is nil in reset");
        }
        qard->reset();

        return 0;
    }



    QueryArgCondModule::QueryArgCondModule(apr_pool_t *p, Logger *l,
                                           apr_ssize_t cmID)
        : CondModule(p, l, cmID),
          queryArgNameRulesMapVec_(pool(), Phases::numPhases())
    {
        apr_ssize_t n = Phases::numPhases();
        apr_ssize_t i;
        for (i = 0; i < n; i++)
        {
            StrPtrMap<SizeVec *> *qaNameRulesMap =
                new (pool()) StrPtrMap<SizeVec *>(0);
            queryArgNameRulesMapVec_.push_back(qaNameRulesMap);
        }
    }



    StrBuffer& QueryArgCondModule::write(StrBuffer& sb) const
    {
        return sb << "queryArgNameRulesMapVec: " << nl << indent
                  << queryArgNameRulesMapVec_
                  << outdent;
    }



    void QueryArgCondModule::storeQueryArgName(const char *queryArgName,
                                               Phases::Phase phase,
                                               apr_ssize_t ruleIdx)
    {
        StrPtrMap<SizeVec *> *qaNameRulesMap = queryArgNameRulesMapVec_[phase];
        SizeVec *ruleIdxs = qaNameRulesMap->find(queryArgName);
        if (ruleIdxs == 0)
        {
            ruleIdxs = new (qaNameRulesMap->pool()) SizeVec(0);
            qaNameRulesMap->insert(queryArgName, ruleIdxs);
        }

        ruleIdxs->push_back(ruleIdx);
    }



    apr_status_t
    QueryArgCondModule::parseXMLCond(apr_pool_t *pTmp,
                                     const apr_xml_elem *elem,
                                     apr_ssize_t ruleIdx,
                                     Phases::Phase phase,
                                     PtrVec<FiltCond *> *filtConds,
                                     SizeVec *filtCondIdxs,
                                     BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        RUM_PTRC_COND(pool(), "QueryArgCondModule::parseXMLCond("
                      "const apr_xml_elem *elem, "
                      "apr_ssize_t ruleIdx"
                      "PtrVec<FiltCond *> filtConds, "
                      "SizeVec *filtCondIdxs), "
                      << "elem: " << elem->name
                      << "ruleIdx: " << ruleIdx
                      << ", filtConds: " << (void *)filtConds
                      << ", filtCondIdxs: " << (void *)filtCondIdxs);

        bool hasNarrowingCondition = false;
        apr_xml_elem *childElem;
        for (childElem = elem->first_child;
             childElem != NULL;
             childElem = childElem->next) {

            if (strcmp(childElem->name, "Name") == 0)
            {
                const char *queryArgName = rum_xml_get_cdata(childElem,
                                                             pTmp,
                                                             1);

                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing QueryArg/Name: " << queryArgName);

                storeQueryArgName(queryArgName, phase, ruleIdx);
                hasNarrowingCondition = true;


                // create a filtering condition for query arg name to
                // support multiple QueryArg conditions
                QueryArgNameFiltCond qaFiltCond(pTmp, logger(), phase, id(),
                                                queryArgName);
                storeUniqueFiltCond(qaFiltCond, filtConds, filtCondIdxs,
                                    filtCondIdxMap);
            }
            else if (strcmp(childElem->name, "NameVal") == 0)
            {
                const char *queryArgNameVal = rum_xml_get_cdata(childElem,
                                                                pTmp,
                                                                1);

                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing QueryArg/NameVal: " << queryArgNameVal);

                char *eq = strchr(const_cast<char *>(queryArgNameVal), '=');
                if (eq == 0)
                {
                    RUM_LOG_COND(logger(), APLOG_ERR,
                                 "invalid QueryArg/NameVal format: "
                                 << queryArgNameVal);

                    // parse next XML element
                    continue;
                }

                *eq = '\0';
                const char *queryArgName = queryArgNameVal;
                const char *queryArgVal = eq + 1;

                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing QueryArg/NameVal, name: "
                             << queryArgName);

                RUM_LOG_COND(logger(), APLOG_DEBUG,
                             "parsing QueryArg/NameVal, val: "
                             << queryArgVal);

                storeQueryArgName(queryArgName, phase, ruleIdx);
                hasNarrowingCondition = true;


                // create a filtering condition for query arg name and value
                QueryArgNameValFiltCond qaFiltCond(pTmp, logger(), phase,
                                                   id(), queryArgName,
                                                   queryArgVal);
                storeUniqueFiltCond(qaFiltCond, filtConds, filtCondIdxs,
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


        if (!hasNarrowingCondition)
        {
            matchAllReqs(ruleIdx, phase);
        }


        return APR_SUCCESS;
    }



    void QueryArgCondModule::lookup2(ReqCtx *reqCtx, Phases::Phase phase,
                                     MatchedIdxs *ruleIdxs) const
    {
        QueryArgReqData *rd =
            static_cast<QueryArgReqData *>(reqCtx->reqData(id()));

        const StrPtrMap<SizeVec *> *qaNameRulesMap =
            queryArgNameRulesMapVec_[phase];
        StrPtrMap<StrVec *>::ConstIterator it(reqCtx->pool(),
                                              rd->queryArgValVecMap());
        while (it.next())
        {
            const SizeVec *ruleIdxs2 = qaNameRulesMap->find(it.key());
            if (ruleIdxs2)
            {
                ruleIdxs->union_with(*ruleIdxs2);
            }
        }
    }



    ReqData *QueryArgCondModule::reqData(ReqCtx *reqCtx) const
    {
        return new (reqCtx->pool()) QueryArgReqData(0, reqCtx);
    }



    apr_status_t QueryArgCondModule::prepLuaAction(lua_State *L,
                                                   ActionCtx *actionCtx) const
    {
        // create rum.queryarg userdata and its metatable, then
        // register its methods into the the metatable; the metamethod
        // __index refers to the metatable itself; "rum" is on top of stack
        lua_newuserdata(L, 0);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_register(L, 0, qaMethods);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "queryarg");

        return APR_SUCCESS;
    }



    const char *QueryArgCondModule::condName_ = "QueryArg";
}
