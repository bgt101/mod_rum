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



#ifndef RUM_RULE_H
#define RUM_RULE_H


#include "PoolAllocated.H"
#include "SizeVec.H"
#include "PtrVec.H"
#include "Action.H"
#include "Phases.H"



namespace rum
{
    class Rule : public PoolAllocatedLight
    {
    public:
        Rule(apr_pool_t *);



        virtual ~Rule();



        SizeVec *filtCondIdxs() { return &filtCondIdxs_; }



        const SizeVec *filtCondIdxs() const { return &filtCondIdxs_; }



        const PtrVec<Action *>& actions() const
            {
                return actions_;
            }



        void addAction(const Action *action)
            {
                actions_.push_back(action);
            }



        void setCondPhase(Phases::Phase condPhase__)
            {
                condPhase_ = condPhase__;
            }



        Phases::Phase condPhase() const
            {
                return condPhase_;
            }



    private:
        Phases::Phase condPhase_;
        SizeVec filtCondIdxs_;
        PtrVec<Action *> actions_;



        Rule(const Rule &from)
            : PoolAllocatedLight(from),
              condPhase_(Phases::InvalidPhase),
              filtCondIdxs_(0),
              actions_(0)
            {
                // this method is private and should not be used
            }



        Rule& operator=(const Rule& that)
            {
                // this method is private and should not be used
                PoolAllocatedLight::operator=(that);
                return *this;
            }



        friend StrBuffer& operator<<(StrBuffer& sb, const Rule& rule);
    };



    inline StrBuffer& operator<<(StrBuffer& sb, const Rule& rule)
    {
        return sb << "condPhase: " << Phases::enum2str(rule.condPhase()) << nl
                  << "filtCondIdxs: " << indent << nl << rule.filtCondIdxs_
                  << outdent << nl
                  << "actions: " << indent << nl
                  << rule.actions_ << outdent;
    }
}


#endif // RUM_RULE_H
