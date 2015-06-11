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



#include "RequestSubreqFiltCond.H"
#include "FiltCondMatch.H"
#include "ReqCtx.H"



namespace rum
{
    RequestSubreqFiltCond::RequestSubreqFiltCond(apr_pool_t *p, Logger *l,
                                                 Phases::Phase phase__,
                                                 apr_ssize_t cmID,
                                                 bool isSubreq)
        : FiltCond(p, l, phase__, cmID),
          isSubreq_(isSubreq),
          asStr_(apr_pstrcat(pool(), "RequestSubreqFiltCond: ",
                             isSubreq_ ? "true" : "false", NULL)),
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



    RequestSubreqFiltCond::RequestSubreqFiltCond(apr_pool_t *p,
                                                 const
                                                 RequestSubreqFiltCond&
                                                 from)
        : FiltCond(p, from),
          isSubreq_(from.isSubreq_),
          asStr_(apr_pstrdup(pool(), from.asStr_)),
          blobID_(pool(), from.blobID_)
    { }



    RequestSubreqFiltCond::~RequestSubreqFiltCond()
    { }



    bool RequestSubreqFiltCond::match(ReqCtx *reqCtx,
                                      FiltCondMatch **fcMatch) const
    {
        RUM_PTRC_COND(pool(),
                      "RequestSubreqFiltCond::match(ReqCtx *reqCtx, "
                      "FiltCondMatch **fcMatch), "
                      "this: " << (void *)this
                      << ", reqCtx: " << (void *)reqCtx
                      << ", fcMatch: " << (void *)fcMatch);

        bool isMatch = ((reqCtx->req()->main != NULL) == isSubreq_);

        RUM_LOG_COND(reqCtx->logger(), APLOG_DEBUG,
                     "isSubreq: " << isSubreq_
                     << ", isMatch: " << isMatch);

        apr_pool_t *fcPool = reqCtx->filtCondMatches()->pool();
        *fcMatch = new (fcPool) FiltCondMatch(0, isMatch);

        return isMatch;
    }



    StrBuffer& RequestSubreqFiltCond::write(StrBuffer& sb) const
    {
        return sb << asStr();
    }



    const char *RequestSubreqFiltCond::condName_ = "Request";
}
