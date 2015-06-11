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



#ifndef RUM_FIELDHANDLER_H
#define RUM_FIELDHANDLER_H


#include "PoolAllocated.H"



// forward declarations
class lua_State;



namespace rum
{
    class FieldHandler : public PoolAllocated
    {
    public:
        FieldHandler(apr_pool_t *p, const char *name__)
            : PoolAllocated(p),
              name_(name__)
            { }



        virtual ~FieldHandler()
            { }



        const char *name() const
            {
                return name_;
            }



        virtual int set(lua_State *L) const = 0;



        virtual int get(lua_State *L) const = 0;



    protected:
        FieldHandler(const FieldHandler& from)
            : PoolAllocated(0),
              name_(from.name_)
            { }



        FieldHandler& operator=(const FieldHandler& that)
            {
                PoolAllocated::operator=(that);
                name_ = that.name_;
                return *this;
            }



    private:
        const char *name_;
    };
}



#endif // RUM_FIELDHANDLER_H
