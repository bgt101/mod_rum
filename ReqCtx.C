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



#include "debug.H"
#include "ReqCtx.H"
#include "Config.H"



namespace rum
{
    ReqCtx::ReqCtx(apr_pool_t *p, Logger *l, request_rec *req__,
                   const Config& config__)
        : PoolAllocated(p),
          logger_(l),
          req_(req__),
          firstLookup_(true),
          handleReq_(false),
          config_(config__),
          luaRef_(0),
          curPhase_(Phases::InvalidPhase),
          curRuleIdxs_(pool()),
          newRuleIdxs_(pool()),
          allRuleIdxs_(pool()),
          allRuleIdxsValid_(false),
          filtCondMatches_(pool()),
          condModuleReqDataVec_(pool(), config_.condModulesMap().size())
    {
        // store each condition module's request data using their ID as index
        condModuleReqDataVec_.grow_to(config_.condModulesMap().size());
        StrPtrMap<CondModule *>::ConstIterator it(pool(),
                                                  config_.condModulesMap());
        while (it.next())
        {
            condModuleReqDataVec_[it.val()->id()] = it.val()->reqData(this);
        }
    }



    ReqCtx::~ReqCtx()
    {
        delete luaRef_;
    }



    ReqData *ReqCtx::reqData(const char *condName)
    {
        ReqData *rd = 0;
        const CondModule *cm = config_.condModulesMap().find(condName);
        if (cm)
        {
            rd = reqData(cm->id());
        }

        return rd;
    }



    void ReqCtx::resetForLookup(Phases::Phase phase)
    {
        if (firstLookup_)
        {
            firstLookup_ = false;
        }
        else
        {
            apr_ssize_t i, sz;

            SmplPtrMap<apr_ssize_t, FiltCondMatch *>::Iterator
                it(pool(), filtCondMatches_);
            while (it.next())
            {
                if (config_.filtConds()[it.key()]->phase() == phase)
                {
                    filtCondMatches_.insert(it.key(), NULL);
                }
            }

            sz = condModuleReqDataVec_.size();
            for (i = 0; i < sz; i++)
            {
                condModuleReqDataVec_[i]->reset();
            }

            if (curPhase_ != phase)
            {
                curPhase_ = phase;
                curRuleIdxs_.union_with(newRuleIdxs_);
            }
            newRuleIdxs_.clear();
            allRuleIdxsValid_ = false;
        }
    }



    lua_State *ReqCtx::luaState()
    {
        if (luaRef_ == 0)
        {
            luaRef_ = new (pool())
                      LuaManager::LuaRef(pool(), config_.luaManager());
        }

        if (luaRef_->isValid())
        {
            return luaRef_->state();
        }
        else
        {
            return 0;
        }
    }



    void ReqCtx::fullLuaGC()
    {
        if (luaRef_ != 0)
        {
            // go through LuaManager to keep from having to call low
            // level Lua API functions from here
            LuaManager::fullLuaGC(luaRef_->state());
        }
    }

}
