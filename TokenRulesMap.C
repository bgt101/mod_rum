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



#include "TokenRulesMap.H"


namespace rum
{
    TokenRulesMap::TokenRulesMap(apr_pool_t *p)
        : PoolAllocated(p),
          hash_(0)
    {
        RUM_PTRC_RULESMAP(pool(), "TokenRulesMap::TokenRulesMap("
                          "apr_pool_t *p), "
                          "this: " << (void *)this);
        hash_ = apr_hash_make(pool());
    }



    TokenRulesMap::~TokenRulesMap()
    {
        RUM_PTRC_RULESMAP(pool(),
                          "TokenRulesMap::~TokenRulesMap(), "
                          "this: " << (void *)this);

        apr_hash_index_t *hi = apr_hash_first(pool(), hash_);

        for (; hi; hi = apr_hash_next(hi))
        {
            void *val;
            apr_hash_this(hi, NULL, NULL, &val);
            delete (SizeVec *)val;
        }
    }



    void TokenRulesMap::store(const char *token, apr_ssize_t pos,
                              apr_ssize_t ruleIdx)
    {
        // create new entry if key does not yet exist, and add
        // ruleIdx to ruleIdxs vector associated with key

        RUM_PTRC_RULESMAP(pool(), "TokenRulesMap::store("
                          "const char *token, "
                          "apr_ssize_t pos, "
                          "apr_ssize_t ruleIdx), "
                          "this: " << (void *)this
                          << ", token: " << token
                          << ", pos: " << pos
                          << ", ruleIdx: " << ruleIdx);

        void *key;
        apr_ssize_t klen;
        mkKey(pool(), token, pos, &key, &klen);

        SizeVec *ruleIdxs = (SizeVec *)apr_hash_get(hash_, key, klen);
        if (ruleIdxs == 0)
        {
            ruleIdxs = new (pool()) SizeVec(0);
            apr_hash_set(hash_, key, klen, ruleIdxs);
        }

        ruleIdxs->push_back(ruleIdx);
    }



    void TokenRulesMap::sortVectors()
    {
        RUM_PTRC_RULESMAP(pool(),
                          "TokenRulesMap::sortVectors(), "
                          "this: " << (void *)this);

        apr_hash_index_t *hi = apr_hash_first(pool(), hash_);

        for (; hi; hi = apr_hash_next(hi))
        {
            void *val;
            apr_hash_this(hi, NULL, NULL, &val);
            SizeVec *ruleIdxs = (SizeVec *)val;
            ruleIdxs->sort();
            ruleIdxs->unique();
        }
    }



    StrBuffer& operator<<(StrBuffer& sb, const TokenRulesMap& trm)
    {
        TokenRulesMap::ConstIterator it(sb.pool(), trm);
        apr_ssize_t i = trm.size();
        while (it.next())
        {
            sb << "[" << it.keyPos() << ", \"" << it.keyTok() << "\"] => "
               << indent << nl << *it << outdent;
            if (--i > 0)
            {
                sb << nl;
            }
        }
        return sb;
    }

}
