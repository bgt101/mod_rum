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



#ifndef RUM_TOKENMATCHER_H
#define RUM_TOKENMATCHER_H


#include "apr_errno.h"
#include "PoolAllocated.H"
#include "PtrVec.H"
#include "SizeVec.H"
#include "TokenRulesMap.H"
#include "Logger.H"



namespace rum
{
    // forward declarations
    class StrBuffer;
    class StrVec;
    class ReqCtx;
    class MatchedIdxs;



    class TokenMatcher : public PoolAllocated
    {
    public:
        enum
        {
            AnyMapBit = 1,
            MaxToks = 9999
        };


        TokenMatcher(apr_pool_t *p, Logger *l, char delim,
                     apr_ssize_t numLeftMaps, apr_ssize_t numRightMaps)
            : PoolAllocated(p),
              logger_(l),
              delim_(delimArr_),
              numLeftMaps_(numLeftMaps),
              numRightMaps_(numRightMaps),
              tokenRulesMap_(pool()),
              reqMatchBitSets_(pool()),
              reqMatchAnyCt_(pool()),
              minToks_(pool()),
              maxToks_(pool()),
              mapPosUsage_(0)
            {
                delimArr_[0] = delim;
                delimArr_[1] = '\0';
            }



        virtual ~TokenMatcher()
            { }



        void procPattern(const char *pattern, apr_pool_t *pTmp,
                         apr_size_t ruleIdx,
                         StrBuffer *regExStr, apr_size_t *numClusters,
                         const char *rePrefix, const char *reSuffix,
                         bool *matchAll);



        apr_status_t parsePattern(const char *pattern,
                                  apr_pool_t *pTmp,
                                  PtrVec<StrVec *> *tokSets);



        static void tokenize(char delim, const char *str, StrVec *toks);



        short leftMapBit(apr_ssize_t i) const
            {
                return static_cast<short>(1 << (1 + i));
            }



        short rightMapBit(apr_ssize_t i) const
            {
                return static_cast<short>((1 << (1 + numLeftMaps_ + i)));
            }



        void mkRegExStr(PtrVec<StrVec *> *tokSets, StrBuffer *regExStr,
                        apr_size_t *numClusters,
                        const char *rePrefix, const char *reSuffix);



        void postProc();



        void lookup(Logger *l, const StrVec& toks,
                    MatchedIdxs *ruleIdxs) const;



    private:
        Logger *logger_;
        const char *delim_;
        char delimArr_[2];
        apr_ssize_t numLeftMaps_;
        apr_ssize_t numRightMaps_;
        TokenRulesMap tokenRulesMap_;
        SizeVec reqMatchBitSets_;
        SizeVec reqMatchAnyCt_;
        SizeVec minToks_;
        SizeVec maxToks_;
        apr_ssize_t mapPosUsage_;



        TokenMatcher(const TokenMatcher& from)
            : PoolAllocated(from),
              logger_(from.logger_),
              delim_(0),
              numLeftMaps_(0),
              numRightMaps_(0),
              tokenRulesMap_(0),
              reqMatchBitSets_(0),
              reqMatchAnyCt_(0),
              minToks_(0),
              maxToks_(0),
              mapPosUsage_(0)
            {
                // this method is private and should not be used
            }



        TokenMatcher& operator=(const TokenMatcher& that)
            {
                // this method is private and should not be used
                PoolAllocated::operator=(that);
                return *this;
            }



        friend StrBuffer& operator<<(StrBuffer& sb, const TokenMatcher& tm);

    };

}


#endif // RUM_TOKENMATCHER_H
