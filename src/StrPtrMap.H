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



#ifndef RUM_STRPTRMAP_H
#define RUM_STRPTRMAP_H


#include "debug.H"
#include "apr_hash.h"
#include "apr_strings.h"
#include "PoolAllocated.H"



namespace rum
{


    // StrPtrMap is a map for storing pointers to objects using a
    // string as the key; the objects whose pointers are stored in the
    // map are owned by the map, and delete is called on them when the
    // map is destroyed


    template <typename T>
    class StrPtrMap : public PoolAllocated
    {
    private:
        StrPtrMap(apr_pool_t *) { }

        virtual ~StrPtrMap() { }

        StrPtrMap(const StrPtrMap& /*from*/) { }
    };



    template <typename T>
    class StrPtrMap<T *> : public PoolAllocated
    {
    public:
        StrPtrMap(apr_pool_t *p)
            : PoolAllocated(p),
              hash_(apr_hash_make(pool()))
            {
                RUM_PTRC_MAP(pool(), "StrPtrMap::StrPtrMap("
                             "apr_pool_t *p), "
                             "this: " << (void *)this);
            }



        virtual ~StrPtrMap()
            {
                RUM_PTRC_MAP(pool(),
                             "StrPtrMap::~StrPtrMap(), "
                             "this: " << (void *)this);

                clear();
            }



        void clear()
            {
                RUM_PTRC_MAP(pool(),
                             "StrPtrMap::clear(), "
                             "this: " << (void *)this);

                apr_hash_index_t *hi = apr_hash_first(pool(), hash_);

                for (; hi; hi = apr_hash_next(hi))
                {
                    const void *key;
                    apr_ssize_t klen;
                    void *val;

                    apr_hash_this(hi, &key, &klen, &val);
                    apr_hash_set(hash_, key, klen, NULL);
                    delete (T *)val;
                }
            }



        void insert(const char *key, const T *objPtr)
            {
                RUM_PTRC_MAP(pool(), "StrPtrMap::insert("
                             "const char *key, "
                             "const T *objPtr), "
                             "this: " << (void *)this
                             << ", key: " << key
                             << ", objPtr: "
                             << (void *)objPtr);

                apr_hash_set(hash_,
                             apr_pstrdup(pool(), key),
                             APR_HASH_KEY_STRING,
                             objPtr);
            }



        T *find(const char *key)
            {
                RUM_PTRC_MAP(pool(), "StrPtrMap::find("
                             "const char *key), "
                             "this: " << (void *)this
                             << ", key: " << key);

                return static_cast<T *>
                    (apr_hash_get(hash_, key, APR_HASH_KEY_STRING));
            }



        const T *find(const char *key) const
            {
                RUM_PTRC_MAP(pool(), "StrPtrMap::find("
                             "const char *key) const, "
                             "this: " << (void *)this
                             << ", key: " << key);

                return static_cast<const T *>
                    (apr_hash_get(hash_, key, APR_HASH_KEY_STRING));
            }



        apr_size_t size() const
            {
                return apr_hash_count(hash_);
            }



        class Iterator : public PoolAllocated
        {
        public:
            Iterator(apr_pool_t *p, StrPtrMap& map)
                : PoolAllocated(p),
                  initialized_(0),
                  map_(map),
                  hi_(0)
                {
                    RUM_PTRC_MAP(pool(), "StrPtrMap::Iterator::Iterator("
                                 "apr_pool_t *p, "
                                 "const StrPtrMap& map)");
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



            const char *key() const
                {
                    const void *vKey;
                    apr_hash_this(hi_, &vKey, NULL, NULL);
                    return static_cast<const char *>(vKey);
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
            const StrPtrMap& map_;
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
            ConstIterator(apr_pool_t *p, const StrPtrMap& map)
                : Iterator(p, const_cast<StrPtrMap&>(map))
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
        StrPtrMap(const StrPtrMap &from)
            : PoolAllocated(from),
              hash_(0)
            { }



    private:
        apr_hash_t *hash_;



        StrPtrMap(apr_pool_t *p, const StrPtrMap &from)
            : PoolAllocated(p, from),
              hash_(0)
            {
                RUM_PTRC_MAP(pool(), "StrPtrMap::StrPtrMap("
                             "apr_pool_t *p, "
                             "const StrPtrMap &from), "
                             "this: " << (void *)this
                             << ", from: " << (void *)&from);

                // this method is private and should not be used
                hash_ = apr_hash_copy(pool(), from.hash_);
            }



        StrPtrMap& operator=(const StrPtrMap& that)
            {
                // this method is private and should not be used

                RUM_PTRC_MAP(pool(), "StrPtrMap::operator=(), "
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



    template <typename T>
    inline StrBuffer& operator<<(StrBuffer& sb, const StrPtrMap<T *>& spm)
    {
        typename StrPtrMap<T *>::ConstIterator it(sb.pool(), spm);
        apr_ssize_t i = spm.size();
        while (it.next())
        {
            sb << "\"" << it.key() << "\" =>" << nl << indent
               << *it << outdent;
            if (--i > 0)
            {
                sb << nl;
            }
        }
        return sb;
    }

}


#endif // RUM_STRPTRMAP_H
