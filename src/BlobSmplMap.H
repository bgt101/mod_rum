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



#ifndef RUM_BLOBSMPLMAP_H
#define RUM_BLOBSMPLMAP_H


#include "debug.H"
#include "apr_hash.h"
#include "apr_strings.h"
#include "PoolAllocated.H"
#include "StrBuffer.H"
#include "Blob.H"



namespace rum
{


    // BlobSmplMap is a map for storing basic types using a blob as
    // the key


    template <typename T>
    class BlobSmplMap : public PoolAllocated
    {
    public:
        BlobSmplMap(apr_pool_t *p)
            : PoolAllocated(p),
              hash_(0)
            {
                RUM_PTRC_MAP(pool(), "BlobSmplMap::BlobSmplMap("
                             "apr_pool_t *p), "
                             "this: " << (void *)this);

                hash_ = apr_hash_make(pool());
            }



        virtual ~BlobSmplMap()
            {
                RUM_PTRC_MAP(pool(),
                             "BlobSmplMap::~BlobSmplMap(), "
                             "this: " << (void *)this);
            }



        T& operator[](const Blob& key)
            {
                RUM_PTRC_MAP(pool(),
                             "BlobSmplMap::operator[](const Blob& key), "
                             "key: " << &key
                             << ", this: " << (void *)this);

                Val *vp = static_cast<Val *>
                          (apr_hash_get(hash_, key.data(), key.size()));
                if (vp == 0)
                {
                    void *nKeyData = apr_palloc(pool(), key.size());
                    memcpy(nKeyData, key.data(), key.size());

                    vp = new (pool()) Val(0);
                    apr_hash_set(hash_,
                                 nKeyData,
                                 key.size(),
                                 vp);
                }
                return (vp->val_);
            }



        void insert(const Blob& key, T val)
            {
                RUM_PTRC_MAP(pool(),
                             "BlobSmplMap::insert(const Blob& key, T val), "
                             "key: " << &key
                             << "val: " << val
                             << ", this: " << (void *)this);

                (*this)[key] = val;
            }



        T *find(const Blob& key)
            {
                RUM_PTRC_MAP(pool(),
                             "BlobSmplMap::find(const Blob& key), "
                             "key: " << &key
                             << ", this: " << (void *)this);

                Val *vp = static_cast<Val *>
                          (apr_hash_get(hash_, key.data(), key.size()));
                return vp ? &(vp->val_) : 0;
            }



        apr_size_t size() const
            {
                return apr_hash_count(hash_);
            }



        class Val : public PoolAllocated
        {
        public:
            Val(apr_pool_t *p)
                : PoolAllocated(p),
                  val_(0)
                { }

            T val_;

        private:
            Val(const Val& from)
                : PoolAllocated(0),
                  val_(from.val_)
                { }

            Val& operator=(const Val& that)
                {
                    PoolAllocated::operator=(that);
                    val_ = that.val_;
                    return *this;
                }
        };



        class Iterator : public PoolAllocated
        {
        public:
            Iterator(apr_pool_t *p, BlobSmplMap& map)
                : PoolAllocated(p),
                  initialized_(0),
                  map_(map),
                  hi_(0)
                {
                    RUM_PTRC_MAP(pool(), "BlobSmplMap::Iterator::Iterator("
                                 "apr_pool_t *p, "
                                 "const BlobSmplMap& map)");
                }



            void reset()
                {
                    initialized_ = 0;
                }



            bool next()
                {
                    if (! initialized_)
                    {
                        hi_ = apr_hash_first(pool(), map_.hash_);
                        initialized_ = true;
                    }
                    else
                    {
                        hi_ = apr_hash_next(hi_);
                    }

                    return (hi_ != 0);
                }



            const void *key() const
                {
                    const void *vKey;
                    apr_hash_this(hi_, &vKey, NULL, NULL);
                    return vKey;
                }



            T& val()
                {
                    void *vVal;
                    apr_hash_this(hi_, NULL, NULL, &vVal);
                    return (static_cast<Val *>(vVal))->val_;
                }



            T& operator*()
                {
                    return val();
                }



        private:
            bool initialized_;
            const BlobSmplMap& map_;
            apr_hash_index_t *hi_;

            Iterator(const Iterator&)
                : PoolAllocated(0),
                  initialized_(0),
                  map_(0),
                  hi_(0)
                { }

            Iterator& operator=(const Iterator&) { }
        };



        class ConstIterator : public Iterator
        {
        public:
            ConstIterator(apr_pool_t *p, const BlobSmplMap& map)
                : Iterator(p, const_cast<BlobSmplMap&>(map))
                { }



            const T& val()
                {
                    return Iterator::val();
                }



            const T& operator*()
                {
                    return Iterator::operator*();
                }

        };



    protected:
        BlobSmplMap(const BlobSmplMap &from)
            : PoolAllocated(from),
              hash_(0)
            { }



    private:
        apr_hash_t *hash_;



        BlobSmplMap(apr_pool_t *p, const BlobSmplMap &from)
            : PoolAllocated(p, from),
              hash_(0)
            {
                // this method is private and should not be used
            }



        BlobSmplMap& operator=(const BlobSmplMap& that)
            {
                // this method is private and should not be used
                return *this;
            }

    };



    template <typename T>
    inline StrBuffer& operator<<(StrBuffer& sb, const BlobSmplMap<T>& ssm)
    {
        typename BlobSmplMap<T>::ConstIterator it(sb.pool(), ssm);
        apr_ssize_t i = ssm.size();
        while (it.next())
        {
            sb << it.key() << " => " << *it;
            if (--i > 0)
            {
                sb << ", ";
            }
        }
        return sb;
    }


}


#endif // RUM_BLOBSMPLMAP_H
