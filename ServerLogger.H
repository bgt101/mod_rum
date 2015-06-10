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



#ifndef RUM_SERVERLOGGER_H
#define RUM_SERVERLOGGER_H


#include "Logger.H"



namespace rum
{
    class ServerLogger : public Logger
    {
    public:
        ServerLogger(apr_pool_t *p, int level__,
#if RUM_AP22
#else
                     int aplogModuleIndex__,
#endif
                     server_rec *server__)
            : Logger(p, level__),
              server_(server__)
#if RUM_AP22
#else
              , aplogModuleIndex_(aplogModuleIndex__)
#endif
            { }



        server_rec *server()
            {
                return server_;
            }



        void log2(const char *file, int line, int level, const char *s);



        ServerLogger(const ServerLogger& from)
            : Logger(from),
              server_(from.server_)
#if RUM_AP22
#else
              , aplogModuleIndex_(from.aplogModuleIndex_)
#endif
            { }



        ServerLogger& operator=(const ServerLogger& that)
            {
                Logger::operator=(that);
                server_ = that.server_;
                return *this;
            }



    private:
        server_rec* server_;
#if RUM_AP22
#else
        int aplogModuleIndex_;
#endif
    };

}



#endif // RUM_SERVERLOGGER_H
