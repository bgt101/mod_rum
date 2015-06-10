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



#ifndef RUM_PATHREGEXFILTCOND_H
#define RUM_PATHREGEXFILTCOND_H



#include "PathFiltCond.H"
#include "RegEx.H"



namespace rum
{
    class PathRegExFiltCond : public PathFiltCond
    {
    public:
        PathRegExFiltCond(apr_pool_t *p, Logger *l, Phases::Phase phase__,
                          apr_ssize_t cmID,
                          const char *regExStr, bool scrub,
                          apr_size_t numClusters);



        PathRegExFiltCond(apr_pool_t *p, const PathRegExFiltCond& from);



        virtual ~PathRegExFiltCond();



        FiltCond *clone(apr_pool_t *p) const
            {
                return new (p) PathRegExFiltCond(p, *this);
            }



        bool match(ReqCtx *reqCtx, FiltCondMatch **fcMatch) const;



        const char *condName() const
            {
                return condName_;
            }



        const char *asStr() const
            {
                return asStr_;
            }



        const Blob& blobID() const
            {
                return blobID_;
            }



        virtual StrBuffer& write(StrBuffer& sb) const;



    private:
        RegEx regEx_;
        bool scrub_;
        apr_size_t numClusters_;
        const char *asStr_;
        Blob blobID_;


        PathRegExFiltCond(const PathRegExFiltCond &from)
            : PathFiltCond(from),
              regEx_(0, 0, 0),
              scrub_(false),
              numClusters_(0),
              asStr_(0),
              blobID_(0, 0, 0)
            { }



        PathRegExFiltCond& operator=(const PathRegExFiltCond& that)
            {
                PathFiltCond::operator=(that);
                return *this;
            }


        static const char *condName_;
    };

}



#endif // RUM_PATHREGEXFILTCOND_H
