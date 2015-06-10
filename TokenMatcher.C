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



#include "TokenMatcher.H"
#include "TokenRulesMap.H"
#include "SizeVec.H"
#include "StrVec.H"
#include "MatchedIdxs.H"
#include "SmplSmplMap.H"



#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif



namespace rum
{
    void TokenMatcher::procPattern(const char *pattern,
                                   apr_pool_t *pTmp,
                                   apr_size_t ruleIdx,
                                   StrBuffer *regExStr,
                                   apr_size_t *numClusters,
                                   const char *rePrefix,
                                   const char *reSuffix,
                                   bool *matchAll)
    {
        RUM_PTRC_TOKMATCH(pool(), "TokenMatcher::procPattern("
                          "const char *pattern, apr_size_t ruleIdx, "
                          "StrBuffer *regExStr, apr_size_t *numClusters, "
                          "const char *rePrefix, "
                          "const char *reSuffix, bool *matchAll), "
                          "pattern: " << pattern
                          << ", ruleIdx: " << ruleIdx);

        PtrVec<StrVec *> tokSets(pool());
        parsePattern(pattern, pTmp, &tokSets);
        RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG, "tokSets: " << tokSets);

        *matchAll = true;
        reqMatchBitSets_.grow_to(ruleIdx + 1);
        reqMatchAnyCt_.grow_to(ruleIdx + 1);
        minToks_.grow_to(ruleIdx + 1);
        maxToks_.grow_to(ruleIdx + 1);

        apr_ssize_t ts = tokSets.size();
        apr_ssize_t lWalk = 0;
        apr_ssize_t rWalk = ts - 1;
        bool lLook = true;
        bool rLook = true;
        apr_ssize_t numStarStar = 0;
        const apr_ssize_t lMax = MIN((numLeftMaps_ - 1), rWalk);

        for (; lLook && (lWalk <= lMax); lWalk++)
        {
            const StrVec *tokSet = tokSets[lWalk];
            apr_ssize_t nToks = tokSet->size();
            apr_ssize_t ti;
            for (ti = 0; ti < nToks; ti++)
            {
                const char *tok = (*tokSet)[ti];
                RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                                 "LEFT: lWalk: " << lWalk
                                 << ", (*tokSet)[" << ti << "]: "
                                 << tok);

                if (strcmp(tok, "**") == 0)
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG, "LEFT: found **");
                    lLook = false;
                    numStarStar++;
                }
                else if (strcmp(tok, "*") == 0)
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG, "LEFT: found *");
                }
                else
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                                     "LEFT: found word: " << tok);
                    tokenRulesMap_.storeLeft(tok, lWalk, ruleIdx);
                    reqMatchBitSets_[ruleIdx] |= leftMapBit(lWalk);
                }
            }
        }


        const apr_ssize_t rMin = MAX(lWalk, (ts - numRightMaps_));

        for (; rLook && (rWalk >= rMin); rWalk--)
        {
            const StrVec *tokSet = tokSets[rWalk];
            apr_ssize_t nToks = tokSet->size();
            apr_ssize_t ti;
            for (ti = 0; ti < nToks; ti++)
            {
                const char *tok = (*tokSet)[ti];
                RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                                 "RIGHT: rWalk: " << rWalk
                                  << ", (*tokSet)[" << ti << "]: "
                                  << tok);

                if (strcmp(tok, "**") == 0)
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG, "RIGHT: found **");
                    rLook = false;
                    numStarStar++;
                }
                else if (strcmp(tok, "*") == 0)
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG, "RIGHT: found *");
                }
                else
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                                     "RIGHT: found word: " << tok);
                    tokenRulesMap_.storeRight(tok, ts - rWalk - 1, ruleIdx);
                    reqMatchBitSets_[ruleIdx] |= rightMapBit(ts - rWalk - 1);
                }
            }
        }


        // add remaining tokens to any map
        for (; lWalk <= rWalk; lWalk++)
        {
            const StrVec *tokSet = tokSets[lWalk];
            apr_ssize_t nToks = tokSet->size();
            apr_ssize_t ti;
            bool anyCtIncd = false;
            for (ti = 0; ti < nToks; ti++)
            {
                const char *tok = (*tokSet)[ti];
                RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                                 "ANY: lWalk: " << lWalk
                                  << ", (*tokSet)[" << ti << "]: "
                                  << tok);

                if (strcmp(tok, "**") == 0)
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG, "ANY: found **");
                    numStarStar++;
                }
                else if (strcmp(tok, "*") == 0)
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG, "ANY: found *");
                }
                else
                {
                    RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                                     "ANY: found word: " << tok);

                    // store token only if token not already stored
                    const SizeVec *iRIdxs =
                        tokenRulesMap_.getRuleIdxsAny(pTmp, tok);
                    if (!(iRIdxs && iRIdxs->contains(ruleIdx)))
                    {
                        tokenRulesMap_.storeAny(tok, ruleIdx);
                        reqMatchBitSets_[ruleIdx] |= AnyMapBit;
                    }

                    // increment any count if not already done for
                    // this token set
                    if (!anyCtIncd)
                    {
                        reqMatchAnyCt_[ruleIdx]++;
                        anyCtIncd = true;
                    }
                }
            }
        }


        minToks_[ruleIdx] = ts - numStarStar;
        maxToks_[ruleIdx] =
            (numStarStar > 0) ? static_cast<apr_ssize_t>(MaxToks) : ts;
        RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                         "ProcPattern: ts: " << ts
                         << ", numStarStar: " << numStarStar
                         << ", minToks: " << minToks_[ruleIdx]
                         << ", maxToks: " << maxToks_[ruleIdx]);

        if (reqMatchBitSets_[ruleIdx] != 0)
        {
            // rule index stored at least once, so we have a narrowing
            // condition which will provide matches
            *matchAll = false;
        }

        // create a RegEx string to be used in a filtering condition
        mkRegExStr(&tokSets, regExStr, numClusters, rePrefix, reSuffix);
        RUM_LOG_TOKMATCH(logger_, APLOG_DEBUG,
                         "ProcPattern: RegEx: " << *regExStr);
    }



    void TokenMatcher::postProc()
    {
        tokenRulesMap_.sortVectors();

        // keep track of which positions were actually used so
        // we can avoid needless lookups
        apr_ssize_t i;
        const apr_ssize_t sz = reqMatchBitSets_.size();
        for (i = 0; i < sz; i++)
        {
            mapPosUsage_ |= reqMatchBitSets_[i];
        }
    }



    apr_status_t TokenMatcher::parsePattern(const char *pattern,
                                            apr_pool_t *pTmp,
                                            PtrVec<StrVec *> *tokSets)
    {
        // example path:
        //   (gadgets|gizmos)/fall/(reviews|ratings)
        //
        // contains the following token sets:
        //   (gadgets|gizmos)
        //   fall
        //   (reviews|ratings)
        //
        // the first token set contains the following tokens:
        //   gadgets
        //   gizmos


        char *pattern2 = apr_pstrdup(pTmp, pattern);

        char *strtokState_p2;
        char *tokSetStr;
        tokSetStr = apr_strtok(pattern2, delim_, &strtokState_p2);
        while (tokSetStr)
        {
            apr_size_t tssSz = strlen(tokSetStr);

            if ((tssSz < 1) ||
                ((*tokSetStr == '(') && (tssSz < 3)))
            {
                // invalid token set
                return APR_EGENERAL;
            }


            StrVec *tokSet = new (tokSets->pool()) StrVec(0);
            tokSets->push_back(tokSet);


            if (*tokSetStr == '(')
            {
                // skip over "(" and ")"
                tokSetStr++;
                tokSetStr[tssSz - 2] = '\0';

                char *strtokState_tss;
                const char *token;
                token = apr_strtok(tokSetStr, "|", &strtokState_tss);
                while (token)
                {
                    if (strchr(token, '*'))
                    {
                        // invalid token
                        return APR_EGENERAL;
                    }
                    // store each token from token set
                    tokSet->push_back(token);
                    token = apr_strtok(0, "|", &strtokState_tss);
                }
            }
            else
            {
                // store token
                tokSet->push_back(tokSetStr);
            }

            tokSetStr = apr_strtok(0, delim_, &strtokState_p2);
        }

        return APR_SUCCESS;
    }



    void TokenMatcher::mkRegExStr(PtrVec<StrVec *> *tokSets,
                                  StrBuffer *regExStr, apr_size_t *numClusters,
                                  const char *rePrefix, const char *reSuffix)
    {
        // */bbb ==> ^/([^/]+)/(bbb)$
        // **/(bbb|Bbb|BBB) ==> ^/([^/]+/+)*(bbb|Bbb|BBB)$
        // (aaa|Aaa|AAA)/**/bbb ==> ^/(aaa|Aaa|AAA)/([^/]+/)*(bbb)$
        // (aaa|Aaa|AAA)/** ==> ^/(aaa|Aaa|AAA)(/+[^/]+)*$


        StrBuffer delimRE(regExStr->pool());
        delimRE << "\\" << delim_;

        StrBuffer leftOrMiddleStarStarRE(regExStr->pool());
        leftOrMiddleStarStarRE << "(?:[^" << delimRE << "]+" << delimRE
                               << "+)*";

        StrBuffer rightStarStarRE(regExStr->pool());
        rightStarStarRE << "(?:" << delimRE << "+[^" << delimRE << "]+)*";

        StrBuffer starRE(regExStr->pool());
        starRE << "[^" << delimRE << "]+";

        bool prevStarStar = true;
        apr_ssize_t ts = tokSets->size();


        *regExStr << rePrefix;


        *numClusters = 0;
        apr_ssize_t i = 0;
        for (; i < ts; i++)
        {
            const StrVec *tokSet = (*tokSets)[i];
            apr_size_t nToks = tokSet->size();

            // add the regex for the delim separator between token
            // sets if the previous token was not a "**" and we're not
            // looking at "**" as the last token
            if (!prevStarStar &&
                !((i == ts - 1) &&
                  (nToks != 0) &&
                  (strcmp((*tokSet)[0], "**") == 0)))
            {
                *regExStr << delimRE << "+";
            }



            apr_size_t ti;
            for (ti = 0; ti < nToks; ti++)
            {
                if (ti == 0)
                {
                    // add opening paren in token set
                    *regExStr << "(";
                }


                if (strcmp((*tokSet)[ti], "*") == 0)
                {
                    prevStarStar = false;
                    *regExStr << starRE;
                }
                else if (strcmp((*tokSet)[ti], "**") == 0)
                {
                    prevStarStar = true;
                    if (i != (ts - 1))
                    {
                        *regExStr << leftOrMiddleStarStarRE;
                        (*numClusters)++;
                    }
                    else
                    {
                        *regExStr << rightStarStarRE;
                        (*numClusters)++;
                    }
                }
                else
                {
                    prevStarStar = false;
                    *regExStr << (*tokSet)[ti];
                }


                if (ti == (nToks - 1))
                {
                    // add closing paren in token set
                    *regExStr << ")";
                }
                else
                {
                    // add OR separator in token set
                    *regExStr << "|";
                }
            }
        }


        *regExStr << reSuffix;
    }



    void TokenMatcher::tokenize(char delim, const char *str, StrVec *toks)
    {
        char delimArr[] = {delim, '\0'};
        char *str2 = apr_pstrdup(toks->pool(), str);
        char *strtokState;
        char *tok;
        tok = apr_strtok(str2, delimArr, &strtokState);
        while (tok)
        {
            toks->push_back(tok);
            tok = apr_strtok(0, delimArr, &strtokState);
        }
    }



    void TokenMatcher::lookup(Logger *theLogger, const StrVec& toks,
                              MatchedIdxs *ruleIdxs) const
    {
        apr_pool_t *thePool = ruleIdxs->pool();
        const apr_ssize_t ts = toks.size();

        // potential rule indices
        SizeVec pRIdxs(thePool);

        // final rule indices
        SizeVec fRIdxs(thePool);

        // map usage for this lookup
        SmplSmplMap<apr_ssize_t, short> lookupMapUsage(thePool);

        // number of "any" matches
        SmplSmplMap<apr_ssize_t, int> lookupAnyCt(thePool);

        // maximum left and right positions
        const apr_ssize_t maxL = MIN(ts, numLeftMaps_);
        const apr_ssize_t maxR = MIN(ts, numRightMaps_);

        apr_ssize_t i, j;

        for (i = 0; i < maxL; i++)
        {
            if (mapPosUsage_ & leftMapBit(i))
            {
                const char *tok = toks[i];

                // intermediate rule indices
                const SizeVec *iRIdxs =
                    tokenRulesMap_.getRuleIdxsLeft(thePool, tok, i);
                if (iRIdxs)
                {
                    // set the lookup map usage for each of the rule
                    // indices for the current position
                    apr_ssize_t isz = iRIdxs->size();
                    for (j = 0; j < isz; j++)
                    {
                        lookupMapUsage[(*iRIdxs)[j]] |= leftMapBit(i);
                    }

                    // add intermediate rule indices to potential rule indices
                    pRIdxs.union_with(*iRIdxs);
                }
            }
        }
        RUM_LOG_TOKMATCH(theLogger, APLOG_DEBUG,
                         "after left scan, pRIdxs: " << pRIdxs);


        for (i = 0; i < maxR; i++)
        {
            if (mapPosUsage_ & rightMapBit(i))
            {
                const char *tok = toks[ts - i - 1];

                // intermediate rule indices
                const SizeVec *iRIdxs =
                    tokenRulesMap_.getRuleIdxsRight(thePool, tok, i);
                if (iRIdxs)
                {
                    // set the lookup map usage for each of the rule
                    // indices for the current position
                    apr_ssize_t isz = iRIdxs->size();
                    for (j = 0; j < isz; j++)
                    {
                        lookupMapUsage[(*iRIdxs)[j]] |= rightMapBit(i);
                    }

                    // add intermediate rule indices to potential rule indices
                    pRIdxs.union_with(*iRIdxs);
                }
            }
        }
        RUM_LOG_TOKMATCH(theLogger, APLOG_DEBUG,
                         "after right scan, pRIdxs: " << pRIdxs);


        // get result for the "any" position
        if (mapPosUsage_ & AnyMapBit)
        {
            for (i = 0; i < ts; i++)
            {
                const char *tok = toks[i];

                // intermediate rule indices
                const SizeVec *iRIdxs =
                    tokenRulesMap_.getRuleIdxsAny(thePool, tok);
                if (iRIdxs)
                {
                    // set the lookup map usage and increment the
                    // "any" count for each of the rule indices
                    apr_ssize_t isz = iRIdxs->size();
                    for (j = 0; j < isz; j++)
                    {
                        lookupMapUsage[(*iRIdxs)[j]] |= AnyMapBit;
                        lookupAnyCt[(*iRIdxs)[j]]++;
                    }

                    // add intermediate rule indices to potential rule indices
                    pRIdxs.union_with(*iRIdxs);
                }
            }
        }
        RUM_LOG_TOKMATCH(theLogger, APLOG_DEBUG,
                         "after any scan, pRIdxs: " << pRIdxs);

        RUM_LOG_TOKMATCH(theLogger, APLOG_DEBUG,
                         "lookupMapUsage: " << lookupMapUsage);
        RUM_LOG_TOKMATCH(theLogger, APLOG_DEBUG,
                         "lookupAnyCt: " << lookupAnyCt);


        // for each of the potential rule indices ensure that the
        // required conditions are met
        apr_ssize_t nPot = pRIdxs.size();
        for (i = 0; i < nPot; i++)
        {
            const apr_ssize_t ruleIdx = pRIdxs[i];
            if ((reqMatchBitSets_[ruleIdx] ==
                (reqMatchBitSets_[ruleIdx] & lookupMapUsage[ruleIdx])) &&
                (reqMatchAnyCt_[ruleIdx] <= lookupAnyCt[ruleIdx]) &&
                (ts >= minToks_[ruleIdx]) && (ts <= maxToks_[ruleIdx]))
            {
                RUM_LOG_TOKMATCH(theLogger, APLOG_DEBUG,
                                 "matched ruleIdx: " << ruleIdx);
                fRIdxs.push_back(ruleIdx);
            }
        }

        ruleIdxs->union_with(fRIdxs);

    }



    StrBuffer& operator<<(StrBuffer& sb, const TokenMatcher& tm)
    {
        return sb << "delim: '" << tm.delim_ << "'" << nl
                  << "numLeftMaps: " << tm.numLeftMaps_ << nl
                  << "numRightMaps: " << tm.numRightMaps_ << nl
                  << "tokenRulesMap: " << indent << nl
                  << tm.tokenRulesMap_ << outdent << nl
                  << "reqMatchBitSets: " << indent << nl
                  << tm.reqMatchBitSets_ << outdent << nl
                  << "reqMatchAnyCt: " << indent << nl
                  << tm.reqMatchAnyCt_ << outdent << nl
                  << "mapPosUsage: " << indent << nl
                  << tm.mapPosUsage_ << outdent << nl
                  << "minToks: " << indent << nl
                  << tm.minToks_ << outdent << nl
                  << "maxToks: " << indent << nl
                  << tm.maxToks_ << outdent;
    }
}
