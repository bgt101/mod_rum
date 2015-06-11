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



#ifndef RUM_STRSMPLMAP_H
#define RUM_STRSMPLMAP_H


#include "debug.H"
#include "apr_hash.h"
#include "apr_strings.h"
#include "PoolAllocated.H"
#include "StrBuffer.H"



namespace rum
{


    // StrSmplMap is a map for storing basic types using a string as
    // the key; the type must be no larger than a pointer


    template <typename T>
    class StrSmplMap : public PoolAllocated
    {
    public:
        StrSmplMap(apr_pool_t *p)
            : PoolAllocated(p),
              hash_(0)
            {
                RUM_PTRC_MAP(pool(), "StrSmplMap::StrSmplMap("
                             "apr_pool_t *p), "
                             "this: " << (void *)this);

                hash_ = apr_hash_make(pool());
            }



        virtual ~StrSmplMap()
            {
                RUM_PTRC_MAP(pool(),
                             "StrSmplMap::~StrSmplMap(), "
                             "this: " << (void *)this);
            }



        T& operator[](const char *key)
            {
                RUM_PTRC_MAP(pool(),
                             "StrSmplMap::operator[](const char *key), "
                             "key: " << key
                             << ", this: " << (void *)this);

                Val *vp = static_cast<Val *>
                          (apr_hash_get(hash_, key, APR_HASH_KEY_STRING));
                if (vp == 0)
                {
                    vp = new (pool()) Val(0);
                    apr_hash_set(hash_,
                                 apr_pstrdup(pool(), key),
                                 APR_HASH_KEY_STRING,
                                 vp);
                }
                return (vp->val_);
            }



        void insert(const char *key, T val)
            {
                RUM_PTRC_MAP(pool(),
                             "StrSmplMap::insert(const char *key, T val), "
                             "key: " << key
                             << "val: " << val
                             << ", this: " << (void *)this);

                (*this)[key] = val;
            }



        T find(const char *key)
            {
                RUM_PTRC_MAP(pool(),
                             "StrSmplMap::find(const char *key), "
                             "key: " << key
                             << ", this: " << (void *)this);

                Val *vp = static_cast<Val *>
                          (apr_hash_get(hash_, key, APR_HASH_KEY_STRING));
                return vp ? vp->val_ : 0;
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
            Iterator(apr_pool_t *p, StrSmplMap& map)
                : PoolAllocated(p),
                  initialized_(0),
                  map_(map),
                  hi_(0)
                {
                    RUM_PTRC_MAP(pool(), "StrSmplMap::Iterator::Iterator("
                                 "apr_pool_t *p, "
                                 "const StrSmplMap& map)");
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
            const StrSmplMap& map_;
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
            ConstIterator(apr_pool_t *p, const StrSmplMap& map)
                : Iterator(p, const_cast<StrSmplMap&>(map))
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
        StrSmplMap(const StrSmplMap &from)
            : PoolAllocated(from),
              hash_(0)
            { }



    private:
        apr_hash_t *hash_;



        StrSmplMap(apr_pool_t *p, const StrSmplMap &from)
            : PoolAllocated(p, from),
              hash_(0)
            {
                // this method is private and should not be used
            }



        StrSmplMap& operator=(const StrSmplMap& that)
            {
                // this method is private and should not be used
                return *this;
            }

    };



    template <typename T>
    inline StrBuffer& operator<<(StrBuffer& sb, const StrSmplMap<T>& ssm)
    {
        typename StrSmplMap<T>::ConstIterator it(sb.pool(), ssm);
        apr_ssize_t i = ssm.size();
        while (it.next())
        {
            sb << "\"" << it.key() << "\" => " << *it;
            if (--i > 0)
            {
                sb << ", ";
            }
        }
        return sb;
    }


}


#endif // RUM_STRSMPLMAP_H
