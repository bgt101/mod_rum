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



#ifndef RUM_STRVEC_H
#define RUM_STRVEC_H


#include "PoolAllocated.H"
#include "SmplVec.H"
#include "StrBuffer.H"



namespace rum
{

    class StrVec : public PoolAllocated
    {
    public:
        StrVec(apr_pool_t *p, apr_size_t initSize = 10)
            : PoolAllocated(p),
              vec_(pool(), initSize)
            { }



        StrVec(apr_pool_t *p, const StrVec& from)
            : PoolAllocated(p),
              vec_(pool(), from.size())
            {
                push_back(from);
            }



        virtual ~StrVec()
            {
                // the pool from which the strings are allocated is
                // destroyed by ~PoolAllocated()
            }



        StrVec& operator=(const StrVec& that)
            {
                PoolAllocated::operator=(that);

                if (this != &that)
                {
                    clear();
                    push_back(that);
                }

                return *this;
            }



        const char *operator[](apr_size_t i) const
            {
                return vec_[i];
            }



        apr_ssize_t size() const
            {
                return vec_.size();
            }



        void clear()
            {
                vec_.clear();
            }



        void push_back(const char *str)
            {
                // allow storage of NULL pointer
                const char *s = str ? apr_pstrdup(pool(), str) : 0;
                vec_.push_back(s);
            }



        void push_back(const StrVec& from)
            {
                apr_ssize_t sz = from.size();
                apr_ssize_t i;
                for (i = 0; i < sz; i++)
                {
                    vec_.push_back(from[i]);
                }
            }



    private:
        SmplVec<const char *>vec_;


        StrVec(const StrVec& /*from*/)
            : PoolAllocated(0),
              vec_(0)
            { }
    };



    inline StrBuffer& operator<<(StrBuffer& sb, const StrVec& sv)
    {
        sb << "[";

        const apr_size_t sz = sv.size();
        apr_size_t i = 0;

        for (; i < sz; i++)
        {
            if (i > 0)
            {
                sb << ", \"" << sv[i] << "\"";
            }
            else
            {
                sb << "\"" << sv[i] << "\"";
            }
        }

        sb << "]";

        return sb;
    }
}



#endif // RUM_STRVEC_H
