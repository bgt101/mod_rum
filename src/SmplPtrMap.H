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



#ifndef RUM_SMPLPTRMAP_H
#define RUM_SMPLPTRMAP_H


#include "debug.H"
#include "apr_hash.h"
#include "PoolAllocated.H"



namespace rum
{


    // SmplPtrMap is a map for storing pointers to objects using a
    // simple type as the key; the objects whose pointers are stored
    // in the map are owned by the map, and delete is called on them
    // when the map is destroyed by default


    template <typename S, typename T>
    class SmplPtrMap : public PoolAllocated
    {
    private:
        SmplPtrMap(apr_pool_t *) { }

        virtual ~SmplPtrMap() { }

        SmplPtrMap(const SmplPtrMap& /*from*/) { }
    };



    template <typename S, typename T>
    class SmplPtrMap<S, T *> : public PoolAllocated
    {
    public:
        SmplPtrMap(apr_pool_t *p)
            : PoolAllocated(p),
              hash_(apr_hash_make(pool()))
            {
                RUM_PTRC_MAP(pool(), "SmplPtrMap::SmplPtrMap("
                             "apr_pool_t *p), "
                             "this: " << (void *)this);
            }



        virtual ~SmplPtrMap()
            {
                RUM_PTRC_MAP(pool(),
                             "SmplPtrMap::~SmplPtrMap(), "
                             "this: " << (void *)this);

                apr_hash_index_t *hi = apr_hash_first(pool(), hash_);

                for (; hi; hi = apr_hash_next(hi))
                {
                    void *val;

                    apr_hash_this(hi, NULL, NULL, &val);
                    delete static_cast<T *>(val);
                }
            }



        void insert(S key, const T *objPtr)
            {
                RUM_PTRC_MAP(pool(), "SmplPtrMap::insert("
                             "S key, "
                             "const T *objPtr), "
                             "this: " << (void *)this
                             << ", key: " << key
                             << ", objPtr: "
                             << (void *)objPtr);

                void *pKey = apr_palloc(pool(), sizeof(S));
                memcpy(pKey, &key, sizeof(S));

                apr_hash_set(hash_,
                             pKey,
                             sizeof(S),
                             objPtr);
            }



        T *find(S key)
            {
                return static_cast<T *>(apr_hash_get(hash_, &key, sizeof(S)));
            }



        const T *find(S key) const
            {
                return static_cast<T *>(apr_hash_get(hash_, &key, sizeof(S)));
            }



        apr_size_t size() const
            {
                return apr_hash_count(hash_);
            }



        class Iterator : public PoolAllocated
        {
        public:
            Iterator(apr_pool_t *p, SmplPtrMap& map)
                : PoolAllocated(p),
                  initialized_(0),
                  map_(map),
                  hi_(0)
                {
                    RUM_PTRC_MAP(pool(), "SmplPtrMap::Iterator::Iterator("
                                 "apr_pool_t *p, "
                                 "const SmplPtrMap& map)");
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



            S key() const
                {
                    const void *vKey;
                    apr_hash_this(hi_, &vKey, NULL, NULL);
                    return *static_cast<const S *>(vKey);
                }



            T *val()
                {
                    void *vVal;
                    apr_hash_this(hi_, NULL, NULL, &vVal);
                    return static_cast<T *>(vVal);
                }



            T& operator*()
                {
                    return *val();
                }



        private:
            bool initialized_;
            const SmplPtrMap& map_;
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
            ConstIterator(apr_pool_t *p, const SmplPtrMap& map)
                : Iterator(p, const_cast<SmplPtrMap&>(map))
                { }



            const T *val()
                {
                    return Iterator::val();
                }



            const T& operator*()
                {
                    return Iterator::operator*();
                }

        };



    protected:
        SmplPtrMap(const SmplPtrMap &from)
            : PoolAllocated(from),
              hash_(0)
            { }



    private:
        apr_hash_t *hash_;



        SmplPtrMap(apr_pool_t *p, const SmplPtrMap &from)
            : PoolAllocated(p, from),
              hash_(0)
            {
                RUM_PTRC_MAP(pool(), "SmplPtrMap::SmplPtrMap("
                             "apr_pool_t *p, "
                             "const SmplPtrMap &from), "
                             "this: " << (void *)this
                             << ", from: " << (void *)&from);

                // this method is private and should not be used
                hash_ = apr_hash_copy(pool(), from.hash_);
            }



        SmplPtrMap& operator=(const SmplPtrMap& that)
            {
                // this method is private and should not be used

                RUM_PTRC_MAP(pool(), "SmplPtrMap::operator=(), "
                             "this: " << (void *)this
                             << ", that: " << (void *)&that);

                PoolAllocated::operator=(that);

                if (this != &that)
                {
                    // this only makes a shallow copy but we really
                    // want a deep copy
                    hash_ = apr_hash_copy(pool(), that.hash_);
                }

                return *this;
            }

    };



    template <typename S, typename T>
    inline StrBuffer& operator<<(StrBuffer& sb, const SmplPtrMap<S, T *>& spm)
    {
        typename SmplPtrMap<S, T *>::ConstIterator it(sb.pool(), spm);
        apr_ssize_t i = spm.size();
        while (it.next())
        {
            sb << it.key() << " =>" << nl << indent
               << *it << outdent;
            if (--i > 0)
            {
                sb << nl;
            }
        }
        return sb;
    }

}


#endif // RUM_SMPLPTRMAP_H
