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



#ifndef RUM_ACTIONCTX_H
#define RUM_ACTIONCTX_H


#include "PoolAllocated.H"



namespace rum
{
    // forward declarations
    class ReqCtx;



    class ActionCtx : public PoolAllocated
    {
    public:
        ActionCtx(apr_pool_t *p, ReqCtx *reqCtx__, apr_ssize_t ruleIdx__)
            : PoolAllocated(p),
              reqCtx_(reqCtx__),
              ruleIdx_(ruleIdx__),
              relookup_(false),
              lastAction_(false)
            { }



        virtual ~ActionCtx()
            { }



        ReqCtx *reqCtx()
            {
                return reqCtx_;
            }



        apr_ssize_t ruleIdx() const
            {
                return ruleIdx_;
            }



        bool relookup() const
            {
                return relookup_;
            }



        bool lastAction() const
            {
                return lastAction_;
            }



        void setRelookup(bool v)
            {
                relookup_ = v;
            }



        void setLastAction(bool v)
            {
                lastAction_ = v;
            }



    private:
        ReqCtx *reqCtx_;
        apr_ssize_t ruleIdx_;
        bool relookup_;
        bool lastAction_;



        ActionCtx(const ActionCtx &from)
            : PoolAllocated(from),
              reqCtx_(from.reqCtx_),
              ruleIdx_(from.ruleIdx_),
              relookup_(from.relookup_),
              lastAction_(from.lastAction_)
            {
                // this method is private and should not be used
            }



        ActionCtx& operator=(const ActionCtx& that)
            {
                // this method is private and should not be used
                PoolAllocated::operator=(that);
                return *this;
            }
    };
}


#endif // RUM_ACTIONCTX_H
