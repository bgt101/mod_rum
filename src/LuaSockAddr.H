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



#ifndef RUM_LUASOCKADDR_H
#define RUM_LUASOCKADDR_H


#include "PoolAllocated.H"
#include "Logger.H"
#include "debug.H"



// forward declarations
class lua_State;



namespace rum
{
    class LuaSockAddr : public PoolAllocated
    {
    public:
        LuaSockAddr(apr_pool_t *p, Logger *l)
            : PoolAllocated(p),
              logger_(l)
            { }



        virtual ~LuaSockAddr()
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



        // apr_sockaddr_t metamethods
        static int sockAddrIndex(lua_State *L);
        static int sockAddrNewindex(lua_State *L);



    private:
        Logger *logger_;
        static char fhmRegKey_useMyAddress_;



        LuaSockAddr(const LuaSockAddr& from)
            : PoolAllocated(from),
              logger_(from.logger_)
            { }



        LuaSockAddr& operator=(const LuaSockAddr& that)
            {
                PoolAllocated::operator=(that);
                logger_ = that.logger_;
                return *this;
            }
    };
}


#endif // RUM_LUASOCKADDR_H
