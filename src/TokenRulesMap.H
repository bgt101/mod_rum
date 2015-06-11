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



#ifndef RUM_TOKENRULESMAP_H
#define RUM_TOKENRULESMAP_H


#include "debug.H"
#include "apr_hash.h"
#include "PoolAllocated.H"
#include "SizeVec.H"



namespace rum
{


    // TokenRulesMap is a map for storing vectors of rule indices
    // using a token with its position as the key


    class TokenRulesMap : public PoolAllocated
    {
    public:
        TokenRulesMap(apr_pool_t *p);



        virtual ~TokenRulesMap();



        // create the hash key by combining the token with its
        // position; trailing null byte is included to allow
        // extracting the token from the key without the need for a
        // buffer
        static void mkKey(apr_pool_t *p,
                          const char *token, apr_ssize_t pos,
                          void **key, apr_ssize_t *klen)
            {
                const apr_ssize_t poslen = sizeof(apr_ssize_t);
                const apr_ssize_t toklen = strlen(token);
                *klen = poslen + toklen + 1;
                *key = apr_palloc(p, *klen);
                apr_ssize_t *pp = static_cast<apr_ssize_t *>(*key);
                *pp = pos;
                char *tp = reinterpret_cast<char *>(pp + 1);
                memcpy(tp, token, toklen);
                tp[toklen] = '\0';
            }



        // extract the position component of the key
        static apr_ssize_t keyPos(const void *key)
            {
                return *(static_cast<const apr_ssize_t *>(key));
            }



        // extract the token component of the key
        static const char *keyTok(const void *key)
            {
                const apr_ssize_t *pp = static_cast<const apr_ssize_t *>(key);
                return reinterpret_cast<const char *>(pp + 1);
            }



        static apr_ssize_t posKeyLeft(apr_ssize_t pos)
            {
                return pos;
            }



        static apr_ssize_t posKeyRight(apr_ssize_t pos)
            {
                return (-1 - pos);
            }



        static apr_ssize_t posKeyAny()
            {
                return 9999;
            }



        void storeLeft(const char *token, apr_ssize_t pos,
                       apr_ssize_t ruleIdx)
            {
                store(token, posKeyLeft(pos), ruleIdx);
            }



        void storeRight(const char *token, apr_ssize_t pos,
                        apr_ssize_t ruleIdx)
            {
                store(token, posKeyRight(pos), ruleIdx);
            }



        void storeAny(const char *token, apr_ssize_t ruleIdx)
            {
                store(token, posKeyAny(), ruleIdx);
            }



        const SizeVec *getRuleIdxsLeft(apr_pool_t *keyPool,
                                       const char *token,
                                       apr_ssize_t pos) const
            {
                return getRuleIdxs(keyPool, token, posKeyLeft(pos));
            }



        const SizeVec *getRuleIdxsRight(apr_pool_t *keyPool,
                                        const char *token,
                                        apr_ssize_t pos) const
            {
                return getRuleIdxs(keyPool, token, posKeyRight(pos));
            }



        const SizeVec *getRuleIdxsAny(apr_pool_t *keyPool,
                                      const char *token) const
            {
                return getRuleIdxs(keyPool, token, posKeyAny());
            }



        apr_size_t size() const
            {
                return apr_hash_count(hash_);
            }



        void sortVectors();



        class Iterator : public PoolAllocated
        {
        public:
            Iterator(apr_pool_t *p, TokenRulesMap& map)
                : PoolAllocated(p),
                  initialized_(0),
                  map_(map),
                  hi_(0)
                {
                    RUM_PTRC_RULESMAP(pool(),
                                      "TokenRulesMap::Iterator::Iterator("
                                      "apr_pool_t *p, "
                                      "const TokenRulesMap& map)");
                }



            void reset()
                {
                    initialized_ = 0;
                }



            bool next()
                {
                    if (! initialized_)
                    {
                        hi_ = apr_hash_first(pool(), map_.hash_);
                        initialized_ = true;
                    }
                    else
                    {
                        hi_ = apr_hash_next(hi_);
                    }

                    return (hi_ != 0);
                }



            SizeVec& operator*() const
                {
                    void *vVal;
                    apr_hash_this(hi_, NULL, NULL, &vVal);
                    return *((SizeVec *)vVal);
                }



            apr_ssize_t keyPos() const
                {
                    const void *vKey;
                    apr_hash_this(hi_, &vKey, NULL, NULL);
                    return TokenRulesMap::keyPos(vKey);
                }



            const char *keyTok() const
                {
                    const void *vKey;
                    apr_hash_this(hi_, &vKey, NULL, NULL);
                    return TokenRulesMap::keyTok(vKey);
                }



            SizeVec *val() const
                {
                    void *vVal;
                    apr_hash_this(hi_, NULL, NULL, &vVal);
                    return static_cast<SizeVec *>(vVal);
                }



        private:
            bool initialized_;
            TokenRulesMap& map_;
            apr_hash_index_t *hi_;

            Iterator(const Iterator& from)
                : PoolAllocated(0),
                  initialized_(0),
                  map_(from.map_),
                  hi_(0)
                { }

            Iterator& operator=(const Iterator& that)
                {
                    PoolAllocated::operator=(that);
                    return *this;
                }
        };



        class ConstIterator : public Iterator
        {
        public:
            ConstIterator(apr_pool_t *p, const TokenRulesMap& map)
                : Iterator(p, const_cast<TokenRulesMap&>(map))
                { }



            const SizeVec& operator*()
                {
                    return Iterator::operator*();
                }



            const SizeVec *val() const
                {
                    return Iterator::val();
                }
        };



    private:
        apr_hash_t *hash_;



        TokenRulesMap(const TokenRulesMap &from)
            : PoolAllocated(from),
              hash_(0)
            {
                // this method is private and should not be used
            }



        void store(const char *token, apr_ssize_t pos, apr_ssize_t ruleIdx);



        const SizeVec *getRuleIdxs(apr_pool_t *keyPool, const char *token,
                                   apr_ssize_t pos) const
            {
                RUM_PTRC_RULESMAP(pool(), "TokenRulesMap::getRuleIdxs("
                                  "const char *token), "
                                  "apr_ssize_t pos, "
                                  "this: " << (void *)this
                                  << ", token: " << token
                                  << ", pos: " << pos);

                void *key;
                apr_ssize_t klen;
                mkKey(keyPool, token, pos, &key, &klen);
                return static_cast<SizeVec *>(apr_hash_get(hash_, key, klen));
            }



        TokenRulesMap& operator=(const TokenRulesMap& that)
            {
                // this method is private and should not be used
                PoolAllocated::operator=(that);
                return *this;
            }



        friend StrBuffer& operator<<(StrBuffer& sb, const TokenRulesMap& trm);

    };


}


#endif // RUM_TOKENRULESMAP_H
