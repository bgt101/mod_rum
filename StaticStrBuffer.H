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



#ifndef RUM_STATICSTRBUFFER_H
#define RUM_STATICSTRBUFFER_H


#include <stdio.h>
#include <string.h>
#include "apr.h"



namespace rum
{
    template <int tsz>
    class StaticStrBuffer
    {
    public:
        StaticStrBuffer()
            {
                buf_[0] = 0;
            }

        enum
        {
            BufSize = tsz
        };


        StaticStrBuffer& append(char c)
            {
                size_t len = strlen(buf_);
                if (len < BufSize - 1)
                {
                    buf_[len] = c;
                    buf_[len + 1] = 0;
                }
                return *this;
            }


        StaticStrBuffer& append(const char *s)
            {
                size_t len = strlen(buf_);
                strncat(buf_ + len, s, BufSize - len - 1);
                return *this;
            }


        const char *asStr() const
            {
                return buf_;
            }


        operator const char*() const
            {
                return asStr();
            }


        void reset()
            {
                buf_[0] = 0;
            }



    private:
        char buf_[BufSize];

    };


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            const char *s)
    {
        return ssb.append(s);
    }


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            char c)
    {
        return ssb.append(c);
    }


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            int i)
    {
        const int sz = 41;
        char buf[sz];
        snprintf(buf, sz - 1, "%d", i);
        return ssb.append(buf);
    }


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            long l)
    {
        const int sz = 41;
        char buf[sz];
        snprintf(buf, sz - 1, "%ld", l);
        return ssb.append(buf);
    }


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            unsigned int i)
    {
        const int sz = 41;
        char buf[sz];
        snprintf(buf, sz - 1, "%u", i);
        return ssb.append(buf);
    }


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            unsigned long l)
    {
        const int sz = 41;
        char buf[sz];
        snprintf(buf, sz - 1, "%lu", l);
        return ssb.append(buf);
    }


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            void *p)
    {
        const int sz = 36;
        char buf[sz];
        snprintf(buf, sz - 1, "%p", p);
        return ssb.append(buf);
    }


    template <int tsz>
    inline StaticStrBuffer<tsz>& operator<<(StaticStrBuffer<tsz>& ssb,
                                            bool b)
    {
        if (b)
        {
            ssb.append("true");
        }
        else
        {
            ssb.append("false");
        }
        return ssb;
    }

}



#endif // RUM_STATICSTRBUFFER_H
