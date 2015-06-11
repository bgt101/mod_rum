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



#ifndef RUM_FILTCOND_H
#define RUM_FILTCOND_H



#include "string.h"
#include "PoolAllocated.H"
#include "Logger.H"
#include "Phases.H"
#include "debug.H"
#include "Blob.H"



// forward declarations
class request_rec;



namespace rum
{
    // forwrard declarations
    class ReqCtx;
    class FiltCondMatch;


    class FiltCond : public PoolAllocatedLight
    {
    public:
        FiltCond(apr_pool_t *p, Logger *l, Phases::Phase phase__,
                 apr_ssize_t cmID)
            : PoolAllocatedLight(p),
              logger_(l),
              phase_(phase__),
              id_(cmID)
            { }



        FiltCond(apr_pool_t *p, const FiltCond& from)
            : PoolAllocatedLight(p, from),
              logger_(from.logger_),
              phase_(from.phase_),
              id_(from.id_)
            { }



        virtual FiltCond *clone(apr_pool_t *p) const = 0;



        virtual ~FiltCond() { }



        FiltCond& operator=(const FiltCond& that)
            {
                PoolAllocatedLight::operator=(that);
                logger_ = that.logger_;
                phase_ = that.phase_;
                id_ = that.id_;
                return *this;
            }



        virtual bool operator==(const FiltCond& other) const
            {
                RUM_PTRC_COND(pool(),
                              "FiltCond::operator==("
                              "const FiltCond& other)"
                              ", &other: " << (void *)&other);

                return ((phase_ == other.phase_) &&
                        (id_ == other.id_) &&
                        (strcmp(asStr(), other.asStr()) == 0));
            }



        virtual bool operator!=(const FiltCond& other) const
            {
                return !(*this == other);
            }



        virtual const char *condName() const = 0;



        virtual const char *asStr() const = 0;



        virtual const Blob& blobID() const = 0;



        virtual bool match(ReqCtx *reqCtx,
                           FiltCondMatch **fcMatch) const = 0;



        virtual StrBuffer& write(StrBuffer& sb) const = 0;



        apr_ssize_t id() const
            {
                return id_;
            }



        Phases::Phase phase() const
            {
                return phase_;
            }



    protected:
        FiltCond(const FiltCond &from)
            : PoolAllocatedLight(from),
              logger_(0),
              phase_(Phases::InvalidPhase),
              id_(0)
            { }



    private:
        Logger *logger_;
        Phases::Phase phase_;
        apr_ssize_t id_;


    };



    inline StrBuffer& operator<<(StrBuffer& sb, const FiltCond& fc)
    {
        fc.write(sb);
        return sb << nl << "phase: " << Phases::enum2str(fc.phase());
    }

}



#endif // RUM_FILTCOND_H
