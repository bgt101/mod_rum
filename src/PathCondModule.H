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



#ifndef RUM_PATHCONDMODULE_H
#define RUM_PATHCONDMODULE_H


#include "CondModule.H"
#include "TokenMatcher.H"



// forward declarations
class lua_State;



namespace rum
{
    // forward declarations
    class PathReqData;



    class PathCondModule : public CondModule
    {
    public:
        PathCondModule(apr_pool_t *p, Logger *l, apr_ssize_t cmID);



        virtual ~PathCondModule() { }



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



        virtual void postConfigProc();



        void lookup2(ReqCtx *reqCtx, Phases::Phase phase,
                     MatchedIdxs *ruleIdxs) const;



        ReqData *reqData(ReqCtx *reqCtx) const;



        apr_status_t prepLuaAction(lua_State *L, ActionCtx *actionCtx) const;



        static void tokenize(const char *path, StrVec *toks)
            {
                TokenMatcher::tokenize('/', path, toks);
            }



        static const StrVec *captures(ActionCtx *actionCtx, apr_ssize_t num);



        static PathReqData *reqData(lua_State *L);



        // rum.path methods
        static int trailingSlash(lua_State *L);

        // tokens array methods
        static int tokensIndex(lua_State *L);
        static int tokensLen(lua_State *L);
        static int tokensTostring(lua_State *L);

        // captures array methods
        static int capturesIndex(lua_State *L);
        static int capturesLen(lua_State *L);
        static int capturesTostring(lua_State *L);

        // mcaptures array methods
        static int mcapturesIndex(lua_State *L);
        static int mcapturesLen(lua_State *L);



    private:
        PtrVec<TokenMatcher *> tokenMatcherVec_;



        // condition name as it appears in the configs
        static const char *condName_;



        PathCondModule(const PathCondModule& from)
            : CondModule(from),
              tokenMatcherVec_(0)
            {
                // this method is private and should not be used
            }



        PathCondModule& operator=(const PathCondModule& /* that */)
            {
                // this method is private and should not be used
                /* CondModule::operator=(that); */
                return *this;
            }
    };
}


#endif // RUM_PATHCONDMODULE_H
