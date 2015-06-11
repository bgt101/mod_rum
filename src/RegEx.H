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



#ifndef RUM_REGEX_H
#define RUM_REGEX_H


#include "httpd.h"
#include "ap_regex.h"
#include "apr_pools.h"
#include "PoolAllocated.H"
#include "StrVec.H"
#include "Logger.H"



namespace rum
{

    class RegEx : public PoolAllocatedLight
    {
    public:
        class MatchData : public PoolAllocatedLight
        {
        public:
            MatchData(apr_pool_t *p, Logger *l, apr_size_t nc, const char *str,
                      ap_regmatch_t *pmatch)
                : PoolAllocatedLight(p),
                  logger_(l),
                  numCaptures_(0),
                  captures_(pool())
                {
                    numCaptures_ = nc;
                    apr_size_t i;
                    for (i = 0; i < nc; i++)
                    {
                        if (pmatch[i].rm_so == -1)
                        {
                            captures_.push_back("");
                        }
                        else
                        {
                            const char *beg = str + pmatch[i].rm_so;
                            apr_size_t len = pmatch[i].rm_eo - pmatch[i].rm_so;
                            captures_.push_back(apr_pstrndup(pool(), beg,
                                                             len));
                        }
                    }
                    RUM_LOG_REGEX(logger_, APLOG_DEBUG,
                                  "captures: " << captures_);
                }


            virtual ~MatchData()
                { }


            void initialize(apr_size_t nc, const char *str,
                            ap_regmatch_t *pmatch)
                {
                    numCaptures_ = nc;
                    apr_size_t i;
                    for (i = 0; i < nc; i++)
                    {
                        if (pmatch[i].rm_so == -1)
                        {
                            captures_.push_back("");
                        }
                        else
                        {
                            const char *beg = str + pmatch[i].rm_so;
                            apr_size_t len = pmatch[i].rm_eo - pmatch[i].rm_so;
                            captures_.push_back(apr_pstrndup(pool(), beg,
                                                             len));
                        }
                    }
                    RUM_LOG_REGEX(logger_, APLOG_DEBUG,
                                  "captures: " << captures_);
                }



            const char *operator[](apr_size_t n) const
                {
                    return captures_[n];
                }



            apr_size_t size() const
                {
                    return numCaptures_;
                }

        private:
            Logger *logger_;
            apr_size_t numCaptures_;
            StrVec captures_;


            MatchData(const MatchData&)
                : PoolAllocatedLight(0),
                  logger_(0),
                  numCaptures_(0),
                  captures_(0)
                {
                    // this method is private and should not be used
                }


            MatchData& operator=(const MatchData& that)
                {
                    // this method is private and should not be used
                    PoolAllocatedLight::operator=(that);
                    return *this;
                }

        };



        RegEx(apr_pool_t *p, Logger *l, const char *pattern__,
              apr_ssize_t numCaptures__ = -1)
            : PoolAllocatedLight(p),
              logger_(l),
              pattern_(apr_pstrdup(pool(), pattern__)),
              numCaptures_(1),
              re_(),
              valid_(true)
            {
                RUM_PTRC_REGEX(pool(), "RegEx::RegEx(apr_pool_t *p, "
                               "const char *pattern__, "
                               "apr_ssize_t numCaptures__), "
                               "this: " << (void *)this
                               << ", pattern: " << pattern__
                               << ", numCaptures__: " << numCaptures__);

                if (numCaptures__ == -1)
                {
                    // calculate number of capture buffers required by
                    // counting the number of open parens in pattern
                    const char *cp = pattern__;
                    for (; *cp; cp++)
                    {
                        if (*cp == '(')
                        {
                            numCaptures_++;
                        }
                    }
                }
                else
                {
                    numCaptures_ = numCaptures__;
                }
                RUM_LOG_REGEX(logger_, APLOG_DEBUG,
                              "numCaptures_: " << numCaptures_);

                if (ap_regcomp(&re_, pattern_, AP_REG_EXTENDED) != 0)
                {
                    RUM_LOG_REGEX(logger_, APLOG_ERR,
                                  "error compiling regular expression: "
                                  << pattern_);

                    valid_ = false;
                }
            }



        RegEx(apr_pool_t *p, const RegEx& from)
            : PoolAllocatedLight(p, from),
              logger_(from.logger_),
              pattern_(apr_pstrdup(pool(), from.pattern_)),
              numCaptures_(from.numCaptures_),
              re_(),
              valid_(from.valid_)
            {
                if (ap_regcomp(&re_, pattern_, AP_REG_EXTENDED) != 0)
                {
                    RUM_LOG_REGEX(logger_, APLOG_ERR,
                                  "error compiling regular expression: "
                                  << pattern_);

                    valid_ = false;
                }
            }



        virtual ~RegEx()
            {
                RUM_PTRC_REGEX(pool(), "RegEx::~RegEx(), "
                               "this: " << (void *)this);

                ap_regfree(&re_);
            }



        const MatchData *match(const char *str, apr_pool_t *mdPool,
                               Logger *mdLogger) const
            {
                const MatchData *md = NULL;

                if (!valid_)
                {
                    return md;
                }

                if (mdPool)
                {
                    apr_size_t nmatch = numCaptures_;
                    ap_regmatch_t *pmatch =
                        (ap_regmatch_t *)
                        apr_palloc(mdPool, sizeof(ap_regmatch_t) * nmatch);

                    if (ap_regexec(&re_, str, nmatch, pmatch, 0) == 0)
                    {
                        md = new (mdPool) MatchData(mdPool, mdLogger,
                                                    numCaptures_, str, pmatch);
                    }
                }

                return md;
            }



        const char *pattern() const
            {
                return pattern_;
            }



        const ap_regex_t& re() const
            {
                return re_;
            }



    protected:
        RegEx(const RegEx& from)
            : PoolAllocatedLight(from),
              logger_(from.logger_),
              pattern_(0),
              numCaptures_(0),
              re_(),
              valid_(false)
            { }



    private:
        Logger *logger_;
        const char *pattern_;
        apr_size_t numCaptures_;
        ap_regex_t re_;
        bool valid_;

        RegEx& operator=(const RegEx& that)
            {
                PoolAllocatedLight::operator=(that);
                return *this;
            }


    };

}


#endif // RUM_REGEX_H
