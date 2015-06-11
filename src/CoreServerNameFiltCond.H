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



#ifndef RUM_CORESERVERNAMEFILTCOND_H
#define RUM_CORESERVERNAMEFILTCOND_H



#include "FiltCond.H"



namespace rum
{
    class CoreServerNameFiltCond : public FiltCond
    {
    public:
        CoreServerNameFiltCond(apr_pool_t *p, Logger *l,
                               Phases::Phase phase__, apr_ssize_t cmID,
                               const char *serverName);



        CoreServerNameFiltCond(apr_pool_t *p,
                               const CoreServerNameFiltCond& from);



        virtual ~CoreServerNameFiltCond();



        FiltCond *clone(apr_pool_t *p) const
            {
                return new (p) CoreServerNameFiltCond(p, *this);
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
        const char *serverName_;
        const char *asStr_;
        Blob blobID_;


        CoreServerNameFiltCond(const CoreServerNameFiltCond &from)
            : FiltCond(from),
              serverName_(0),
              asStr_(0),
              blobID_(0, 0, 0)
            { }



        CoreServerNameFiltCond& operator=(const CoreServerNameFiltCond& that)
            {
                FiltCond::operator=(that);
                return *this;
            }


        static const char *condName_;
    };

}



#endif // RUM_CORESERVERNAMEFILTCOND_H
