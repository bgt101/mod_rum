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



#ifndef RUM_SMPLVEC_H
#define RUM_SMPLVEC_H


#include <stdlib.h> // qsort()

#include "debug.H"
#include "apr_tables.h"
#include "PoolAllocated.H"
#include "StrBuffer.H"



namespace rum
{


    // SmplVec is a simple vector intended to store basic types
    // such as int, char, pointers, etc., not objects
    //
    // this is a thin wrapper around APR arrays to allow for simpler
    // syntax and type safety


    template <typename T>
    class SmplVec : public PoolAllocated
    {
    public:
        SmplVec(apr_pool_t *p, apr_size_t initAlloc = 18)
            : PoolAllocated(p),
              arrayHdr_(apr_array_make(pool(), static_cast<int>(initAlloc),
                                       sizeof(T)))
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::SmplVec<T>("
                                 "apr_pool_t *p, apr_size_t initAlloc = 1), "
                                 "this: " << (void *)this
                                 << ", initAlloc: " << initAlloc
                                 << ", pool(): " << pool()
                                 << ", arrayHdr_: " << arrayHdr_);
            }



        SmplVec(apr_pool_t *p, const SmplVec& from)
            : PoolAllocated(p),
              arrayHdr_(apr_array_make(pool(), static_cast<int>(from.size()), sizeof(T)))
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::SmplVec<T>("
                                 "apr_pool_t *p, "
                                 "const SmplVec& from), "
                                 "this: " << (void *)this
                                 << ", from: " << (void *)&from);

                push_back(from);
            }



        virtual ~SmplVec()
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::~SmplVec<T>(), "
                                 "this: " << (void *)this);
            }



        SmplVec& operator=(const SmplVec& that)
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::operator=(), "
                                 "this: " << (void *)this
                                 << ", that: " << (void *)&that);

                PoolAllocated::operator=(that);

                if (this != &that)
                {
                    clear();
                    push_back(that);
                }

                return *this;
            }



        virtual T& operator[](apr_size_t i)
            {
                return ((T *)arrayHdr_->elts)[i];
            }



        virtual const T& operator[](apr_size_t i) const
            {
                return ((T *)arrayHdr_->elts)[i];
            }



        virtual apr_ssize_t size() const
            {
                return arrayHdr_->nelts;
            }



        virtual apr_ssize_t allocSize() const
            {
                return arrayHdr_->nalloc;
            }



        virtual void clear()
            {
                arrayHdr_->nelts = 0;
            }



        virtual void push_back(T i)
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::push_back(T i), "
                                 "this: " << (void *)this
                                 << ", i: " << i
                                 << ", arrayHdr_: " << (void *)arrayHdr_);

                *(T *)apr_array_push(arrayHdr_) = i;
            }



        virtual void push_back(const SmplVec& v)
            {
                apr_array_cat(arrayHdr_, v.arrayHdr_);
            }



        virtual void sort()
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::sort(), "
                                 "this: " << (void *)this);
                qsort((void *)arrayHdr_->elts, arrayHdr_->nelts,
                      sizeof(T), compar);
            }



        virtual bool contains(T v) const
            {
                const apr_size_t sz = size();
                apr_size_t i = 0;
                bool found = false;

                for (; (! found) && (i < sz); i++)
                {
                    if ((*this)[i] == v)
                    {
                        found = true;
                    }
                }

                return found;
            }



        virtual void grow_to(apr_ssize_t n)
            {
                const apr_ssize_t as = allocSize();
                if (as < n)
                {
                    apr_ssize_t nas;
                    const apr_ssize_t das = as * 2;
                    if (das < n)
                    {
                        nas = n;
                    }
                    else
                    {
                        nas = das;
                    }

                    apr_array_header_t *newHdr =
                        apr_array_make(pool(),
                                       static_cast<int>(das),
                                       sizeof(T));
                    apr_array_cat(newHdr, arrayHdr_);
                    arrayHdr_ = newHdr;
                }

                arrayHdr_->nelts = static_cast<int>(n);
             }



        virtual void unique()
            {
                const apr_size_t sz = size();

                if (sz > 1)
                {
                    apr_size_t get = 1;
                    apr_size_t put = 1;

                    for (; get < sz; get++)
                    {
                        if ((*this)[get] != (*this)[put - 1])
                        {
                            if (get != put)
                            {
                                (*this)[put] = (*this)[get];
                            }
                            put++;
                        }
                    }

                    arrayHdr_->nelts = static_cast<int>(put);
                }
            }



        virtual void intersect_with(const SmplVec& other)
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::intersect_with("
                                 "const SmplVec& other), "
                                 "this: " << (void *)this
                                 << ", other: " << (void *)&other);

                const apr_size_t tsz = size();
                const apr_size_t osz = other.size();

                if ((tsz > 0) && (osz == 0))
                {
                    arrayHdr_->nelts = 0;
                }
                else if ((tsz > 0) && (osz > 0))
                {
                    apr_size_t tget = 0;
                    apr_size_t oget = 0;
                    apr_size_t put = 0;

                    while ((tget < tsz) && (oget < osz))
                    {
                        if ((*this)[tget] == other[oget])
                        {
                            if (tget != put)
                            {
                                (*this)[put] = (*this)[tget];
                            }
                            tget++;
                            oget++;
                            put++;
                        }
                        else if ((*this)[tget] < other[oget])
                        {
                            tget++;
                        }
                        else
                        {
                            oget++;
                        }
                    }
                    arrayHdr_->nelts = static_cast<int>(put);
                }
            }



        virtual void union_with(const SmplVec& other)
            {
                RUM_PTRC_VECTOR(pool(), "SmplVec<T>::union_with("
                                 "const SmplVec& other), "
                                 "this: " << (void *)this
                                 << ", other: " << (void *)&other);

                const apr_size_t tsz = size();
                const apr_size_t osz = other.size();

                if (osz > 0)
                {
                    if (tsz == 0)
                    {
                        push_back(other);
                    }
                    else
                    {
                        SmplVec tmpVec(pool(), *this);

                        clear();

                        apr_size_t tget = 0;
                        apr_size_t oget = 0;

                        while ((tget < tsz) && (oget < osz))
                        {
                            if (tmpVec[tget] == other[oget])
                            {
                                push_back(tmpVec[tget]);
                                tget++;
                                oget++;
                            }
                            else if (tmpVec[tget] < other[oget])
                            {
                                push_back(tmpVec[tget]);
                                tget++;
                            }
                            else
                            {
                                push_back(other[oget]);
                                oget++;
                            }
                        }

                        while (tget < tsz)
                        {
                            push_back(tmpVec[tget]);
                            tget++;
                        }

                        while (oget < osz)
                        {
                            push_back(other[oget]);
                            oget++;
                        }
                    }
                }
            }



    protected:
        SmplVec(const SmplVec& from)
            : PoolAllocated(from),
              arrayHdr_(0)
            { }



    private:
        apr_array_header_t *arrayHdr_;



        // comparison function used by sort()
        static int compar(const void *a1, const void *a2)
            {
                const T *t1 = (const T *)a1;
                const T *t2 = (const T *)a2;

                if (*t1 == *t2)
                {
                    return 0;
                }
                else if (*t1 < *t2)
                {
                    return -1;
                }
                else
                {
                    return 1;
                }
            }


        template <typename U>
        friend StrBuffer& operator<<(StrBuffer& sb, const SmplVec<U>& sv);


    };



    template <typename U>
    inline StrBuffer& operator<<(StrBuffer& sb, const SmplVec<U>& sv)
    {
        sb << "[";

        const apr_size_t sz = sv.size();
        apr_size_t i = 0;

        for (; i < sz; i++)
        {
            if (i > 0)
            {
                sb << ", " << sv[i];
            }
            else
            {
                sb << sv[i];
            }
        }

        sb << "]";

        return sb;
    }


}



#endif // RUM_SMPLVEC_H
