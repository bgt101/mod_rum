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



#include "CondModule.H"
#include "FiltCond.H"
#include "ReqCtx.H"
#include "Config.H"
#include "MatchedIdxs.H"



namespace rum
{
    CondModule::CondModule(apr_pool_t *p, Logger *l, apr_ssize_t cmID)
        : PoolAllocated(p),
          logger_(l),
          id_(cmID),
          matchAllRuleIdxsVec_(pool(), Phases::numPhases())
    {
        apr_ssize_t i;
        apr_ssize_t n = Phases::numPhases();
        for (i = 0; i < n; i++)
        {
            SizeVec *sv = new (pool()) SizeVec(0);
            matchAllRuleIdxsVec_.push_back(sv);
        }
    }



    // store the address of the new filter condition in the filter
    // conditions vector only if it's not identical to an existing
    // filter condition, and then store the index of the new or
    // equivalent filter condition from the filter conditions vector
    // in the filter conditions indexes vector
    void
    CondModule::storeUniqueFiltCond(const FiltCond& filtCond,
                                    PtrVec<FiltCond *> *filtConds,
                                    SizeVec *filtCondIdxs,
                                    BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        // determine whether condition is identical to an existing one
        apr_size_t condIdx;
        apr_size_t *idxPtr = filtCondIdxMap->find(filtCond.blobID());
        if (idxPtr)
        {
            // identical condition found, so use that instead
            condIdx = *idxPtr;
        }
        else
        {
            // condition is unique, so store copy of it
            filtConds->push_back(filtCond.clone(filtConds->pool()));
            condIdx = filtConds->size() - 1;
            filtCondIdxMap->insert(filtCond.blobID(), condIdx);
        }

        // store index of condition
        filtCondIdxs->push_back(condIdx);
    }



    void CondModule::postConfProc()
    {
        apr_ssize_t i;
        apr_ssize_t n = Phases::numPhases();
        for (i = 0; i < n; i++)
        {
            SizeVec *rIdxs = matchAllRuleIdxsVec_[i];
            rIdxs->sort();
            rIdxs->unique();
        }
        postConfigProc();
    }



    void CondModule::matchAllReqs(apr_ssize_t ruleIdx, Phases::Phase phase)
    {
        RUM_PTRC_COND(pool(), "CondModule::matchAllReqs("
                      "apr_ssize_t ruleIdx), "
                      "ruleIdx: " << ruleIdx);

        matchAllRuleIdxsVec_[phase]->push_back(ruleIdx);
    }



    void CondModule::lookup(ReqCtx *reqCtx, Phases::Phase phase,
                            MatchedIdxs *ruleIdxs) const
    {
        const SizeVec &mari = *matchAllRuleIdxsVec_[phase];
        if (mari.size() < reqCtx->config().numPhaseRules(phase))
        {
            // not every rule matches every request, so do a lookup

            lookup2(reqCtx, phase, ruleIdxs);
            ruleIdxs->union_with(mari);
        }
        else
        {
            // every rule matches every request

            ruleIdxs->setMatchAll();
        }
    }


}
