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



#ifndef RUM_ACTION_H
#define RUM_ACTION_H


#include "apr_pools.h"
#include "PoolAllocated.H"
#include "debug.H"
#include "Logger.H"
#include "Phases.H"



// defined in httpd.h
class request_rec;



namespace rum
{
    // forward declarations
    class ActionCtx;



    class Action : public PoolAllocatedLight
    {
    public:
        Action(apr_pool_t *p, Logger *l, Phases::Phase phase__)
            : PoolAllocatedLight(p),
              logger_(l),
              phase_(phase__)
            {
                RUM_PTRC_ACTION(pool(), "Action::Action(), "
                                "this: " << (void *)this);
            }



        virtual ~Action()
            {
                RUM_PTRC_ACTION(pool(), "Action::~Action(), "
                                "this: " << (void *)this);
            }



        virtual apr_status_t run(ActionCtx *) const = 0;



        virtual const char *actionName() const = 0;



        virtual StrBuffer& write(StrBuffer& sb) const = 0;



        Logger *logger()
            {
                return logger_;
            }



        Phases::Phase phase() const
            {
                return phase_;
            }



    protected:
        Action(const Action& from)
            : PoolAllocatedLight(from),
              logger_(from.logger_),
              phase_(from.phase_)
            { }


        Action& operator=(const Action& that)
            {
                PoolAllocatedLight::operator=(that);
                logger_ = that.logger_;
                phase_ = that.phase_;
                return *this;
            }



    private:
        Logger *logger_;
        Phases::Phase phase_;


    };



    inline StrBuffer& operator<<(StrBuffer& sb, const Action& action)
    {
        action.write(sb);
        return sb << nl << "phase: " << Phases::enum2str(action.phase());
    }

}


#endif // RUM_ACTION_H
