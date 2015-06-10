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



#ifndef RUM_STRSTRMAP_H
#define RUM_STRSTRMAP_H


#include "debug.H"
#include "apr_hash.h"
#include "apr_strings.h"
#include "PoolAllocated.H"
#include "TmpPool.H"
#include "StrBuffer.H"



namespace rum
{


    class StrStrMap : public PoolAllocated
    {
    public:
        StrStrMap(apr_pool_t *p)
            : PoolAllocated(p),
              mbrPtr_(new (pool()/*, PoolAllocated::UseSubPools*/)
                      PoolAllocated(0)),
              hash_(apr_hash_make(mbrPtr_->pool()))
            {
                RUM_PTRC_STRSTRMAP(pool(), "StrStrMap::StrStrMap("
                                   "apr_pool_t *p), "
                                   "this: " << (void *)this);
            }



        StrStrMap(apr_pool_t *p, const StrStrMap &from)
            : PoolAllocated(p),
              mbrPtr_(new (pool()/*, PoolAllocated::UseSubPools*/)
                      PoolAllocated(0)),
              hash_(apr_hash_make(mbrPtr_->pool()))
            {
                RUM_PTRC_STRSTRMAP(pool(), "StrStrMap::StrStrMap("
                                   "apr_pool_t *p, "
                                   "const StrStrMap &from), "
                                   "this: " << (void *)this
                                   << ", from: " << (void *)&from);

                insert(from);
            }



        virtual ~StrStrMap()
            {
                RUM_PTRC_STRSTRMAP(pool(), "StrStrMap::~StrStrMap(), "
                                   "this: " << (void *)this);

                delete mbrPtr_;
            }



        StrStrMap& operator=(const StrStrMap& that)
            {
                RUM_PTRC_STRSTRMAP(pool(), "StrStrMap::operator=(), "
                                   "this: " << (void *)this
                                   << ", that: " << (void *)&that);

                PoolAllocated::operator=(that);

                if (this != &that)
                {
                    delete mbrPtr_;
                    mbrPtr_ = new (pool()/*, PoolAllocated::UseSubPools*/)
                              PoolAllocated(0);
                    hash_ = apr_hash_make(mbrPtr_->pool());
                    insert(that);
                }

                return *this;
            }



        void insert(const char *key, const char *val)
            {
                RUM_PTRC_STRSTRMAP(pool(), "StrStrMap::insert("
                                   "const char *key, "
                                   "const char *val), "
                                   "this: " << (void *)this
                                   << ", key: " << key
                                   << ", val: " << val);

                apr_hash_set(hash_,
                             apr_pstrdup(mbrPtr_->pool(), key),
                             APR_HASH_KEY_STRING,
                             apr_pstrdup(mbrPtr_->pool(), val));
            }



        void insert(const StrStrMap& from)
            {
                RUM_PTRC_STRSTRMAP(pool(), "StrStrMap::insert("
                                   "const StrStrMap& from)"
                                   "this: " << (void *)this
                                   << ", &from: " << (void *)&from);

                apr_pool_t *tmpPool = mbrPtr_->pool();

                apr_hash_index_t *hi = apr_hash_first(tmpPool, from.hash_);
                for (; hi; hi = apr_hash_next(hi))
                {
                    const void *vkey;
                    void *vval;
                    apr_hash_this(hi, &vkey, NULL, &vval);

                    insert((const char *)vkey, (const char *)vval);
                }
            }



        const char *find(const char *key) const
            {
                RUM_PTRC_STRSTRMAP(pool(), "StrStrMap::find("
                                   "const char *key), "
                                   "this: " << (void *)this
                                   << ", key: " << key);

                return (const char *) apr_hash_get(hash_, key,
                                                   APR_HASH_KEY_STRING);
            }



        apr_size_t size() const
            {
                return apr_hash_count(hash_);
            }



    private:
        PoolAllocated *mbrPtr_;

        apr_hash_t *hash_;


        StrStrMap(const StrStrMap &from)
            : PoolAllocated(from),
              mbrPtr_(0),
              hash_(0)
            {
                // this method is private and should not be used
            }



        friend StrBuffer& operator<<(StrBuffer& sb, const StrStrMap& ssm);


    };



    inline StrBuffer& operator<<(StrBuffer& sb, const StrStrMap& ssm)
    {
        TmpPool tmpPool(sb.pool());
        apr_size_t i = 0;
        const char *str = 0;

        apr_hash_index_t *hi = apr_hash_first(tmpPool, ssm.hash_);
        for (; hi; hi = apr_hash_next(hi), i++)
        {
            const void *vkey;
            void *vval;
            apr_hash_this(hi, &vkey, NULL, &vval);
            const char *key = (const char *)vkey;
            const char *val = (const char *)vval;

            if (i == 0)
            {
                str = apr_pstrcat(tmpPool,
                                  key, ": ", val,
                                  NULL);
            }
            else
            {
                str = apr_pstrcat(tmpPool,
                                  str, ", ",
                                  key, ": ", val,
                                  NULL);
            }
        }

        if (str == 0)
        {
            str = "EMPTY";
        }

        return sb << str;
    }



}



#endif // RUM_STRSTRMAP_H
