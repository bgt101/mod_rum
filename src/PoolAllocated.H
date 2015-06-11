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



#ifndef RUM_POOLALLOCATED_H
#define RUM_POOLALLOCATED_H


#include <new>
#include <assert.h>
#include "apr_pools.h"



namespace rum
{

    // classes derived from PoolAllocated will have their memory
    // allocated from the APR pool which is supplied as an argument to
    // the new operator

    class PoolAllocated
    {
    public:



        enum SubPoolUsage
        {
            NoSubPools,
            UseSubPools
        };



        PoolAllocated(apr_pool_t *p);



        PoolAllocated(apr_pool_t *p, const PoolAllocated& /*from*/);



        virtual ~PoolAllocated();



        void destroyWithPool(apr_pool_t *p);



        void destroyWithPool()
            {
                destroyWithPool(pool());
            }



        void* operator new(size_t s,
                           apr_pool_t *p,
                           SubPoolUsage u = NoSubPools);



        void operator delete(void* op);



        PoolAllocated& operator=(const PoolAllocated& /*that*/);



        apr_pool_t *pool()
            {
                return pool_;
            }



        const apr_pool_t *pool() const
            {
                return pool_;
            }



        static apr_size_t *getAllocAddress(const PoolAllocated *op)
            {
                if (op->dynAlloc_)
                {
                    apr_size_t as = APR_ALIGN_DEFAULT(sizeof(apr_size_t));
                    apr_size_t ps = APR_ALIGN_DEFAULT(sizeof(apr_pool_t *));
                    apr_size_t us = APR_ALIGN_DEFAULT(sizeof(SubPoolUsage));

                    return
                        (apr_size_t *)(((char *)op) - us - ps - ps - as);
                }
                else
                {
                    return 0;
                }
            }



        static apr_size_t getAllocSize(const PoolAllocated *op)
            {
                if (op->dynAlloc_)
                {
                    return *getAllocAddress(op);
                }
                else
                {
                    return 0;
                }
            }



        static apr_pool_t *getPool(const PoolAllocated *op)
            {
                if (op->dynAlloc_)
                {
                    apr_size_t ps = APR_ALIGN_DEFAULT(sizeof(apr_pool_t *));
                    apr_size_t us = APR_ALIGN_DEFAULT(sizeof(SubPoolUsage));

                    return
                        *((apr_pool_t **)(((char *)op) - us - ps - ps));
                }
                else
                {
                    return 0;
                }
            }



        static apr_pool_t *getObjPool(const PoolAllocated *op)
            {
                apr_pool_t *p = 0;

                if (op->dynAlloc_)
                {
                    apr_size_t ps = APR_ALIGN_DEFAULT(sizeof(apr_pool_t *));
                    apr_size_t us = APR_ALIGN_DEFAULT(sizeof(SubPoolUsage));

                    p = *((apr_pool_t **)(((char *)op) - us - ps));
                }

                return p;
            }



        static SubPoolUsage getSubPoolUsage(const PoolAllocated *op)
            {
                if (op->dynAlloc_)
                {
                    apr_size_t us = APR_ALIGN_DEFAULT(sizeof(SubPoolUsage));

                    return *((SubPoolUsage *)(((char *)op) - us));
                }
                else
                {
                    return NoSubPools;
                }
            }



        static void clearSubPoolUsage(const PoolAllocated *op)
            {
                if (op->dynAlloc_)
                {
                    apr_size_t us = APR_ALIGN_DEFAULT(sizeof(SubPoolUsage));

                    *((SubPoolUsage *)(((char *)op) - us)) = NoSubPools;
                }
            }



        static apr_pool_t *usePool(apr_pool_t *p, const PoolAllocated *op)
            {
                if (p != 0)
                {
                    return p;
                }
                else
                {
                    return getObjPool(op);
                }
            }



        static apr_status_t PoolAllocatedCleanup(void *);



    protected:
        PoolAllocated(const PoolAllocated& /*from*/);



    private:
        bool dynAlloc_;
        apr_pool_t *pool_;
        apr_pool_t *destroyerPool_;


    };



    class PoolAllocatedLight
    {
    public:
        PoolAllocatedLight(apr_pool_t *p)
            : pool_(p)
            {
                assert(pool_);
            }



        PoolAllocatedLight(apr_pool_t *p, const PoolAllocatedLight& /*from*/)
            : pool_(p)
            {
                assert(pool_);
            }



        virtual ~PoolAllocatedLight()
            { }



        PoolAllocatedLight& operator=(const PoolAllocatedLight& /*that*/)
            {
                return *this;
            }



        void *operator new(size_t s, apr_pool_t *p)
            {
                return apr_palloc(p, s);
            }


        void operator delete(void *op) { }



        apr_pool_t *pool()
            {
                return pool_;
            }



        const apr_pool_t *pool() const
            {
                return pool_;
            }



    protected:
        PoolAllocatedLight(const PoolAllocatedLight& /*from*/)
            : pool_(0)
            { }



    private:
        apr_pool_t *pool_;

    };



    struct PoolAllocatedLight2
    {
        virtual ~PoolAllocatedLight2()
            { }

        void *operator new(size_t s, apr_pool_t *p)
            {
                return apr_pcalloc(p, s);
            }

        void operator delete(void *op) { }
    };


}



#endif // RUM_POOLALLOCATED_H
