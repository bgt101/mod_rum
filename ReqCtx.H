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



#ifndef RUM_REQCTX_H
#define RUM_REQCTX_H


#include "httpd.h"
#include "PoolAllocated.H"
#include "SizeVec.H"
#include "FiltCondMatch.H"
#include "LuaManager.H"
#include "PtrVec.H"
#include "ReqData.H"
#include "Phases.H"
#include "SmplPtrMap.H"



namespace rum
{
    // forward declarations
    class Config;
    class Logger;



    class ReqCtx : public PoolAllocated
    {
    public:
        ReqCtx(apr_pool_t *p, Logger *l, request_rec *req__,
               const Config& config__);



        virtual ~ReqCtx();



        request_rec *req()
            {
                return req_;
            }



        const Config& config()
            {
                return config_;
            }



        SmplPtrMap<apr_ssize_t, FiltCondMatch *> *filtCondMatches()
            {
                return &filtCondMatches_;
            }



        ReqData *reqData(apr_ssize_t id)
            {
                return condModuleReqDataVec_[id];
            }



        ReqData *reqData(const char *condName);



        void addRuleIdx(Phases::Phase phase, apr_ssize_t ruleIdx)
            {
                if (curPhase_ != phase)
                {
                    curPhase_ = phase;
                    curRuleIdxs_.union_with(newRuleIdxs_);
                    newRuleIdxs_.clear();
                }
                newRuleIdxs_.push_back(ruleIdx);
                allRuleIdxsValid_ = false;
            }



        const SizeVec& ruleIdxs()
            {
                if (!allRuleIdxsValid_)
                {
                    allRuleIdxs_ = curRuleIdxs_;
                    allRuleIdxs_.union_with(newRuleIdxs_);
                    allRuleIdxsValid_ = true;
                }

                return allRuleIdxs_;
            }



        Logger *logger()
            {
                return logger_;
            }


        void doHandleReq()
            {
                handleReq_ = true;
            }



        bool handleReq()
            {
                return handleReq_;
            }



        void resetForLookup(Phases::Phase phase);



        lua_State *luaState();



        void fullLuaGC();



    private:
        Logger *logger_;
        request_rec *req_;
        bool firstLookup_;
        bool handleReq_;
        const Config& config_;
        LuaManager::LuaRef *luaRef_;
        Phases::Phase curPhase_;
        SizeVec curRuleIdxs_;
        SizeVec newRuleIdxs_;
        SizeVec allRuleIdxs_;
        bool allRuleIdxsValid_;
        SmplPtrMap<apr_ssize_t, FiltCondMatch *> filtCondMatches_;
        PtrVec<ReqData *> condModuleReqDataVec_;



        ReqCtx(const ReqCtx &from)
            : PoolAllocated(from),
              logger_(0),
              req_(0),
              firstLookup_(false),
              handleReq_(false),
              config_(from.config_),
              luaRef_(from.luaRef_),
              curPhase_(Phases::InvalidPhase),
              curRuleIdxs_(0),
              newRuleIdxs_(0),
              allRuleIdxs_(0),
              allRuleIdxsValid_(false),
              filtCondMatches_(0),
              condModuleReqDataVec_(0)
            {
                // this method is private and should not be used
            }



        ReqCtx& operator=(const ReqCtx& that)
            {
                // this method is private and should not be used
                PoolAllocated::operator=(that);
                return *this;
            }



        friend StrBuffer& operator<<(StrBuffer& sb, const ReqCtx& ReqCtx);
    };



    inline StrBuffer& operator<<(StrBuffer& sb, const ReqCtx& reqCtx)
    {
        return sb << "request_rec: " << (void *)reqCtx.req_ << nl
                  << "curRuleIdxs: " << indent << nl << reqCtx.curRuleIdxs_
                  << "newRuleIdxs: " << indent << nl << reqCtx.newRuleIdxs_
                  << outdent << nl
                  << "condModuleReqDataVec: " << indent << nl
                  << reqCtx.condModuleReqDataVec_ << outdent
                  << "filtCondMatches: " << indent << nl
                  << reqCtx.filtCondMatches_ << outdent;
    }
}


#endif // RUM_REQCTX_H
