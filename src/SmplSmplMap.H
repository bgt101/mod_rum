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



#ifndef RUM_SMPLSMPLMAP_H
#define RUM_SMPLSMPLMAP_H


#include "debug.H"
#include "apr_hash.h"
#include "SmplPtrMap.H"
#include "PoolAllocated.H"



namespace rum
{


    // SmplsmplMap is a map for storing simple types using a simple
    // type as the key



    template <typename T>
    struct SmplSmplMapBucket : public PoolAllocatedLight2
    {
        SmplSmplMapBucket()
            : val(0)
            { }
        T val;
    };



    template <typename T>
    inline StrBuffer& operator<<(StrBuffer& sb, const SmplSmplMapBucket<T>& b)
    {
        return sb << b.val;
    }



    template <typename S, typename T>
    class SmplSmplMap : public SmplPtrMap<S, SmplSmplMapBucket<T> * >
    {
    public:


        SmplSmplMap(apr_pool_t *p)
            : SmplPtrMap<S, SmplSmplMapBucket<T> * >(p)
            { }


        virtual ~SmplSmplMap()
            { }



        T operator[](apr_size_t i) const
            {
                const SmplSmplMapBucket<T> *bucket =
                    SmplPtrMap<S, SmplSmplMapBucket<T> * >::find(i);
                if (bucket)
                {
                    return bucket->val;
                }
                else
                {
                    T t;
                    memset(&t, 0, sizeof(T));
                    return t;
                }
            }



        T& operator[](apr_size_t i)
            {
                SmplSmplMapBucket<T> *bucket =
                    SmplPtrMap<S, SmplSmplMapBucket<T> * >::find(i);
                if (!bucket)
                {
                    apr_pool_t *bPool =
                        SmplPtrMap<S, SmplSmplMapBucket<T> * >::pool();
                    bucket = new (bPool) SmplSmplMapBucket<T>();

                    SmplPtrMap<S, SmplSmplMapBucket<T> * >::insert(i, bucket);
                }

                return bucket->val;
            }

    };

}


#endif // RUM_SMPLSMPLMAP_H
