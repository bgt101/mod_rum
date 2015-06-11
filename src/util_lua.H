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



#ifndef RUM_UTIL_LUA_H
#define RUM_UTIL_LUA_H


#include "lua.hpp"



#define RUM_PTR_REGISTRY_NAME_SUFFIX ".RUM.PtrTypeRegKey"

#define RUM_PTR_REGISTRY_NAME(T) (#T RUM_PTR_REGISTRY_NAME_SUFFIX)

#define RUM_PUSH_PTR(T,L,p) rum::pushPtr<T>(L,p,RUM_PTR_REGISTRY_NAME(T))

#define RUM_CHECK_PTR(T,L,n) rum::checkPtr<T>(L,n,RUM_PTR_REGISTRY_NAME(T))

#define RUM_CHECK_NON_NULL_PTR(T,L,n) \
    rum::checkNonNullPtr<T>(L,n,RUM_PTR_REGISTRY_NAME(T))



namespace rum
{
    template <typename T>
    inline void pushPtr(lua_State *L, T *p, const char *tname)
    {
        *static_cast<T **>(lua_newuserdata(L, sizeof(T *))) = p;
        luaL_newmetatable(L, tname);
        lua_setmetatable(L, -2);
    }



    template <typename T>
    inline T *checkPtr(lua_State *L, int narg, const char *tname)
    {
        return *static_cast<T **>(luaL_checkudata(L, narg, tname));
    }



    template <typename T>
    inline T *checkNonNullPtr(lua_State *L, int narg, const char *tname)
    {
        T *p = *static_cast<T **>(luaL_checkudata(L, narg, tname));
        if (!p)
        {
            luaL_error(L, "NULL %s pointer", tname);
        }
        return p;
    }
}


#endif // RUM_UTIL_LUA_H
