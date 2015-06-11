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



#include <assert.h>
#include "debug.H"
#include "PoolAllocated.H"


namespace rum
{

    PoolAllocated::PoolAllocated(apr_pool_t *p)
        : dynAlloc_(p == 0),
          pool_(usePool(p, this)),
          destroyerPool_(0)
    {
        RUM_STRC_POOL(255, "PoolAllocated::PoolAllocated(), this: "
                      << (void *)this
                      << ", p: " << p
                      << ", pool_: " << pool_
                      << ", dynamic: " << dynAlloc_);

        assert(pool_);
    }



    PoolAllocated::PoolAllocated(apr_pool_t *p,
                                 const PoolAllocated& /*from*/)
        : dynAlloc_(p == 0),
          pool_(usePool(p, this)),
          destroyerPool_(0)
    {
        RUM_STRC_POOL(255, "PoolAllocated::PoolAllocated("
                      "apr_pool_t *p, "
                      "const PoolAllocated& from), this: "
                      << (void *)this
                      << ", p: " << p
                      << ", pool_: " << pool_
                      << ", dynamic: " << dynAlloc_
                      /*<< ", from: " << (void *)&from*/);

        assert(pool_);
    }



    PoolAllocated::PoolAllocated(const PoolAllocated& /*from*/)
        : dynAlloc_(false),
          pool_(0),
          destroyerPool_(0)
    {
        RUM_STRC_POOL(255, "PoolAllocated::PoolAllocated("
                      "const PoolAllocated& from), this: "
                      << (void *)this
                      << ", dynamic: " << dynAlloc_
                      /*<< ", from: " << (void *)&from*/);

        assert(pool_);
    }



    PoolAllocated::~PoolAllocated()
    {
        RUM_STRC_POOL(255, "PoolAllocated::~PoolAllocated(), this: "
                      << (void *)this
                      << ", dynamic: " << dynAlloc_);

        destroyWithPool(0);
    }



    void PoolAllocated::destroyWithPool(apr_pool_t *p)
    {
        // if changing registration with pool
        if (destroyerPool_ != p)
        {
            // if previously registered, unregister
            if (destroyerPool_ != 0)
            {
                RUM_STRC_POOL(255, "PoolAllocated::destroyWithPool("
                              "apr_pool_t *pool), this: "
                              << (void *)this
                              << ", unregistering with pool: "
                              << (void *)destroyerPool_);

                apr_pool_cleanup_kill(destroyerPool_,
                                      (void*)this,
                                      PoolAllocatedCleanup);
            }

            destroyerPool_ = p;

            // register with pool
            if (destroyerPool_ != 0)
            {
                RUM_STRC_POOL(255, "PoolAllocated::destroyWithPool("
                              "apr_pool_t *p), this: "
                              << (void *)this
                              << ", registering with pool: "
                              << (void *)destroyerPool_);

                apr_pool_pre_cleanup_register(destroyerPool_,
                                              (void *)this,
                                              PoolAllocatedCleanup);
            }

        }
    }



    void* PoolAllocated::operator new(size_t s,
                                      apr_pool_t *ppool,
                                      SubPoolUsage u)
    {
        // allocate extra space to store:
        //
        //   * number of bytes allocated
        //   * parent pool pointer
        //   * pool pointer used by this object
        //   * SubPoolUsage
        //
        // the extra data is stored at the beginning of the
        // allocated block, and the object pointer returned by
        // the new operator is adjusted by adding to it the
        // appropriate amount

        apr_size_t as = APR_ALIGN_DEFAULT(sizeof(apr_size_t));
        apr_size_t ps = APR_ALIGN_DEFAULT(sizeof(apr_pool_t *));
        apr_size_t us = APR_ALIGN_DEFAULT(sizeof(SubPoolUsage));
        apr_size_t ns = as + ps + ps + us + s;



        apr_pool_t *objPool;

        if (u == UseSubPools)
        {
            apr_pool_create(&objPool, ppool);
        }
        else
        {
            objPool = ppool;
        }



        void *p = apr_palloc(objPool, ns);

        // store the size of the allocated block
        *((apr_size_t *)p) = ns;

        // store the pool pointer
        *((apr_pool_t **)(((char *)p) + as)) = ppool;

        // store the object's pool pointer
        *((apr_pool_t **)(((char *)p) + as + ps)) = objPool;

        // store the SubPoolUsage
        *((SubPoolUsage *)(((char *)p) + as + ps + ps)) = u;

        // object pointer points to memory following the
        // allocated block size and the pool pointer
        void *op = ((char *)p) + as + ps + ps + us;

        RUM_PTRC_POOL(ppool, "PoolAllocated::operator new("
                      "size_t s, apr_pool_t *ppool, "
                      "SubPoolUsage u = UseSubPools)"
                      << ", u: " << (int)u
                      << ", ppool: " << ppool
                      << ", objPool: " << objPool
                      << ", " << ns << " @ " << p
                      << ", op: " << op);

        return op;
    }



    void PoolAllocated::operator delete(void* op)
    {
        if (((PoolAllocated *)op)->dynAlloc_)
        {
            const PoolAllocated *pa = (const PoolAllocated *)op;

            RUM_STRC_POOL(255, "PoolAllocated::operator delete("
                          "void* op), "
                          << getAllocSize(pa)
                          << " @ "
                          << (void *)getAllocAddress(pa)
                          << ", SubPoolUsage: "
                          << (int)getSubPoolUsage(pa)
                          << ", objPool: "
                          << (void *)getObjPool(pa));

            if (getSubPoolUsage(pa) == UseSubPools)
            {
                apr_pool_destroy(getObjPool(pa));
            }
        }
        else
        {
            RUM_STRC_POOL(255, "PoolAllocated::operator delete("
                          "void* op), " <<
                          "op: " << (void *)op <<
                          ", on stack");
        }
    }



    PoolAllocated& PoolAllocated::operator=(const PoolAllocated& /*that*/)
    {
        RUM_PTRC_POOL(pool(), "PoolAllocated::operator=("
                      "const PoolAllocated& that), "
                      << "this: " << (void *)this
                      /*<< ", that: " << (void *)&that*/);

        return *this;
    }




    apr_status_t PoolAllocated::PoolAllocatedCleanup(void *p)
    {
        RUM_STRC_POOL(255, "PoolAllocated::"
                      "PoolAllocatedCleanup: deleting: " << p);

        PoolAllocated *pa = (PoolAllocated *)p;
        clearSubPoolUsage(pa);
        delete pa;
        return APR_SUCCESS;
    }

}
