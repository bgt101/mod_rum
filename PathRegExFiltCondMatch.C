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



#include "PathRegExFiltCondMatch.H"



namespace rum
{
    PathRegExFiltCondMatch::PathRegExFiltCondMatch(apr_pool_t *p, bool match__,
                                                   const RegEx::MatchData&
                                                   regExMD,
                                                   bool scrub,
                                                   apr_size_t numClusters)
        : PathFiltCondMatch(p, match__)
    {
        if (match__)
        {
            apr_ssize_t n = regExMD.size() - numClusters;
            apr_ssize_t i;
            for (i = 1; i < n; i++)
            {
                const char *s = regExMD[i];

                if (scrub)
                {
                    apr_ssize_t len = strlen(s);
                    if (*s == '/')
                    {
                        captures()->push_back(s + 1);
                    }
                    else if ((len > 0) && (*(s + len - 1) == '/'))
                    {
                        char buf[len];
                        memcpy(buf, s, len - 1);
                        buf[len - 1] = '\0';
                        captures()->push_back(buf);
                    }
                    else
                    {
                        captures()->push_back(s);
                    }
                }
                else
                {
                    captures()->push_back(s);
                }
            }
        }
    }
}
