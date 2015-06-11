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



#include "apr_xml.h"

#include "debug.H"
#include "get_cdata.H"
#include "rum_errno.H"
#include "StrVec.H"
#include "Config.H"
#include "CondModule.H"
#include "FiltCond.H"
#include "Rule.H"
#include "LuaAction.H"
#include "RedirIntAction.H"
#include "RedirExtAction.H"
#include "ReqCtx.H"
#include "ActionCtx.H"
#include "CoreCondModule.H"
#include "RequestCondModule.H"
#include "PathCondModule.H"
#include "QueryArgCondModule.H"
#include "Logger.H"
#include "util_misc.H"
#include "apr_fnmatch.h"
#include "MatchedIdxs.H"
#include "TmpPool.H"



namespace rum
{

    Config::Config(apr_pool_t *p, Logger *l, apr_pool_t *pTmp,
                   const char *baseDir__, const StrVec& configFiles__,
                   apr_ssize_t maxLookups__, LuaManager *luaManager__)
        : PoolAllocated(p),
          logger_(l),
          baseDir_(baseDir__),
          configFiles_(pool()),
          ctorError_(false),
          maxLookups_(maxLookups__),
          luaManager_(luaManager__),
          condModulesMap_(pool()),
          condUsageMap_(pool()),
          filtConds_(pool()),
          rules_(pool()),
          commonPreActions_(pool()),
          defnScripts_(pool()),
          defnScriptFiles_(pool()),
          errHandlerScriptUsed_(false),
          errHandlerScriptFileUsed_(false),
          errHandlerChunk_(0),
          phaseUsageVec_(pool(), Phases::numPhases()),
          condPhaseUsageVec_(pool(), Phases::numPhases()),
          actionPhaseUsageVec_(pool(), Phases::numPhases()),
          phaseRulesIdxs_(pool(), Phases::numPhases())
    {
        RUM_PTRC_CONFIG(pool(), "Config::Config(), this: "
                        << (void *)this);

        registerStaticCondModules();

        StrPtrMap<CondModule *>::Iterator it(pTmp, condModulesMap_);
        while (it.next())
        {
            RUM_LOG_CONFIG(logger_, APLOG_DEBUG,
                           "registering LuaPrepEnv call back for CondModule: "
                           << it.key());
            luaManager_->registerPrepEnvCB(it.val()->getLuaPrepEnvCB());
        }


        apr_ssize_t i;
        apr_ssize_t sz = configFiles__.size();
        for (i = 0; i < sz; i++)
        {
            const char *canonGlob =
                rum_base_dir_relative(pTmp, baseDir_, configFiles__[i]);

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "canonGlob: " << canonGlob);

            bool isGlob;
            apr_ssize_t numFiles =
                addFilesFromGlob(&configFiles_, pTmp, &isGlob, canonGlob);
            if (numFiles == 0)
            {
                if (isGlob)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_WARNING,
                                   "no match for RUM config glob: "
                                   << canonGlob);
                }
                else
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "RUM config file does not exist: "
                                   << canonGlob);
                    ctorError_ = true;
                }
            }
        }

        if (!ctorError_ && (procXMLFiles(pTmp) != APR_SUCCESS))
        {
            RUM_LOG_CONFIG(logger_, APLOG_ERR,
                           "error processing RUM config files: " <<
                           configFiles_);
            ctorError_ = true;
        }

        const apr_ssize_t nPhases = Phases::numPhases();
        phaseRulesIdxs_.grow_to(nPhases);
        phaseUsageVec_.grow_to(nPhases);
        condPhaseUsageVec_.grow_to(nPhases);
        for (i = 0; i < nPhases; i++)
        {
            phaseUsageVec_[i] = false;
            condPhaseUsageVec_[i] = false;
            actionPhaseUsageVec_[i] = false;
            phaseRulesIdxs_[i] = new (pool()) SizeVec(0);
        }

        const apr_ssize_t nRules = rules_.size();
        for (i = 0; i < nRules; i++)
        {
            Rule *rule = rules_[i];
            Phases::Phase phase = rule->condPhase();
            phaseRulesIdxs_[phase]->push_back(i);
            phaseUsageVec_[phase] = true;
            condPhaseUsageVec_[phase] = true;
            apr_ssize_t j;
            apr_ssize_t nRuleActions = rule->actions().size();
            for (j = 0; j < nRuleActions; j++)
            {
                phase = rule->actions()[j]->phase();
                phaseUsageVec_[phase] = true;
                actionPhaseUsageVec_[phase] = true;
            }
        }
    }



    Config::~Config()
    {
        RUM_PTRC_CONFIG(pool(), "Config::~Config(), this: "
                        << (void *)this);
    }



    apr_ssize_t Config::addFilesFromGlob(StrVec *files, apr_pool_t *pTmp,
                                         bool *isGlob, const char *glob)
    {
        apr_ssize_t numMatches = 0;

        if (apr_fnmatch_test(glob))
        {
            *isGlob = true;


#ifdef WIN32
            const char pathSep = '\\';
#else
            const char pathSep = '/';
#endif

            const char *idx = strrchr(glob, pathSep);

            const char *path;
            if (idx == NULL) {
                path = ".";
            }
            else {
                path = apr_pstrndup(pTmp, glob, idx - glob);
                glob = idx + 1;
            }

            apr_status_t rv;
            apr_dir_t *dir;
            rv = apr_dir_open(&dir, path, pTmp);
            if (rv == APR_SUCCESS)
            {
                apr_finfo_t finfo;
                const apr_int32_t finfoFlags = APR_FINFO_NAME;
                while (apr_dir_read(&finfo, finfoFlags, dir) == APR_SUCCESS)
                {
                    if (apr_fnmatch(glob, finfo.name, 0) == APR_SUCCESS)
                    {
                        StrBuffer buf(pTmp, path);
                        buf << pathSep << finfo.name;
                        files->push_back(buf);
                        numMatches++;
                    }
                }
                apr_dir_close(dir);
            }
        }
        else
        {
            *isGlob = false;
            files->push_back(glob);
            numMatches++;
        }

        return numMatches;
    }



    apr_status_t Config::registerStaticCondModules()
    {
        CondModule *cm;

        cm = new (pool())
             CoreCondModule(0, logger_, condModulesMap_.size());
        condModulesMap_.insert(cm->condName(), cm);

        cm = new (pool())
             RequestCondModule(0, logger_, condModulesMap_.size());
        condModulesMap_.insert(cm->condName(), cm);

        cm = new (pool())
             PathCondModule(0, logger_, condModulesMap_.size());
        condModulesMap_.insert(cm->condName(), cm);

        cm = new (pool())
             QueryArgCondModule(0, logger_, condModulesMap_.size());
        condModulesMap_.insert(cm->condName(), cm);

        return APR_SUCCESS;
    }



    apr_status_t Config::procXMLFiles(apr_pool_t *pTmp)
    {
        RUM_PTRC_CONFIG(pool(),
                        "Config::procXMLFiles()");

        BlobSmplMap<apr_size_t> filtCondIdxMap(pTmp);

        apr_ssize_t i;
        apr_ssize_t sz = configFiles_.size();
        for (i = 0; i < sz; i++)
        {
            RUM_LOG_CONFIG(logger_, APLOG_DEBUG,
                           "processing conf file: " << configFiles_[i]);

            if (procXMLFile(pTmp, configFiles_[i], &filtCondIdxMap) !=
                APR_SUCCESS)
            {
                return APR_EGENERAL;
            }
        }


        // post process condition modules
        StrPtrMap<CondModule *>::Iterator it(pTmp, condModulesMap_);
        while (it.next())
        {
            RUM_LOG_CONFIG(logger_, APLOG_DEBUG,
                           "post processing CondModule: " << it.key());
            it.val()->postConfProc();
        }


        // tell Lua manager about the global definitions
        luaManager_->updateDefinitions(defnScripts_, defnScriptFiles_);


        return APR_SUCCESS;
    }



    apr_status_t Config::procXMLFile(apr_pool_t *pTmp, const char *confFile,
                                     BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        RUM_PTRC_CONFIG(pool(),
                        "Config::procXMLFile(const char *fileName), "
                        "fileName: " << confFile);

        apr_file_t *fd;
        apr_status_t status;
        apr_xml_parser *parser;
        apr_xml_doc *xmlDoc;

        status = apr_file_open(&fd, confFile, (APR_READ | APR_BUFFERED),
                               APR_OS_DEFAULT, pTmp);
        if (status != APR_SUCCESS)
        {
            RUM_LOG_CONFIG(logger_, APLOG_ERR,
                           "unable to open config file: " << confFile);
            return status;
        }

        apr_xml_parse_file(pTmp, &parser, &xmlDoc, fd, 2000);
        if (parser != NULL)
        {
            RUM_LOG_CONFIG(logger_, APLOG_ERR,
                           "unable to parse config file: " << confFile);
            return APR_EGENERAL;
        }


        if (strcmp(xmlDoc->root->name, "RumConf") != 0)
        {
            RUM_LOG_CONFIG(logger_, APLOG_ERR,
                           "unrecognized root name in XML : "
                           << xmlDoc->root->name);
            return APR_EGENERAL;
        }


        // process XML elements
        apr_xml_elem *element;
        for (element = xmlDoc->root->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            if (strcmp(element->name, "CommonPreAction") == 0)
            {
                status = procXML_CommonPreAction(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "Definitions") == 0)
            {
                status = procXML_Definitions(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "ErrorHandler") == 0)
            {
                status = procXML_ErrorHandler(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "Rules") == 0)
            {
                status = procXML_Rules(pTmp, element, filtCondIdxMap);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized XML in <"
                               << xmlDoc->root->name << ">: <"
                               << element->name << ">");
                return APR_EGENERAL;
            }
        }


        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_Rules(apr_pool_t *pTmp,
                                       const apr_xml_elem *elem,
                                       BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        apr_ssize_t ruleCounter = 0;
        TmpPool *tmpPool = 0;

        apr_xml_elem *element;
        for (element = elem->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            // use a new sub pool of the main temp pool after
            // processing a number of rules so that the main temp pool
            // doesn't keep growing
            if ((ruleCounter++ % 200) == 0)
            {
                delete tmpPool;
                tmpPool = new (pTmp) TmpPool(0);
            }

            if (strcmp(element->name, "Rule") == 0)
            {
                apr_status_t status = procXML_Rule(*tmpPool, element,
                                                   filtCondIdxMap);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized XML in <"
                               << elem->name << ">: <"
                               << element->name << ">");
                return APR_EGENERAL;
            }
        }


        return APR_SUCCESS;
    }




    apr_status_t Config::procXML_Rule(apr_pool_t *pTmp,
                                      const apr_xml_elem *elem,
                                      BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        apr_pool_t *rulePool = pool();
        Rule *rule = new (rulePool) Rule(rulePool);
        rules_.push_back(rule);
        apr_ssize_t ruleIdx = rules_.size() - 1;


        StrPtrMap<CondModule *>::ConstIterator it(pTmp, condModulesMap_);
        while (it.next())
        {
            RUM_LOG_CONFIG(logger_, APLOG_DEBUG,
                           "resetting usage: " << it.key());
            condUsageMap_[it.key()] = false;
        }


        apr_xml_elem *element;
        for (element = elem->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            if (strcmp(element->name, "Conditions") == 0)
            {
                apr_status_t status = procXML_Conditions(pTmp,
                                                         element,
                                                         ruleIdx,
                                                         rule->filtCondIdxs(),
                                                         filtCondIdxMap);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "Actions") == 0)
            {
                apr_status_t status = procXML_Actions(pTmp, element, ruleIdx);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized XML in <"
                               << elem->name << ">: <"
                               << element->name << ">");
                return APR_EGENERAL;
            }
        }


        // condition modules which did not appear in the rule cause
        // that rule to match all requests
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG,
                       "condUsageMap_: " << condUsageMap_);
        StrPtrMap<CondModule *>::Iterator it2(pTmp, condModulesMap_);
        while (it2.next())
        {
            if (!condUsageMap_[it2.key()])
            {
                RUM_LOG_CONFIG(logger_, APLOG_DEBUG,
                               "match all reqs; CondModule: "
                                << it2.key()
                                << ", ruleIdx: " << ruleIdx);
                it2.val()->matchAllReqs(ruleIdx, rule->condPhase());
            }
        }


        return APR_SUCCESS;
    }



    apr_status_t
    Config::procXML_Conditions(apr_pool_t *pTmp,
                               const apr_xml_elem *elem,
                               apr_ssize_t ruleIdx,
                               SizeVec *filtCondIdxs,
                               BlobSmplMap<apr_size_t> *filtCondIdxMap)
    {
        apr_xml_attr *attr;
        for (attr = elem->attr; attr; attr = attr->next)
        {
            if (strcmp(attr->name, "phase") == 0)
            {
                Phases::Phase phase = Phases::str2enum(attr->value);
                if (phase == Phases::InvalidPhase)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "invalid phase: " << attr->value);
                    return APR_EGENERAL;
                }
                rules_[ruleIdx]->setCondPhase(phase);
            }
        }

        Phases::Phase condPhase = rules_[ruleIdx]->condPhase();
        apr_xml_elem *element;
        for (element = elem->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            CondModule *cm = condModulesMap_.find(element->name);
            if (cm == 0)
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized condition: "
                               << element->name);
                return APR_EGENERAL;
            }
            apr_status_t status = cm->parseXMLCond(pTmp,
                                                   element,
                                                   ruleIdx,
                                                   condPhase,
                                                   &filtConds_,
                                                   filtCondIdxs,
                                                   filtCondIdxMap);
            if (status != APR_SUCCESS)
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML condition <"
                               << element->name << "> failed");
                return APR_EGENERAL;
            }

            condUsageMap_[element->name] = true;
        }

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_Actions(apr_pool_t *pTmp,
                                         const apr_xml_elem *elem,
                                         apr_ssize_t ruleIdx)
    {
        // default Actions phase
        Phases::Phase phase = rules_[ruleIdx]->condPhase();

        apr_xml_attr *attr;
        for (attr = elem->attr; attr; attr = attr->next)
        {
            if (strcmp(attr->name, "phase") == 0)
            {
                phase = Phases::str2enum(attr->value);
                if (phase == Phases::InvalidPhase)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "invalid phase: " << attr->value);
                    return APR_EGENERAL;
                }
            }
        }

        apr_xml_elem *element;
        for (element = elem->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            if (strcmp(element->name, "Script") == 0)
            {
                apr_status_t status = procXML_Script(pTmp,element, ruleIdx,
                                                     phase);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "ScriptFile") == 0)
            {
                apr_status_t status = procXML_ScriptFile(pTmp, element,
                                                         ruleIdx, phase);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "InternalRedirect") == 0)
            {
                apr_status_t status = procXML_InternalRedirect(pTmp, element,
                                                               ruleIdx, phase);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "ExternalRedirect") == 0)
            {
                apr_status_t status = procXML_ExternalRedirect(pTmp, element,
                                                               ruleIdx, phase);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized XML in <"
                               << elem->name << ">: <"
                               << element->name << ">");
                return APR_EGENERAL;
            }
        }


        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_Script(apr_pool_t *pTmp,
                                        const apr_xml_elem *elem,
                                        apr_ssize_t ruleIdx,
                                        Phases::Phase phase)
    {
        Rule *rule = rules_[ruleIdx];
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *script = rum_xml_get_cdata(elem, pTmp, 1);

        const Blob *chunk;
        apr_status_t st = luaManager_->compileScript(rule->pool(), script,
                                                     &chunk);
        if (st == APR_SUCCESS)
        {
            apr_pool_t *aPool = rule->pool();
            rule->addAction(new (aPool) LuaAction(aPool, logger_,
                                                  phase, chunk));
        }

        return st;
    }



    apr_status_t Config::procXML_ScriptFile(apr_pool_t *pTmp,
                                            const apr_xml_elem *elem,
                                            apr_ssize_t ruleIdx,
                                            Phases::Phase phase)
    {
        Rule *rule = rules_[ruleIdx];
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *scriptFile = rum_xml_get_cdata(elem, pTmp, 1);

        const Blob *chunk;
        apr_status_t st = luaManager_->compileScriptFile(rule->pool(),
                                                         scriptFile,
                                                         &chunk);
        if (st == APR_SUCCESS)
        {
            apr_pool_t *aPool = rule->pool();
            rule->addAction(new (aPool)
                            LuaAction(aPool, logger_, phase, chunk));
        }

        return st;
    }



    apr_status_t Config::procXML_InternalRedirect(apr_pool_t *pTmp,
                                                  const apr_xml_elem *elem,
                                                  apr_ssize_t ruleIdx,
                                                  Phases::Phase phase)
    {
        Rule *rule = rules_[ruleIdx];
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *uri = rum_xml_get_cdata(elem, pTmp, 1);
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "target URI: [[" << uri << "]]");
        apr_pool_t *aPool = rule->pool();
        rule->addAction(new (aPool)
                        RedirIntAction(aPool, logger_, phase, uri));

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_ExternalRedirect(apr_pool_t *pTmp,
                                                  const apr_xml_elem *elem,
                                                  apr_ssize_t ruleIdx,
                                                  Phases::Phase phase)
    {
        Rule *rule = rules_[ruleIdx];
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *url = rum_xml_get_cdata(elem, pTmp, 1);
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "target URL: [[" << url << "]]");
        apr_pool_t *aPool = rule->pool();
        rule->addAction(new (aPool)
                        RedirExtAction(aPool, logger_, phase, url));

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_Definitions(apr_pool_t *pTmp,
                                             const apr_xml_elem *elem)
    {
        apr_xml_elem *element;
        for (element = elem->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            if (strcmp(element->name, "Script") == 0)
            {
                apr_status_t status = procXML_DefnScript(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "ScriptFile") == 0)
            {
                apr_status_t status = procXML_DefnScriptFile(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized XML in <"
                               << elem->name << ">: <"
                               << element->name << ">");
                return APR_EGENERAL;
            }
        }

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_ErrorHandler(apr_pool_t *pTmp,
                                              const apr_xml_elem *elem)
    {
        apr_xml_elem *element;
        for (element = elem->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            if (strcmp(element->name, "Script") == 0)
            {
                if (errHandlerScriptFileUsed_)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "<Script> and <ScriptFile> may not be "
                                   "specified together in <ErrorHandler>");
                    return APR_EGENERAL;
                }
                if (errHandlerScriptUsed_)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "<Script> was already specified once in "
                                   "<ErrorHandler>");
                    return APR_EGENERAL;
                }
                errHandlerScriptUsed_ = true;
                apr_status_t status = procXML_ErrHandlerScript(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "ScriptFile") == 0)
            {
                if (errHandlerScriptUsed_)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "<Script> and <ScriptFile> may not be "
                                   "specified together in <ErrorHandler>");
                    return APR_EGENERAL;
                }
                if (errHandlerScriptFileUsed_)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "<ScriptFile> was already specified once in "
                                   "<ErrorHandler>");
                    return APR_EGENERAL;
                }
                errHandlerScriptFileUsed_ = true;
                apr_status_t status = procXML_ErrHandlerScriptFile(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized XML in <"
                               << elem->name << ">: <"
                               << element->name << ">");
                return APR_EGENERAL;
            }
        }

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_CommonPreAction(apr_pool_t *pTmp,
                                                 const apr_xml_elem *elem)
    {
        apr_xml_elem *element;
        for (element = elem->first_child;
             element != NULL;
             element = element->next) {

            RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "element: <"
                           << element->name << ">");

            if (strcmp(element->name, "Script") == 0)
            {
                apr_status_t status = procXML_CPAScript(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else if (strcmp(element->name, "ScriptFile") == 0)
            {
                apr_status_t status = procXML_CPAScriptFile(pTmp, element);
                if (status != APR_SUCCESS)
                {
                    RUM_LOG_CONFIG(logger_, APLOG_ERR, "parsing XML <"
                                   << element->name << "> failed");
                    return status;
                }
            }
            else
            {
                RUM_LOG_CONFIG(logger_, APLOG_ERR, "unrecognized XML in <"
                               << elem->name << ">: <"
                               << element->name << ">");
                return APR_EGENERAL;
            }
        }

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_CPAScript(apr_pool_t *pTmp,
                                           const apr_xml_elem *elem)
    {
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *script = rum_xml_get_cdata(elem, pTmp, 1);
        const Blob* chunk;
        apr_status_t st = luaManager_->compileScript(commonPreActions_.pool(),
                                                     script, &chunk);
        if (st == APR_SUCCESS)
        {
            commonPreActions_.push_back(chunk);
        }

        return st;
    }



    apr_status_t Config::procXML_CPAScriptFile(apr_pool_t *pTmp,
                                               const apr_xml_elem *elem)
    {
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *scriptFile = rum_xml_get_cdata(elem, pTmp, 1);
        const Blob* chunk;
        apr_status_t st =
            luaManager_->compileScriptFile(commonPreActions_.pool(),
                                           scriptFile, &chunk);
        if (st == APR_SUCCESS)
        {
            commonPreActions_.push_back(chunk);
        }

        return st;
    }



    apr_status_t Config::procXML_DefnScript(apr_pool_t *pTmp,
                                            const apr_xml_elem *elem)
    {
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *script = rum_xml_get_cdata(elem, pool(), 1);
        defnScripts_.push_back(script);

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_DefnScriptFile(apr_pool_t *pTmp,
                                                const apr_xml_elem *elem)
    {
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *scriptFile = rum_xml_get_cdata(elem, pool(), 1);
        defnScriptFiles_.push_back(scriptFile);

        return APR_SUCCESS;
    }



    apr_status_t Config::procXML_ErrHandlerScript(apr_pool_t *pTmp,
                                                  const apr_xml_elem *elem)
    {
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *script = rum_xml_get_cdata(elem, pool(), 1);
        apr_status_t st = luaManager_->compileScript(commonPreActions_.pool(),
                                                     script, &errHandlerChunk_);
        return st;
    }



    apr_status_t Config::procXML_ErrHandlerScriptFile(apr_pool_t *pTmp,
                                                      const apr_xml_elem *elem)
    {
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "processing element: <"
                       << elem->name << ">");
        const char *scriptFile = rum_xml_get_cdata(elem, pool(), 1);
        apr_status_t st =
            luaManager_->compileScriptFile(commonPreActions_.pool(),
                                           scriptFile, &errHandlerChunk_);
        return st;
    }



    void Config::lookupRules(ReqCtx *reqCtx, Phases::Phase phase) const
    {
        if (condPhaseUsage(phase))
        {
            // first apply the narrowing conditions from all condition
            // modules, intersecting the intermediate set of rules
            StrPtrMap<CondModule *>::ConstIterator it(reqCtx->pool(),
                                                      condModulesMap_);
            MatchedIdxs narrMatchedIdxs(reqCtx->pool(), phaseRulesIdxs(phase),
                                        true);
            MatchedIdxs cmMatchedIdxs(reqCtx->pool(), phaseRulesIdxs(phase));

            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                           "AAA3: reqCtx->ruleIdxs(): " << reqCtx->ruleIdxs());
            reqCtx->resetForLookup(phase);
            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                           "AAA4: reqCtx->ruleIdxs(): " << reqCtx->ruleIdxs());
            bool firstTime = true;
            bool done = false;
            while (!done && it.next())
            {
                cmMatchedIdxs.clear();
                it.val()->lookup(reqCtx, phase, &cmMatchedIdxs);
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                               "lookup result for module "
                               << it.key() << ": " << cmMatchedIdxs);

                narrMatchedIdxs.intersect_with(cmMatchedIdxs);

                if (narrMatchedIdxs.size() == 0)
                {
                    done = true;
                }
            }
            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                           "lookup result for all narrowing modules: "
                           << narrMatchedIdxs);


            // next apply the filtering conditions found in the rules
            // obtained using the narrowing conditions
            apr_ssize_t i;
            const apr_ssize_t sz = narrMatchedIdxs.size();
            for (i = 0; i < sz; i++)
            {
                const apr_ssize_t ruleIdx = narrMatchedIdxs.at(i);
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                               "considering rule index: "
                               << ruleIdx);
                const Rule& rule = *rules_[ruleIdx];
                const SizeVec& filtCondIdxs = *(rule.filtCondIdxs());
                apr_ssize_t i2;
                const apr_ssize_t sz2 = filtCondIdxs.size();
                bool match = true;
                for (i2 = 0; match && i2 < sz2; i2++)
                {
                    const apr_ssize_t filtCondIdx = filtCondIdxs[i2];
                    const FiltCondMatch *fcMatch =
                        reqCtx->filtCondMatches()->find(filtCondIdx);
                    if (!fcMatch)
                    {
                        FiltCondMatch *fcMatch2;
                        const FiltCond *filtCond = filtConds_[filtCondIdx];
                        RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                       "calling match() for filter "
                                       "condition " << filtCondIdx << ": "
                                       << indent << nl
                                       << *filtCond << outdent);
                        match = filtCond->match(reqCtx, &fcMatch2);
                        RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                       "filter condition "
                                       << filtCondIdx << " match: "
                                       << match);
                        reqCtx->filtCondMatches()->insert(filtCondIdx,
                                                          fcMatch2);
                    }
                    else
                    {
                        match = fcMatch->match();

                        RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                       "filter condition "
                                       << filtCondIdx << " cached match: "
                                       << match);
                    }
                }

                if (match)
                {
                    // there were no filtering conditions which did not match
                    RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                   "matched ruleIdx " << ruleIdx
                                   << " in phase " << Phases::enum2str(phase));
                    RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                   "reqCtx->ruleIdxs(): "
                                   << reqCtx->ruleIdxs());
                    reqCtx->addRuleIdx(phase, ruleIdx);
                    RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                   "reqCtx->ruleIdxs(): "
                                   << reqCtx->ruleIdxs());
                }
            }
        }
    }



    apr_status_t Config::runActions(ReqCtx *reqCtx, Phases::Phase phase) const
    {
        apr_status_t aStatus = RUM_DECLINED;
        if (actionPhaseUsage(phase))
        {
            bool delayedOK = false;
            apr_ssize_t numRules = reqCtx->ruleIdxs().size();
            apr_ssize_t i;
            for (i = 0; i < numRules; i++)
            {
                const apr_ssize_t ruleIdx = reqCtx->ruleIdxs()[i];
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                               "running actions for rule index " << ruleIdx);
                const Rule& rule = *rules_[ruleIdx];
                const PtrVec<Action *>& actions = rule.actions();
                ActionCtx actionCtx(reqCtx->pool(), reqCtx, ruleIdx);
                apr_ssize_t numActions = actions.size();
                apr_ssize_t j;
                for (j = 0; j < numActions; j++)
                {
                    const Action& action = *actions[j];
                    if (action.phase() == phase)
                    {
                        RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                       "running action index "
                                       << j << " for rule index " << ruleIdx);
                        aStatus = action.run(&actionCtx);
                        switch (aStatus)
                        {
                        case APR_EGENERAL:
                            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_ERR,
                                           "action index "
                                           << j << " failed for rule index "
                                           << ruleIdx);
                            return aStatus;
                        case RUM_OK:
                            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_INFO,
                                           "action index "
                                           << j << " for rule index "
                                           << ruleIdx << " returned OK");
                            return aStatus;
                        case RUM_DELAYED_OK:
                            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_INFO,
                                           "action index "
                                           << j << " for rule index "
                                           << ruleIdx
                                           << " returned DELAYED_OK");
                            delayedOK = true;
                            break;
                        case RUM_RELOOKUP:
                            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_INFO,
                                           "action index "
                                           << j << " for rule index "
                                           << ruleIdx
                                           << " initiated re-lookup");
                            return aStatus;
                        }
                    }
                }
            }

            if (delayedOK)
            {
                aStatus = RUM_OK;
            }
        }

        return aStatus;
    }



    int Config::lookupAndRun(ReqCtx *reqCtx, Phases::Phase phase) const
    {
        int ret = DECLINED;
        if (phaseUsage(phase))
        {
            bool done;
            apr_ssize_t i;
            apr_status_t aStatus = RUM_OK;
            for (done = false, i = 0;
                 !done && (i < maxLookups());
                 i++)
            {
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                               "AAA1: reqCtx->ruleIdxs(): "
                               << reqCtx->ruleIdxs());


                // perform lookups
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                               "phase: "
                               << Phases::enum2str(phase)
                               << ", performing lookups, i: " << i);
                lookupRules(reqCtx, phase);
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_INFO,
                               "phase: "
                               << Phases::enum2str(phase)
                               << ", matching rule indices: "
                               << reqCtx->ruleIdxs());

                // run the actions
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                               "phase: "
                               << Phases::enum2str(phase)
                               << ", running actions, i: " << i);
                aStatus = runActions(reqCtx, phase);
                if (aStatus == RUM_RELOOKUP)
                {
                    RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                   "phase: "
                                   << Phases::enum2str(phase)
                                   << ", will do another lookup");
                }
                else if(aStatus == APR_EGENERAL)
                {
                    RUM_LOG_CONFIG(reqCtx->logger(), APLOG_ERR,
                                   "phase: "
                                   << Phases::enum2str(phase)
                                   << ", error running action, bailing");
                    return aStatus;
                }
                else
                {
                    RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                                   "phase: "
                                   << Phases::enum2str(phase)
                                   << ", done with lookups");
                    done = true;
                }
            }
            if (!done)
            {
                RUM_LOG_CONFIG(reqCtx->logger(), APLOG_ERR,
                               "phase: "
                               << Phases::enum2str(phase)
                               << ", max number of lookups reached: "
                               << maxLookups());
            }

            if ((aStatus == RUM_OK) || (aStatus == RUM_DELAYED_OK))
            {
                ret = OK;
            }


            RUM_LOG_CONFIG(reqCtx->logger(), APLOG_DEBUG,
                           "AAA2: reqCtx->ruleIdxs(): " << reqCtx->ruleIdxs());
        }
        return ret;
    }



    StrBuffer& operator<<(StrBuffer& sb, const Config& c)
    {
        sb << "baseDir: " << nl << indent
           << c.baseDir() << nl << outdent
           << "configFiles: " << nl << indent
           << c.configFiles_ << nl << outdent;

        // StrBuffer wastes too much memory in the current quick and
        // dirty implementation, so only show details if number of
        // rules is not too much
        if (c.rules_.size() <= 50)
        {
            sb << "filtConds: " << nl << indent
               << c.filtConds_ << nl << outdent
               << "rules: " << nl << indent
               << c.rules_ << nl << outdent
               << "number of common-pre-actions: " << nl << indent
               << c.commonPreActions_.size() << nl << outdent
               << "condModulesMap: " << nl << indent
               << c.condModulesMap_ << outdent;
        }
        else
        {
            sb << "number of filter conditions: " << nl << indent
               << c.filtConds_.size() << nl << outdent
               << "number of rules: " << nl << indent
               << c.rules_.size() << nl << outdent
               << "number of common-pre-actions: " << nl << indent
               << c.commonPreActions_.size() << nl << outdent
               << "number of condition modules: " << nl << indent
               << c.condModulesMap_.size() << outdent;
        }

        return sb;
    }

}
