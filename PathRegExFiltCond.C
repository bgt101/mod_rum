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



#include "PathRegExFiltCond.H"
#include "PathRegExFiltCondMatch.H"
#include "ReqCtx.H"



namespace rum
{
    PathRegExFiltCond::PathRegExFiltCond(apr_pool_t *p, Logger* l,
                                         Phases::Phase phase__,
                                         apr_ssize_t cmID, const char *pattern,
                                         bool scrub, apr_size_t numClusters)
        : PathFiltCond(p, l, phase__, cmID, true),
          regEx_(pool(), l, pattern),
          scrub_(scrub),
          numClusters_(numClusters),
          asStr_(apr_pstrcat(pool(), "PathRegExFiltCond: \"",
                             pattern, "\"", NULL)),
          blobID_(pool(), 0, 0, false)
    {
        apr_ssize_t bSize = sizeof(phase__) +
                            sizeof(cmID) +
                            strlen(asStr_);
        void *bData = apr_palloc(blobID_.pool(), bSize);
        char *cp = static_cast<char *>(bData);
        memcpy(cp, &phase__, sizeof(phase__));
        cp += sizeof(phase__);
        memcpy(cp, &cmID, sizeof(cmID));
        cp += sizeof(cmID);
        memcpy(cp, asStr_, strlen(asStr_));
        blobID_.set(bData, bSize, false);
    }



    PathRegExFiltCond::PathRegExFiltCond(apr_pool_t *p,
                                         const PathRegExFiltCond& from)
        : PathFiltCond(p, from),
          regEx_(pool(), from.regEx_),
          scrub_(from.scrub_),
          numClusters_(from.numClusters_),
          asStr_(apr_pstrdup(pool(), from.asStr_)),
          blobID_(pool(), from.blobID_)
    { }



    PathRegExFiltCond::~PathRegExFiltCond()
    { }



    bool PathRegExFiltCond::match(ReqCtx *reqCtx,
                                  FiltCondMatch **fcMatch) const
    {
        RUM_PTRC_COND(pool(),
                      "PathRegExFiltCond::match(ReqCtx *reqCtx, "
                      "FiltCondMatch **fcMatch), "
                      "this: " << (void *)this
                      << ", reqCtx: " << (void *)reqCtx
                      << ", fcMatch: " << (void *)fcMatch);

        const RegEx::MatchData *md = regEx_.match(reqCtx->req()->uri,
                                                  reqCtx->pool(),
                                                  reqCtx->logger());
        RUM_LOG_COND(reqCtx->logger(), APLOG_DEBUG,
                     "pattern: " << regEx_.pattern()
                     << ", uri: " << reqCtx->req()->uri
                     << ", isMatch: " << (md != 0));

        bool isMatch = md ? true : false;
        apr_pool_t *fcPool = reqCtx->filtCondMatches()->pool();
        *fcMatch = new (fcPool) PathRegExFiltCondMatch(0, isMatch, *md,
                                                       scrub_,
                                                       numClusters_);

        return isMatch;
    }



    StrBuffer& PathRegExFiltCond::write(StrBuffer& sb) const
    {
        return sb << asStr();
    }



    const char *PathRegExFiltCond::condName_ = "Path";
}
