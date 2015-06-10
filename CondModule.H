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



#ifndef RUM_CONDMODULE_H
#define RUM_CONDMODULE_H


#include "PoolAllocated.H"
#include "PtrVec.H"
#include "SizeVec.H"
#include "apr_errno.h"
#include "Logger.H"
#include "LuaManager.H"
#include "Phases.H"
#include "BlobSmplMap.H"



// forward declarations
class apr_xml_elem;
class lua_State;



namespace rum
{
    // forward declarations
    class StrBuffer;
    class FiltCond;
    class ReqCtx;
    class ActionCtx;
    class ReqData;
    class MatchedIdxs;



    class CondModule : public PoolAllocated
    {
    public:
        CondModule(apr_pool_t *p, Logger *l, apr_ssize_t cmID);



        virtual ~CondModule() { }



        virtual const char *condName() const = 0;



        virtual StrBuffer& write(StrBuffer& sb) const = 0;



        virtual apr_status_t
        parseXMLCond(apr_pool_t *pTmp,
                     const apr_xml_elem *elem,
                     apr_ssize_t ruleIdx,
                     Phases::Phase phase,
                     PtrVec<FiltCond *> *filtConds,
                     SizeVec *filtCondIdxs,
                     BlobSmplMap<apr_size_t> *filtCondIdxMap) = 0;



        void storeUniqueFiltCond(const FiltCond& filtCond,
                                 PtrVec<FiltCond *> *filtConds,
                                 SizeVec *filtCondIdxs,
                                 BlobSmplMap<apr_size_t> *filtCondIdxMap);



        virtual void matchAllReqs(apr_ssize_t ruleIdx, Phases::Phase phase);



        virtual void postConfProc();



        virtual void postConfigProc() = 0;



        virtual void lookup(ReqCtx *reqCtx, Phases::Phase phase,
                            MatchedIdxs *ruleIdxs) const;



        virtual void lookup2(ReqCtx *reqCtx, Phases::Phase phase,
                             MatchedIdxs *ruleIdxs) const = 0;



        virtual ReqData *reqData(ReqCtx *reqCtx) const = 0;



        virtual apr_status_t prepLuaAction(lua_State *L,
                                           ActionCtx *actionCtx) const = 0;



        apr_ssize_t id() const
            {
                return id_;
            }



        Logger *logger()
            {
                return logger_;
            }



        virtual PrepEnvCBFunc getLuaPrepEnvCB() const
            {
                return NULL;
            }



    protected:
        CondModule(const CondModule& from)
            : PoolAllocated(from),
              logger_(0),
              id_(0),
              matchAllRuleIdxsVec_(0)
            { }



    private:
        Logger *logger_;
        apr_ssize_t id_;
        PtrVec<SizeVec *> matchAllRuleIdxsVec_;



        CondModule& operator=(const CondModule& that)
            {
                PoolAllocated::operator=(that);
                return *this;
            }



        friend StrBuffer& operator<<(StrBuffer& sb, const CondModule& cm);

    };



    inline StrBuffer& operator<<(StrBuffer& sb, const CondModule& cm)
    {
        sb << "name: \"" << cm.condName() << "\"" << nl
           << "matchAllRuleIdxs: " << indent << nl
           << cm.matchAllRuleIdxsVec_ << outdent << nl;

        return cm.write(sb);
    }

}


#endif // RUM_CONDMODULE_H
