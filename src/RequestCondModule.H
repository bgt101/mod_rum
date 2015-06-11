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



#ifndef RUM_REQUESTCONDMODULE_H
#define RUM_REQUESTCONDMODULE_H


#include "CondModule.H"



// forward declarations
class lua_State;



namespace rum
{
    ///
    /// RequestCondModule provides the RUM interface to the
    /// request_rec structure.
    ///
    class RequestCondModule : public CondModule
    {
    public:
        RequestCondModule(apr_pool_t *p, Logger *l, apr_ssize_t cmID);



        virtual ~RequestCondModule() { }



        virtual const char *condName() const
            {
                return condName_;
            }



        virtual StrBuffer& write(StrBuffer& sb) const;



        apr_status_t parseXMLCond(apr_pool_t *pTmp,
                                  const apr_xml_elem *elem,
                                  apr_ssize_t ruleIdx,
                                  Phases::Phase phase,
                                  PtrVec<FiltCond *> *filtConds,
                                  SizeVec *filtCondIdxs,
                                  BlobSmplMap<apr_size_t> *filtCondIdxMap);



        virtual void postConfigProc() { }



        void lookup2(ReqCtx *reqCtx, Phases::Phase phase,
                     MatchedIdxs *ruleIdxs) const { }



        ReqData *reqData(ReqCtx *reqCtx) const;



        PrepEnvCBFunc getLuaPrepEnvCB() const
            {
                return luaPrepEnvCB;
            }



        static void luaPrepEnvCB(lua_State *, Logger *, apr_pool_t *);



        apr_status_t prepLuaAction(lua_State *L, ActionCtx *actionCtx) const;



    private:
        // condition name as it appears in the configs
        static const char *condName_;



        RequestCondModule(const RequestCondModule& from)
            : CondModule(from)
            {
                // this method is private and should not be used
            }



        RequestCondModule& operator=(const RequestCondModule& /* that */)
            {
                // this method is private and should not be used
                /*CondModule::operator=(that);*/
                return *this;
            }



        void storeRequestName(const char *requestName, apr_ssize_t ruleIdx);
    };
}


#endif // RUM_REQUESTCONDMODULE_H
