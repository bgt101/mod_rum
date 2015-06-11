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



#ifndef RUM_CORECONDMODULE_H
#define RUM_CORECONDMODULE_H


#include "CondModule.H"



// forward declarations
class lua_State;



namespace rum
{
    ///
    /// CoreCondModule provides the RUM interface to a request's core
    /// properties.
    ///
    class CoreCondModule : public CondModule
    {
    public:
        CoreCondModule(apr_pool_t *p, Logger *l, apr_ssize_t cmID)
            : CondModule(p, l, cmID)
            { }



        virtual ~CoreCondModule() { }



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



        const FiltCond *cloneFiltCond(apr_pool_t *p, const FiltCond *filtCond);




        virtual void postConfigProc() { }



        void lookup2(ReqCtx *reqCtx, Phases::Phase phase,
                     MatchedIdxs *ruleIdxs) const { }



        ReqData *reqData(ReqCtx *reqCtx) const;



        apr_status_t prepLuaAction(lua_State *L, ActionCtx *actionCtx) const;



        // rum.core methods
        static int coreInternalRedirect(lua_State *L);
        static int coreExternalRedirect(lua_State *L);
        static int coreLogEmerg(lua_State *L);
        static int coreLogAlert(lua_State *L);
        static int coreLogCrit(lua_State *L);
        static int coreLogError(lua_State *L);
        static int coreLogWarn(lua_State *L);
        static int coreLogNotice(lua_State *L);
        static int coreLogInfo(lua_State *L);
        static int coreLogDebug(lua_State *L);
        static int coreSetContentType(lua_State *L);
        static int coreProxy(lua_State *L);
        static int corePuts(lua_State *L);
        static int coreRflush(lua_State *L);
        static int coreRputs(lua_State *L);
        static int coreRwrite(lua_State *L);
        static int coreInteractiveLua(lua_State *L);
        static int coreState(lua_State *L);
        static int coreAPIVersionMajor(lua_State *L);
        static int coreAPIVersionMinor(lua_State *L);



        // rum.core metamethods
        static int coreIndex(lua_State *L);
        static int coreNewindex(lua_State *L);



    private:
        // condition name as it appears in the configs
        static const char *condName_;



        CoreCondModule(const CoreCondModule& from)
            : CondModule(from)
            {
                // this method is private and should not be used
            }



        CoreCondModule& operator=(const CoreCondModule& /* that */)
            {
                // this method is private and should not be used
                /* CondModule::operator=(that); */
                return *this;
            }



        void storeCoreName(const char *coreName, apr_ssize_t ruleIdx);
    };
}


#endif // RUM_CORECONDMODULE_H
