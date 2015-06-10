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



#include "PathCondModule.H"
#include "PathReqData.H"
#include "StrBuffer.H"
#include "PathRegExFiltCond.H"
#include "PathMatchedFiltCond.H"
#include "PathSlashFiltCond.H"
#include "PathFiltCondMatch.H"
#include "SizeVec.H"
#include "apr_xml.h"
#include "get_cdata.H"
#include "ReqCtx.H"
#include "ActionCtx.H"
#include "LuaAction.H"
#include "lua.hpp"
#include "Config.H"



namespace rum
{


    // methods within "rum.path"
    static const struct luaL_Reg pathMethods[] = {
        {"trailing_slash", PathCondModule::trailingSlash},
        {0, 0}
    };



    // methods of rum.path.tokens
    const struct luaL_Reg tokensMethods[] = {
        {"__index", PathCondModule::tokensIndex},
        {"__len", PathCondModule::tokensLen},
        {"__tostring", PathCondModule::tokensTostring},
        {0, 0}
    };



    // methods of rum.path.captures
    const struct luaL_Reg capturesMethods[] = {
        {"__index", PathCondModule::capturesIndex},
        {"__len", PathCondModule::capturesLen},
        {"__tostring", PathCondModule::capturesTostring},
        {0, 0}
    };



    // methods of rum.path.mcaptures
    const struct luaL_Reg mcapturesMethods[] = {
        {"__index", PathCondModule::mcapturesIndex},
        {"__len", PathCondModule::mcapturesLen},
        {0, 0}
    };



    PathCondModule::PathCondModule(apr_pool_t *p, Logger *l, apr_ssize_t cmID)
        : CondModule(p, l, cmID),
          tokenMatcherVec_(pool(), Phases::numPhases())
    {
        apr_ssize_t n = Phases::numPhases();
        apr_ssize_t i;
        for (i = 0; i < n; i++)
        {
            TokenMatcher *tm = new (pool()) TokenMatcher(0, l, '/', 3, 3);
            tokenMatcherVec_.push_back(tm);
        }
    }



    void PathCondModule::postConfigProc()
    {
        apr_ssize_t n = Phases::numPhases();
        apr_ssize_t i;
        for (i = 0; i < n; i++)
        {
            tokenMatcherVec_[i]->postProc();
        }
    }



    StrBuffer& PathCondModule::write(StrBuffer& sb) const
    {
        return sb << "TokenMatcher: " << nl << indent << tokenMatcherVec_
                  << outdent;
    }



    apr_status_t
    PathCondModule::parseXMLCond(apr_pool_t *pTmp,
                                 const apr_xml_elem *elem,
                                 apr_ssize_t ruleIdx,
                                 Phases::Phase phase,
                                 PtrVec<FiltCond *> *filtConds,
                                 SizeVec *filtCondIdxs,
                                 BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        RUM_PTRC_COND(pool(), "PathCondModule::parseXMLCond("
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

            if (strcmp(childElem->name, "RegEx") == 0)
            {
                const char *regExStr = rum_xml_get_cdata(childElem,
                                                         pTmp,
                                                         1);
                PathRegExFiltCond newCond(pTmp, logger(), phase, id(),
                                          regExStr, false, 0);
                storeUniqueFiltCond(newCond, filtConds, filtCondIdxs,
                                    filtCondIdxMap);
            }
            else if (strcmp(childElem->name, "Pattern") == 0)
            {
                bool trailingSlashOptional = false;
                apr_xml_attr *attr;
                for (attr = childElem->attr; attr != NULL; attr = attr->next) 
                {
                    if (strcmp(attr->name, "trailingSlashOptional") == 0)
                    {
                        if ((strcmp(attr->value, "true") == 0) ||
                            (strcmp(attr->value, "1") == 0))
                        {
                            trailingSlashOptional = true;
                        }
                    }
                    else
                    {
                        RUM_LOG_COND(logger(), APLOG_ERR,
                                     "unrecognized attribute, condition: "
                                     << condName()
                                     << "/"
                                     << childElem->name
                                     << ", attribute: "
                                     << attr->name);
                    }
                }


                hasNarrowingCondition = true;
                const char *pattern = rum_xml_get_cdata(childElem,
                                                        pTmp,
                                                        1);

                StrBuffer regExStr(pTmp);
                bool matchAll;
                apr_size_t numClusters;

                TokenMatcher *tm = tokenMatcherVec_[phase];
                tm->procPattern(pattern, pTmp, ruleIdx, &regExStr,
                                &numClusters, "^/+", "/*$", &matchAll);

                if (regExStr.size() > 0)
                {
                    PathRegExFiltCond newCond(pTmp, logger(), phase, id(),
                                              regExStr, false, 0);
                    storeUniqueFiltCond(newCond, filtConds, filtCondIdxs,
                                        filtCondIdxMap);
                }
                else
                {
                    // create a filter condition that always returns
                    // true, and is used to simply capture the match
                    // data
                    PathMatchedFiltCond matchedCond(pTmp, logger(),
                                                    phase, id());
                    storeUniqueFiltCond(matchedCond, filtConds, filtCondIdxs,
                                        filtCondIdxMap);
                }

                if (!trailingSlashOptional)
                {
                    apr_ssize_t len = strlen(pattern);
                    bool hasTrailSlash =
                        (len == 0) ? false : (*(pattern + len - 1) == '/');
                    PathSlashFiltCond slashCond(pTmp, logger(), phase, id(),
                                                hasTrailSlash);
                    storeUniqueFiltCond(slashCond, filtConds, filtCondIdxs,
                                        filtCondIdxMap);
                }

                RUM_LOG_COND(logger(), APLOG_DEBUG, "ruleIdx: " << ruleIdx
                             << " matchAll: " << matchAll);

                if (matchAll)
                {
                    matchAllReqs(ruleIdx, phase);
                }
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



    void PathCondModule::lookup2(ReqCtx *reqCtx, Phases::Phase phase,
                                 MatchedIdxs *ruleIdxs) const
    {
        const TokenMatcher *tm = tokenMatcherVec_[phase];
        PathReqData *rd =
            static_cast<PathReqData *>(reqCtx->reqData(id()));
        tm->lookup(reqCtx->logger(), rd->pathTokens(), ruleIdxs);
    }



    ReqData *PathCondModule::reqData(ReqCtx *reqCtx) const
    {
        return new (reqCtx->pool()) PathReqData(0, reqCtx);
    }



    PathReqData *PathCondModule::reqData(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        PathReqData *prd =
            static_cast<PathReqData *>
            (actionCtx->reqCtx()->reqData(condName_));
        if (prd == 0)
        {
            luaL_error(L, "ReqData is nil");
        }
        return prd;
    }



    const StrVec *PathCondModule::captures(ActionCtx *actionCtx,
                                           apr_ssize_t num)
    {
        ReqCtx *reqCtx = actionCtx->reqCtx();
        const StrVec *captures = 0;
        const CondModule *cm =
            reqCtx->config().condModulesMap().find(condName_);
        const apr_ssize_t cmID = cm->id();
        const Rule *rule = reqCtx->config().rules()[actionCtx->ruleIdx()];
        const SizeVec& filtCondIdxs = *(rule->filtCondIdxs());
        const apr_ssize_t fcisz = filtCondIdxs.size();
        apr_ssize_t n = 0;
        apr_ssize_t i;
        bool found = false;
        apr_ssize_t filtCondIdx;
        const FiltCond *filtCond;
        for (i = 0; !found && (i < fcisz); i++)
        {
            filtCondIdx = filtCondIdxs[i];
            filtCond = reqCtx->config().filtConds()[filtCondIdx];
            if (filtCond->id() == cmID)
            {
                const PathFiltCond *pfc =
                    static_cast<const PathFiltCond *>(filtCond);
                if (pfc->hasCaptures())
                {
                    if (n == num)
                    {
                        found = true;
                    }
                    else
                    {
                        n++;
                    }
                }
            }
        }

        if (found)
        {
            const PathFiltCondMatch *pfcm =
                static_cast<const PathFiltCondMatch *>
                (reqCtx->filtCondMatches()->find(filtCondIdx));
            if (pfcm)
            {
                captures = pfcm->captures();
            }
        }

        return captures;
    }



    int PathCondModule::capturesIndex(lua_State *L)
    {
        // remember, in Lua indices are 1-based

        int index = luaL_checkint(L, 2);
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const StrVec *capVec = captures(actionCtx, 0);
        if ((capVec != 0) && (index > 0) && (capVec->size() >= index))
        {
            lua_pushstring(L, (*capVec)[index - 1]);
        }
        else
        {
            lua_pushnil(L);
        }
        return 1;
    }



    int PathCondModule::capturesLen(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const StrVec *capVec = captures(actionCtx, 0);
        int len = 0;
        if (capVec != 0)
        {
            len = static_cast<int>(capVec->size());
        }

        lua_pushinteger(L, len);
        return 1;
    }



    int PathCondModule::capturesTostring(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const StrVec *capVec = captures(actionCtx, 0);
        StrBuffer buf(actionCtx->reqCtx()->pool());
        buf << "{";
        if (capVec != 0)
        {
            const apr_ssize_t sz = capVec->size();
            apr_ssize_t i;
            for (i = 0; i < sz; i++)
            {
                if (i > 0)
                {
                    buf << ", \"" << (*capVec)[i] << "\"";
                }
                else
                {
                    buf << "\"" << (*capVec)[i] << "\"";
                }
            }
        }
        buf << "}";

        lua_pushstring(L, buf);
        return 1;
    }



    int PathCondModule::mcapturesIndex(lua_State *L)
    {
        // remember, in Lua indices are 1-based

        int index = luaL_checkint(L, 2);
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const StrVec *capVec = captures(actionCtx, index - 1);
        if (capVec)
        {
            int sz = static_cast<int>(capVec->size());
            lua_createtable(L, sz, 0);
            int i;
            for (i = 0; i < sz; i++)
            {
                // Lua indices start at 1
                lua_pushinteger(L, i + 1);
                lua_pushstring(L, (*capVec)[i]);
                lua_settable(L, -3);
            }
        }
        else
        {
            lua_pushnil(L);
        }
        return 1;
    }



    int PathCondModule::mcapturesLen(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        ReqCtx *reqCtx = actionCtx->reqCtx();
        const CondModule *cm =
            reqCtx->config().condModulesMap().find(condName_);
        const apr_ssize_t cmID = cm->id();
        const Rule *rule = reqCtx->config().rules()[actionCtx->ruleIdx()];
        const SizeVec& filtCondIdxs = *(rule->filtCondIdxs());
        const apr_ssize_t fcisz = filtCondIdxs.size();
        apr_ssize_t n = 0;
        apr_ssize_t i;
        apr_ssize_t filtCondIdx;
        const FiltCond *filtCond;
        for (i = 0; i < fcisz; i++)
        {
            filtCondIdx = filtCondIdxs[i];
            filtCond = reqCtx->config().filtConds()[filtCondIdx];
            if (filtCond->id() == cmID)
            {
                const PathFiltCond *pfc =
                    static_cast<const PathFiltCond *>(filtCond);
                if (pfc->hasCaptures())
                {
                    n++;
                }
            }
        }

        lua_pushinteger(L, n);
        return 1;
    }



    int PathCondModule::tokensIndex(lua_State *L)
    {
        // remember, in Lua indices are 1-based
        int index = luaL_checkint(L, 2);
        PathReqData *prd = reqData(L);
        apr_ssize_t sz = prd->pathTokens().size();
        if ((index < 1) || (index > sz))
        {
            lua_pushnil(L);
        }
        else
        {
            lua_pushstring(L, prd->pathTokens()[index - 1]);
        }
        return 1;
    }



    int PathCondModule::tokensLen(lua_State *L)
    {
        lua_pushinteger(L, reqData(L)->pathTokens().size());
        return 1;
    }



    int PathCondModule::tokensTostring(lua_State *L)
    {
        PathReqData *prd = reqData(L);
        StrBuffer buf(prd->pool());
        buf << "{";
        const apr_ssize_t sz = prd->pathTokens().size();
        apr_ssize_t i;
        for (i = 0; i < sz; i++)
        {
            if (i > 0)
            {
                buf << ", \"" << prd->pathTokens()[i] << "\"";
            }
            else
            {
                buf << "\"" << prd->pathTokens()[i] << "\"";
            }
        }
        buf << "}";
        lua_pushstring(L, buf);
        return 1;
    }



    int PathCondModule::trailingSlash(lua_State *L)
    {
        ActionCtx *actionCtx = LuaAction::getActionCtx(L);
        const char *uri = actionCtx->reqCtx()->req()->uri;
        lua_pushboolean(L, (uri && *uri && (uri[strlen(uri) - 1] == '/')));
        return 1;
    }



    apr_status_t PathCondModule::prepLuaAction(lua_State *L,
                                               ActionCtx *actionCtx) const
    {
        // create rum.path table, plus the tokens, captures and
        // mcaptures userdata inside it; "rum" is on top of stack
        lua_newtable(L);

        lua_newuserdata(L, 0);
        lua_newtable(L);
        luaL_register(L, 0, tokensMethods);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "tokens");

        lua_newuserdata(L, 0);
        lua_newtable(L);
        luaL_register(L, 0, capturesMethods);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "captures");

        lua_newuserdata(L, 0);
        lua_newtable(L);
        luaL_register(L, 0, mcapturesMethods);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "mcaptures");

        luaL_register(L, 0, pathMethods);

        lua_setfield(L, -2, "path");

        return APR_SUCCESS;
    }



    const char *PathCondModule::condName_ = "Path";
}
