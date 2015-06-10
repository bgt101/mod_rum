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



#ifndef RUM_QUERYARGCONDMODULE_H
#define RUM_QUERYARGCONDMODULE_H


#include "CondModule.H"
#include "StrPtrMap.H"
#include "SizeVec.H"
#include "QueryArgReqData.H"



// forward declarations
class lua_State;



namespace rum
{
    ///
    /// QueryArgCondModule provides the RUM interface to a request's
    /// query arguments.  It performs quick hash based lookups for
    /// query argument names, and uses filtering rules to match query
    /// argument values.
    ///
    class QueryArgCondModule : public CondModule
    {
    public:
        QueryArgCondModule(apr_pool_t *p, Logger *l, apr_ssize_t cmID);



        virtual ~QueryArgCondModule() { }



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
                     MatchedIdxs *ruleIdxs) const;



        ReqData *reqData(ReqCtx *reqCtx) const;



        apr_status_t prepLuaAction(lua_State *L, ActionCtx *actionCtx) const;



        // queryarg table operation methods
        static int qaGet(lua_State *L);
        static int qaRawGet(lua_State *L);
        static int qaMget(lua_State *L);
        static int qaRawMget(lua_State *L);
        static int qaKeys(lua_State *L);
        static int reset(lua_State *L);

        static int getVal(lua_State *L, bool raw);
        static int getVals(lua_State *L, bool raw);



    private:
        // vector of hash maps for each phase, used in storing
        // narrowing conditions; key is query argument name, value is
        // vector of indices of rules which contain a condition where
        // the query argument name appears
        PtrVec<StrPtrMap<SizeVec *> *> queryArgNameRulesMapVec_;



        // condition name as it appears in the configs
        static const char *condName_;



        QueryArgCondModule(const QueryArgCondModule& from)
            : CondModule(from),
              queryArgNameRulesMapVec_(0)
            {
                // this method is private and should not be used
            }



        QueryArgCondModule& operator=(const QueryArgCondModule& /* that */)
            {
                // this method is private and should not be used
                /* CondModule::operator=(that); */
                return *this;
            }



        void storeQueryArgName(const char *queryArgName, Phases::Phase phase,
                               apr_ssize_t ruleIdx);
    };
}


#endif // RUM_QUERYARGCONDMODULE_H
