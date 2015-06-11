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



#ifndef RUM_PTRVEC_H
#define RUM_PTRVEC_H


#include "debug.H"
#include "apr_tables.h"

#include "PoolAllocated.H"
#include "StrBuffer.H"



namespace rum
{

    template <typename T>
    class PtrVec : public PoolAllocated
    {
    private:
        PtrVec(apr_pool_t *) { }

        virtual ~PtrVec() { }

        PtrVec(const PtrVec& from) { }
    };



    ///
    /// PtrVec is a vector for storing pointers to objects. The
    /// objects whose pointers are stored in the vector are owned by
    /// the vector, and delete is called on them when the vector is
    /// destroyed.
    ///
    /// This is a thin wrapper around APR arrays to allow for simpler
    /// syntax and type safety.
    ///
    template <typename T>
    class PtrVec<T *> : public PoolAllocated
    {
    public:

        ///
        /// Construct a PtrVec object, preallocating space for the
        /// optionally specified number of elements.
        ///
        /// @param p the pool to allocate from
        ///
        /// @param initAlloc number of elements for which space is
        /// initially allocated during the construction of the vector
        ///
        /// @note The size of the vector right after construction is
        /// always 0.
        ///
        PtrVec(apr_pool_t *p, apr_size_t initAlloc = 18)
            : PoolAllocated(p),
              arrayHdr_(0)
            {
                RUM_PTRC_VECTOR(pool(), "PtrVec<T *>::PtrVec<T *>("
                                "apr_pool_t *p), "
                                "this: " << (void *)this);

                arrayHdr_ = apr_array_make(pool(), static_cast<int>(initAlloc),
                                           sizeof(T *));
            }



        ///
        /// Destroy a PtrVec object, calling delete on pointers
        /// stored within it.
        ///
        virtual ~PtrVec()
            {
                RUM_PTRC_VECTOR(pool(), "PtrVec<T *>::~PtrVec<T *>(), "
                                "this: " << (void *)this);

                // delete objects being pointed to
                apr_size_t i;
                const apr_size_t sz = size();
                for (i = 0; i < sz; i++)
                {
                    RUM_PTRC_VECTOR(pool(), "PtrVec<T *>::~PtrVec<T *>(), "
                                    "this: " << (void *)this
                                    << ", deleting: "
                                    << (void *)((*this)[i]));

                    delete (*this)[i];
                }
            }



        ///
        /// Array access operator for LHS assignment.
        ///
        /// @param i index into the vector
        ///
        /// @return pointer to object at specified index
        ///
        /// @note No bound checking is done.
        ///
        T *&operator[](apr_size_t i)
            {
                return ((T **)arrayHdr_->elts)[i];
            }



        ///
        /// Const array access operator.
        ///
        /// @param i index into the vector
        ///
        /// @return const pointer to object at specified index
        ///
        /// @note No bound checking is done.
        ///
        const T *operator[](apr_size_t i) const
            {
                return ((T **)arrayHdr_->elts)[i];
            }



        ///
        /// Return the size of the vector.
        ///
        /// @return size of the vector
        ///
        apr_ssize_t size() const
            {
                return arrayHdr_->nelts;
            }



        ///
        /// Return the allocated size of the vector, which is equal to
        /// or greater than the actual size.
        ///
        /// @return allocated size of the vector
        ///
        apr_ssize_t allocSize() const
            {
                return arrayHdr_->nalloc;
            }



        ///
        /// Append a pointer to the end of the vector.
        ///
        /// @param i pointer to be appended
        ///
        void push_back(const T *i)
            {
                *(const T **)apr_array_push(arrayHdr_) = i;
            }



        ///
        /// Increase the size of the vector to the specified value if
        /// the current size is smaller.  A null pointer is stored in
        /// the newly created elements.
        ///
        /// @param n desired new size of the vector
        ///
        void grow_to(apr_ssize_t n, bool valid = true)
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

                if (!valid)
                {
                    memset(arrayHdr_->elts + (sizeof(T *) * size()),
                           -1,
                           (sizeof(T *) * (n - size())));
                }
                arrayHdr_->nelts = static_cast<int>(n);
            }



    protected:
        PtrVec(const PtrVec &from)
            : PoolAllocated(from),
              arrayHdr_(0)
            { }



    private:
        PtrVec(apr_pool_t *p, const PtrVec &from)
            : PoolAllocated(p, from),
              arrayHdr_(0)
            {
                RUM_PTRC_VECTOR(pool(), "PtrVec<T *>::PtrVec<T *>("
                                "apr_pool_t *p, "
                                "const PtrVec &from), "
                                "this: " << (void *)this
                                << ", from: " << (void *)&from);

                arrayHdr_ = apr_array_make(pool(), from.size(),
                                           sizeof(T *));
                push_back(from);
            }



        PtrVec& operator=(const PtrVec& that)
            {
                RUM_PTRC_VECTOR(pool(), "PtrVec<T *>::operator=(), "
                                "this: " << (void *)this
                                << ", that: " << (void *)&that);

                PoolAllocated::operator=(that);

                if (this != &that)
                {
                    arrayHdr_->nelts = 0;
                    push_back(that);
                }

                return *this;
            }



        void push_back(const PtrVec &v)
            {
                apr_array_cat(arrayHdr_, v.arrayHdr_);
            }



        apr_array_header_t *arrayHdr_;


    };



    ///
    /// Output the vector into an StrBuffer, invoking operator<<() for
    /// each object whose address is stored in the vector.  New lines
    /// and indentation will be inserted at the appropriate places.
    ///
    /// @param sb the StrBuffer object reference
    ///
    /// @param pv the vector
    ///
    /// @note This operator is for writing into an StrBuffer object,
    /// not a stream.
    ///
    template <typename T>
    inline StrBuffer& operator<<(StrBuffer& sb, const PtrVec<T *>& pv)
    {
        const apr_ssize_t sz = pv.size();
        const apr_ssize_t szmo = sz - 1;
        apr_ssize_t i = 0;

        for (; i < sz; i++)
        {
            const T *t = pv[i];

            sb << i << " => " << nl << indent;

            if (t)
            {
                sb << *t;
            }
            else
            {
                sb << "(nil)";
            }
            sb << outdent;

            if (i < szmo)
            {
                sb << nl;
            }
        }

        return sb;
    }

}


#endif // RUM_PTRVEC_H
