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



#ifndef RUM_MATCHEDIDXS_H
#define RUM_MATCHEDIDXS_H


#include "apr.h"

#include "SizeVec.H"



namespace rum
{

    class MatchedIdxs : public SizeVec
    {
    public:
        MatchedIdxs(apr_pool_t *p, const SizeVec *allIdxs__,
                    bool matchAll__ = false)
            : SizeVec(p),
              allIdxs_(allIdxs__),
              matchAll_(matchAll__)
            { }



        void clear()
            {
                SizeVec::clear();
                matchAll_ = false;
            }



        const SizeVec *allIdxs() const
            {
                return allIdxs_;
            }



        bool matchAll() const
            {
                return matchAll_;
            }



        void setMatchAll(bool matchAll__ = true)
            {
                matchAll_ = matchAll__;
            }



        apr_ssize_t size() const
            {
                if (matchAll())
                {
                    return allIdxs_->size();
                }
                else
                {
                    return SizeVec::size();
                }
            }



        void intersect_with(const MatchedIdxs& other)
            {
                if (!other.matchAll())
                {
                    if (matchAll())
                    {
                        *this = other;
                    }
                    else
                    {
                        SizeVec::intersect_with(other);
                    }
                }
            }



        void intersect_with(const SizeVec& other)
            {
                if (other.size() < allIdxs_->size())
                {
                    if (matchAll())
                    {
                        *this = other;
                    }
                    else
                    {
                        SizeVec::intersect_with(other);
                    }
                }
            }



        void union_with(const MatchedIdxs& other)
            {
                if (!matchAll())
                {
                    if (other.matchAll())
                    {
                        matchAll_ = true;
                    }
                    else
                    {
                        SizeVec::union_with(other);
                        if (SizeVec::size() == allIdxs()->size())
                        {
                            matchAll_ = true;
                        }
                    }
                }
            }



        void union_with(const SizeVec& other)
            {
                if (!matchAll())
                {
                    if (other.size() == allIdxs()->size())
                    {
                        matchAll_ = true;
                    }
                    else
                    {
                        SizeVec::union_with(other);
                        if (SizeVec::size() == allIdxs()->size())
                        {
                            matchAll_ = true;
                        }
                    }
                }
            }



        apr_ssize_t at(apr_size_t i) const
            {
                return (*this)[i];
            }



        const apr_ssize_t& operator[](apr_size_t i) const
            {
                if (matchAll())
                {
                    return (*allIdxs_)[i];
                }
                else
                {
                    return SizeVec::operator[](i);
                }
            }



        apr_ssize_t& operator[](apr_size_t i)
            {
                if (matchAll() && (SizeVec::size() < allIdxs_->size()))
                {
                    *static_cast<SizeVec *>(this) = *allIdxs_;
                }
                return SizeVec::operator[](i);
            }



        MatchedIdxs& operator=(const MatchedIdxs& that)
            {
                SizeVec::operator=(that);

                if (this != &that)
                {
                    allIdxs_ = that.allIdxs_;
                    matchAll_ = that.matchAll_;
                }

                return *this;
            }



        MatchedIdxs& operator=(const SizeVec& that)
            {
                SizeVec::operator=(that);

                if (size() == allIdxs_->size())
                {
                    matchAll_ = true;
                }
                else
                {
                    matchAll_ = false;
                }

                return *this;
            }



    protected:
        MatchedIdxs(const MatchedIdxs& from)
            : SizeVec(from),
              allIdxs_(0),
              matchAll_(false)
            { }



    private:
        const SizeVec *allIdxs_;
        bool matchAll_;
    };

}



#endif // RUM_MATCHEDIDXS_H
