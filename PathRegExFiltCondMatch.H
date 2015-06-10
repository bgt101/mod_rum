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



#ifndef RUM_PATHREGEXFILTCONDMATCH_H
#define RUM_PATHREGEXFILTCONDMATCH_H



#include "debug.H"
#include "PathFiltCondMatch.H"
#include "RegEx.H"



namespace rum
{
    class PathRegExFiltCondMatch : public PathFiltCondMatch
    {
    public:
        PathRegExFiltCondMatch(apr_pool_t *p, bool match__,
                               const RegEx::MatchData& regExMD, bool scrub,
                               apr_size_t numClusters);



        virtual ~PathRegExFiltCondMatch() { }



    private:
        PathRegExFiltCondMatch(const PathRegExFiltCondMatch &/*from*/)
            : PathFiltCondMatch(0, false)
            { }



        PathRegExFiltCondMatch& operator=(const PathRegExFiltCondMatch& that)
            {
                PathFiltCondMatch::operator=(that);
                return *this;
            }
    };

}



#endif // RUM_PATHREGEXFILTCONDMATCH_H
