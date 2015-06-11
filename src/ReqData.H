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



#ifndef RUM_CONDMODULEREQDATA_H
#define RUM_CONDMODULEREQDATA_H


#include "PoolAllocated.H"



namespace rum
{
    // forward declarations
    class StrBuffer;
    class ReqCtx;



    ///
    /// ReqData is the base class for classes that store
    /// various condition module specific data from a request.
    ///
    class ReqData : public PoolAllocated
    {
    public:
        ReqData(apr_pool_t *p, ReqCtx *reqCtx__)
            : PoolAllocated(p),
              reqCtx_(reqCtx__)
            { }



        ReqCtx *reqCtx()
            {
                return reqCtx_;
            }



        virtual void reset() = 0;



        virtual StrBuffer& write(StrBuffer& sb) const = 0;



    protected:
        ReqData(const ReqData& from)
            : PoolAllocated(from),
              reqCtx_(from.reqCtx_)
            { }



        ReqData& operator=(const ReqData& that)
            {
                PoolAllocated::operator=(that);
                reqCtx_ = that.reqCtx_;
                return *this;
            }



    private:
        ReqCtx *reqCtx_;
    };



    inline StrBuffer& operator<<(StrBuffer& sb, const ReqData& cmrd)
    {
        return cmrd.write(sb);
    }

}


#endif // RUM_CONDMODULEREQDATA_H
