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



#ifndef RUM_CONFIG_H
#define RUM_CONFIG_H


#include "PoolAllocated.H"
#include "StrPtrMap.H"
#include "StrSmplMap.H"
#include "PtrVec.H"
#include "SizeVec.H"
#include "StrVec.H"
#include "StrBuffer.H"
#include "FiltCond.H"
#include "Rule.H"
#include "CondModule.H"
#include "BlobVec.H"
#include "Phases.H"
#include "BlobSmplMap.H"



// forward declarations
class apr_xml_elem;



namespace rum
{
    // forward declarations
    class CondModule;
    class ReqCtx;
    class Logger;
    class LuaManager;



    class Config : public PoolAllocated
    {
    public:
        Config(apr_pool_t *, Logger *, apr_pool_t *pTmp, const char *baseDir__,
               const StrVec& configFiles__, apr_ssize_t maxLookups__,
               LuaManager *);



        virtual ~Config();



        static apr_ssize_t addFilesFromGlob(StrVec *files, apr_pool_t *pTmp,
                                            bool *isGlob, const char *glob);



        bool ctorError() const
            {
                return ctorError_;
            }



        const char *baseDir() const
            {
                return baseDir_;
            }



        apr_ssize_t maxLookups() const
            {
                return maxLookups_;
            }



        apr_status_t procXMLFiles(apr_pool_t *pTmp);



        apr_status_t procXMLFile(apr_pool_t *pTmp, const char *confFile,
                                 BlobSmplMap<apr_size_t> *filtCondIdxMap);



        apr_status_t procXML_Definitions(apr_pool_t *pTmp,
                                         const apr_xml_elem *elem);



        apr_status_t procXML_ErrorHandler(apr_pool_t *pTmp,
                                          const apr_xml_elem *elem);



        apr_status_t procXML_CommonPreAction(apr_pool_t *pTmp,
                                             const apr_xml_elem *elem);



        apr_status_t procXML_Rules(apr_pool_t *pTmp,
                                   const apr_xml_elem *elem,
                                   BlobSmplMap<apr_size_t> *filtCondIdxMap);



        apr_status_t procXML_Rule(apr_pool_t *pTmp, const apr_xml_elem *elem,
                                  BlobSmplMap<apr_size_t> *filtCondIdxMap);



        apr_status_t
        procXML_Conditions(apr_pool_t *pTmp,
                           const apr_xml_elem *elem,
                           apr_ssize_t ruleIdx,
                           SizeVec *filtCondIdxs,
                           BlobSmplMap<apr_size_t> *filtCondIdxMap);



        apr_status_t procXML_Actions(apr_pool_t *pTmp,
                                     const apr_xml_elem *elem,
                                     apr_ssize_t ruleIdx);



        apr_status_t procXML_Script(apr_pool_t *pTmp,
                                    const apr_xml_elem *elem,
                                    apr_ssize_t ruleIdx,
                                    Phases::Phase phase);



        apr_status_t procXML_ScriptFile(apr_pool_t *pTmp,
                                        const apr_xml_elem *elem,
                                        apr_ssize_t ruleIdx,
                                        Phases::Phase phase);



        apr_status_t procXML_CPAScript(apr_pool_t *pTmp,
                                       const apr_xml_elem *elem);



        apr_status_t procXML_CPAScriptFile(apr_pool_t *pTmp,
                                           const apr_xml_elem *elem);



        apr_status_t procXML_DefnScript(apr_pool_t *pTmp,
                                        const apr_xml_elem *elem);



        apr_status_t procXML_DefnScriptFile(apr_pool_t *pTmp,
                                            const apr_xml_elem *elem);



        apr_status_t procXML_ErrHandlerScript(apr_pool_t *pTmp,
                                              const apr_xml_elem *elem);



        apr_status_t procXML_ErrHandlerScriptFile(apr_pool_t *pTmp,
                                                  const apr_xml_elem *elem);



        apr_status_t procXML_InternalRedirect(apr_pool_t *pTmp,
                                              const apr_xml_elem *elem,
                                              apr_ssize_t ruleIdx,
                                              Phases::Phase phase);



        apr_status_t procXML_ExternalRedirect(apr_pool_t *pTmp,
                                              const apr_xml_elem *elem,
                                              apr_ssize_t ruleIdx,
                                              Phases::Phase phase);



        apr_status_t registerStaticCondModules();



        void lookupRules(ReqCtx *reqCtx, Phases::Phase phase) const;



        apr_status_t runActions(ReqCtx *reqCtx, Phases::Phase phase) const;



        int lookupAndRun(ReqCtx *reqCtx, Phases::Phase phase) const;



        const PtrVec<FiltCond *>& filtConds() const
            {
                return filtConds_;
            }



        const StrPtrMap<CondModule *>& condModulesMap() const
            {
                return condModulesMap_;
            }



        const BlobVec& commonPreActions() const
            {
                return commonPreActions_;
            }



        const StrVec& defnScripts() const
            {
                return defnScripts_;
            }



        const StrVec& defnScriptFiles() const
            {
                return defnScriptFiles_;
            }



        const Blob* errHandlerChunk() const
            {
                return errHandlerChunk_;
            }



        const PtrVec<Rule *>& rules() const
            {
                return rules_;
            }



        LuaManager *luaManager() const
            {
                return luaManager_;
            }



        bool phaseUsage(Phases::Phase phase) const
            {
                return phaseUsageVec_[phase];
            }



        bool condPhaseUsage(Phases::Phase phase) const
            {
                return condPhaseUsageVec_[phase];
            }



        bool actionPhaseUsage(Phases::Phase phase) const
            {
                return actionPhaseUsageVec_[phase];
            }



        const SizeVec *phaseRulesIdxs(Phases::Phase phase) const
            {
                return phaseRulesIdxs_[phase];
            }



        apr_ssize_t numPhaseRules(Phases::Phase phase) const
            {
                return phaseRulesIdxs(phase)->size();
            }



    private:
        Logger *logger_;
        const char *baseDir_;
        StrVec configFiles_;
        bool ctorError_;
        apr_ssize_t maxLookups_;
        LuaManager *luaManager_;
        StrPtrMap<CondModule *> condModulesMap_;
        StrSmplMap<bool> condUsageMap_;
        PtrVec<FiltCond *> filtConds_;
        PtrVec<Rule *> rules_;
        BlobVec commonPreActions_;
        StrVec defnScripts_;
        StrVec defnScriptFiles_;
        bool errHandlerScriptUsed_;
        bool errHandlerScriptFileUsed_;
        const Blob *errHandlerChunk_;
        SmplVec<bool> phaseUsageVec_;
        SmplVec<bool> condPhaseUsageVec_;
        SmplVec<bool> actionPhaseUsageVec_;
        PtrVec<SizeVec *> phaseRulesIdxs_;



        Config(const Config &from)
            : PoolAllocated(from),
              logger_(0),
              baseDir_(0),
              configFiles_(0),
              ctorError_(true),
              maxLookups_(0),
              luaManager_(0),
              condModulesMap_(0),
              condUsageMap_(0),
              filtConds_(0),
              rules_(0),
              commonPreActions_(0),
              defnScripts_(0),
              defnScriptFiles_(0),
              errHandlerScriptUsed_(false),
              errHandlerScriptFileUsed_(false),
              errHandlerChunk_(0),
              phaseUsageVec_(0),
              condPhaseUsageVec_(0),
              actionPhaseUsageVec_(0),
              phaseRulesIdxs_(0)
            {
                // this method is private and should not be used
            }



        Config& operator=(const Config& that)
            {
                // this method is private and should not be used
                PoolAllocated::operator=(that);
                return *this;
            }



        friend StrBuffer& operator<<(StrBuffer& sb, const Config& c);
    };

}


#endif // RUM_CONFIG_H
