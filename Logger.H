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



#ifndef RUM_LOGGER_H
#define RUM_LOGGER_H


#include "httpd.h"
#include "http_log.h"
#include "PoolAllocated.H"



namespace rum
{
    class Logger : public PoolAllocated
    {
    public:
        Logger(apr_pool_t *p, int level__)
            : PoolAllocated(p),
              level_(level__)
            { }



        int level() const
            {
                return level_;
            }



        void log(const char *file, int line, int level__, const char *s)
            {
                if (level_ >= level__)
                {
                    log2(file, line, level__, s);
                }
            }



    protected:
        Logger(const Logger& from)
            : PoolAllocated(from),
              level_(from.level_)
            { }



        Logger& operator=(const Logger& that)
            {
                PoolAllocated::operator=(that);
                level_ = that.level_;
                return *this;
            }



        virtual void log2(const char *file, int line, int level,
                          const char *s) = 0;



    private:
        int level_;
    };

}



#endif // RUM_LOGGER_H
