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



#ifndef RUM_LUAREQUESTREC_H
#define RUM_LUAREQUESTREC_H


#include "PoolAllocated.H"
#include "Logger.H"
#include "debug.H"



// forward declarations
class lua_State;



namespace rum
{
    class LuaRequestRec : public PoolAllocated
    {
    public:
        LuaRequestRec(apr_pool_t *p, Logger *l)
            : PoolAllocated(p),
              logger_(l)
            { }



        virtual ~LuaRequestRec()
            { }



        Logger *logger()
            {
                return logger_;
            }



        static void *fhmRegKey()
            {
                return &fhmRegKey_useMyAddress_;
            }



        static void luaPrepEnv(lua_State *L, Logger *logger, apr_pool_t *);



        // request methods
        static int requestIsInternalRedirect(lua_State *L);
        static int requestIsSubrequest(lua_State *L);
        static int getRemoteLogname(lua_State *L);
        static int getServerName(lua_State *L);
        static int getServerPort(lua_State *L);
#if RUM_AP22
        static int defaultType(lua_State *L);
#endif
        static int setContentType(lua_State *L);
        static int rputs(lua_State *L);
        static int isInitialReq(lua_State *L);
        static int httpScheme(lua_State *L);



        // request virtual fields
        static int requestOutermostRequest(lua_State *L);



        // request metamethods
        static int requestIndex(lua_State *L);
        static int requestNewindex(lua_State *L);



    private:
        Logger *logger_;
        static char fhmRegKey_useMyAddress_;



        LuaRequestRec(const LuaRequestRec& from)
            : PoolAllocated(from),
              logger_(from.logger_)
            { }



        LuaRequestRec& operator=(const LuaRequestRec& that)
            {
                PoolAllocated::operator=(that);
                logger_ = that.logger_;
                return *this;
            }
    };
}


#endif // RUM_LUAREQUESTREC_H
