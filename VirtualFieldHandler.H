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



#ifndef RUM_VIRTUALFIELDHANDLER_H
#define RUM_VIRTUALFIELDHANDLER_H


#include "FieldHandler.H"
#include "lua.hpp"
#include "util_lua.H"



namespace rum
{
    class VirtualFieldHandler : public FieldHandler
    {
    public:
        struct fieldReg
        {
            const char *name;
            lua_CFunction getFunc;
            lua_CFunction setFunc;
        };



        VirtualFieldHandler(apr_pool_t *p, const char *name__,
                            lua_CFunction getFunc__, lua_CFunction setFunc__)
            : FieldHandler(p, name__),
              getFunc_(getFunc__),
              setFunc_(setFunc__)
            { }



        int set(lua_State *L) const
            {
                int rc;
                if (setFunc_)
                {
                    rc = (setFunc_)(L);
                }
                else
                {
                    luaL_error(L, "attemp to set read-only field: %s",
                               name());
                    return 0;
                }
                return rc;
            }



        int get(lua_State *L) const
            {
                int rc;
                if (getFunc_)
                {
                    rc = (getFunc_)(L);
                }
                else
                {
                    luaL_error(L, "attemp to get write-only field: %s",
                               name());
                    return 0;
                }
                return rc;
            }



    private:
        lua_CFunction getFunc_;
        lua_CFunction setFunc_;



        VirtualFieldHandler(const VirtualFieldHandler& from)
            : FieldHandler(0, 0),
              getFunc_(from.getFunc_),
              setFunc_(from.setFunc_)
            { }



        VirtualFieldHandler& operator=(const VirtualFieldHandler& that)
            {
                FieldHandler::operator=(that);
                getFunc_ = that.getFunc_;
                setFunc_ = that.setFunc_;
                return *this;
            }
    };
}



#endif // RUM_VIRTUALFIELDHANDLER_H
