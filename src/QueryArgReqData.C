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



#include "QueryArgReqData.H"
#include "ReqCtx.H"
#include "apr_strings.h"
#include "string.h"
#include "util_uri.H"
#include "util_misc.H"



namespace rum
{

    const StrPtrMap<StrVec *>&
    QueryArgReqData::queryArgValVecMap()
    {
        if (!isParsedQueryArgs_)
        {
            parse(false);
            isParsedQueryArgs_ = true;
        }

        return queryArgValVecMap_;
    }



    const StrPtrMap<StrVec *>&
    QueryArgReqData::queryArgRawValVecMap()
    {
        if (!isParsedRawQueryArgs_)
        {
            parse(true);
            isParsedRawQueryArgs_ = true;
        }

        return queryArgRawValVecMap_;
    }



    const StrPtrMap<StrVec *>&
    QueryArgReqData::queryArgAnyValVecMap()
    {
        if (isParsedRawQueryArgs_)
        {
            return queryArgRawValVecMap();
        }
        else
        {
            return queryArgValVecMap();
        }
    }



    void QueryArgReqData::parse(bool raw)
    {
        StrPtrMap<StrVec *> *map;
        if (raw)
        {
            map = &queryArgRawValVecMap_;
        }
        else
        {
            map = &queryArgValVecMap_;
        }

        if (map->size() > 0)
        {
            map->clear();
        }


        if (reqCtx()->req()->args)
        {
            char *args2 = apr_pstrdup(pool(), reqCtx()->req()->args);

            // ignore optional #fragment at the end of the query string
            *rum_strchrnul(args2, '#') = '\0';

            char *state;
            char *tok;
            tok = apr_strtok(args2, "&", &state);
            while (tok)
            {
                if (*tok != '\0')
                {
                    char *qaName = tok;
                    char *qaVal = NULL;
                    char *eq = strchr(tok, '=');
                    if (eq)
                    {
                        *eq = '\0';
                        if (eq + 1)
                        {
                            qaVal = eq + 1;
                        }
                    }

                    // URI decode name
                    rum_uri_decode(qaName);

                    if (!raw)
                    {
                        // URI decode value
                        rum_uri_decode(qaVal);
                    }

                    StrVec *valVec = map->find(qaName);
                    if (valVec == 0)
                    {
                        valVec = new (pool()) StrVec(0);
                        map->insert(tok, valVec);
                    }
                    valVec->push_back(qaVal);
                }
                tok = apr_strtok(0, "&", &state);
            }
        }
    }



    const StrVec *
    QueryArgReqData::queryArgValVec(const char *qaName)
    {
        return queryArgValVecMap().find(qaName);
    }



    const StrVec *
    QueryArgReqData::queryArgRawValVec(const char *qaName)
    {
        return queryArgRawValVecMap().find(qaName);
    }



    void QueryArgReqData::reset()
    {
        queryArgValVecMap_.clear();
        queryArgRawValVecMap_.clear();
        isParsedQueryArgs_ = false;
        isParsedRawQueryArgs_ = false;
    }



    StrBuffer& QueryArgReqData::write(StrBuffer& sb) const
    {
        return sb << "queryArgValVecMap: " << nl << indent
                  << queryArgValVecMap_ << outdent
                  << "isParsedQueryArgs: " << isParsedQueryArgs_;
    }


}
