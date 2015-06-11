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



#ifndef RUM_BLOB_H
#define RUM_BLOB_H


#include "PoolAllocated.H"



namespace rum
{
    class Blob : public PoolAllocatedLight
    {
    public:
        Blob(apr_pool_t *p, const void *data__, apr_ssize_t size__,
             bool copy = true)
            : PoolAllocatedLight(p),
              data_(0),
              size_(size__)
            {
                if (copy)
                {
                    void *dst = apr_palloc(pool(), size_);
                    memcpy(dst, data__, size_);
                    data_ = dst;
                }
                else
                {
                    data_ = data__;
                }
            }



        Blob(apr_pool_t *p, const Blob& from)
            : PoolAllocatedLight(p, from),
              data_(0),
              size_(from.size_)
            {
                void *dst = apr_palloc(pool(), size_);
                memcpy(dst, from.data_, size_);
                data_ = dst;
            }



        const void *data() const
            {
                return data_;
            }



        int size() const
            {
                return static_cast<int>(size_);
            }



        void set(const void *data__, apr_ssize_t size__, bool copy = true)
            {
                size_ = size__;
                if (copy)
                {
                    void *dst = apr_palloc(pool(), size_);
                    memcpy(dst, data__, size_);
                    data_ = dst;
                }
                else
                {
                    data_ = data__;
                }
            }



    protected:
        Blob(const Blob& from)
            : PoolAllocatedLight(from),
              data_(from.data_),
              size_(from.size_)
            { }



        Blob& operator=(const Blob& that)
            {
                PoolAllocatedLight::operator=(that);
                data_ = that.data_;
                size_ = that.size_;
                return *this;
            }



    private:
        const void *data_;
        apr_ssize_t size_;
    };

}



#endif // RUM_BLOB_H
