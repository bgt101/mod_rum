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



#include <httpd.h>
#include <http_log.h>
#include <http_main.h>
#include <http_core.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_request.h>

#include <apr.h>
#include <apr_lib.h>
#include <apr_strings.h>

#include <unistd.h>
#include <stdlib.h>
#include "rum_errno.H"
#include "Config.H"
#include "LuaManager.H"
#include "ReqCtx.H"
#include "ReqLogger.H"
#include "ServerLogger.H"
#include "util_misc.H"


/* ending sentinel of command_rec array */
#define RUM_AP_END_CMD {NULL, NULL, NULL, 0, static_cast<cmd_how>(0), NULL}



using namespace rum;



// forward declarations
static void *create_rum_server_config(apr_pool_t *p, server_rec *s);
static void register_hooks(apr_pool_t *p);
extern const command_rec rum_cmds[];



#if RUM_AP22
module AP_MODULE_DECLARE_DATA rum_module =
#else
AP_DECLARE_MODULE(rum) =
#endif
{
    STANDARD20_MODULE_STUFF,
    NULL,                     /* dir config creater */
    NULL,                     /* dir merger --- default is to override */
    create_rum_server_config, /* server config */
    NULL,                     /* merge server config */
    rum_cmds,                 /* command apr_table_t */
    register_hooks            /* register hooks */
};



struct rum_server_config
{
    StrVec *configFiles;
    const char *baseDir;
    Config *conf;
    apr_ssize_t maxLookups;
    apr_ssize_t fullLuaGC;
    apr_ssize_t useMain;
    apr_ssize_t initLuaStatePoolSize;
    server_rec *server;
};



static void *create_rum_server_config(apr_pool_t *p, server_rec *s)
{
    rum_server_config *sc =
        static_cast<rum_server_config *>
        (apr_pcalloc(p, sizeof(rum_server_config)));

    sc->server = s;

    sc->configFiles = new (p) StrVec(0);
    sc->configFiles->destroyWithPool();

    return sc;
}



static const char *cmd_config_file(cmd_parms *cmd, void *mc, const char *a1)
{
    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(cmd->server->module_config, &rum_module));

    sc->configFiles->push_back(a1);

    return NULL;
}



static const char *cmd_max_lookups(cmd_parms *cmd, void *mc, const char *a1)
{
    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(cmd->server->module_config, &rum_module));

    sc->maxLookups = atol(a1);

    return NULL;
}



static const char *cmd_base_dir(cmd_parms *cmd, void *mc, const char *a1)
{
    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(cmd->server->module_config, &rum_module));

    sc->baseDir = a1;

    return NULL;
}



static const char *cmd_initial_lua_state_pool_size(cmd_parms *cmd,
                                                   void *mc, const char *a1)
{
    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(cmd->server->module_config, &rum_module));

    sc->initLuaStatePoolSize = atoi(a1);

    return NULL;
}



static const char *cmd_full_lua_gc(cmd_parms *cmd, void *mc, int on)
{
    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(cmd->server->module_config, &rum_module));

    sc->fullLuaGC = on;

    return NULL;
}



static const char *cmd_use_main(cmd_parms *cmd, void *mc, int on)
{
    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(cmd->server->module_config, &rum_module));

    sc->useMain = on;

    return NULL;
}



static ReqCtx *mk_ReqCtx(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG mk_ReqCtx");

    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(r->server->module_config, &rum_module));

    ReqCtx *reqCtx = NULL;

    if ((sc->server == r->server) && sc->conf)
    {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                      "RUM processing request, uri: %s", r->uri);

        rum::ReqLogger *logger =
            new (r->pool) rum::ReqLogger(0,
#if RUM_AP22
                                         r->server->loglevel,
#else
                                         r->log->level,
                                         APLOG_MODULE_INDEX,
#endif
                                         r);
        logger->destroyWithPool();

        reqCtx = new (r->pool) ReqCtx(0, logger, r, *sc->conf);
        reqCtx->destroyWithPool();
        ap_set_module_config(r->request_config, &rum_module, reqCtx);
    }
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                      "RUM ignoring request, uri: %s", r->uri);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END mk_ReqCtx");

    return reqCtx;
}



static int rum_translate_name_pre_rewrite(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG translate_name_pre_rewrite hook");

    int ret = DECLINED;

    ReqCtx *reqCtx = mk_ReqCtx(r);

    if (reqCtx)
    {
        Phases::Phase phase = Phases::TranslateNamePreRewrite;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);

        if (r->filename &&
            ((strncmp(r->filename, "redirect:", 9) == 0) ||
             (r->proxyreq && (strncmp(r->filename, "proxy:", 6) == 0))))
        {
            ret = OK;
        }
        else if (ap_is_HTTP_REDIRECT(r->status))
        {
            ret = r->status;
        }
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END translate_name_pre_rewrite hook; status: %d", ret);

    return ret;
}



static int rum_translate_name(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG translate_name hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::TranslateName;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);

        if (r->filename &&
            ((strncmp(r->filename, "redirect:", 9) == 0) ||
             (r->proxyreq && (strncmp(r->filename, "proxy:", 6) == 0))))
        {
            ret = OK;
        }
        else if (ap_is_HTTP_REDIRECT(r->status))
        {
            ret = r->status;
        }
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END translate_name hook; status: %d", ret);

    return ret;
}



static int rum_map_to_storage(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG map_to_storage hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::MapToStorage;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);

        if (r->filename &&
            ((strncmp(r->filename, "redirect:", 9) == 0) ||
             (r->proxyreq && (strncmp(r->filename, "proxy:", 6) == 0))))
        {
            ret = OK;
        }
        else if (ap_is_HTTP_REDIRECT(r->status))
        {
            ret = r->status;
        }
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END map_to_storage; status: %d", ret);

    return ret;
}



static int rum_header_parser(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG header_parser hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::HeaderParser;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END header_parser; status: %d", ret);

    return ret;
}



static int rum_access_checker(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG access_checker hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::AccessChecker;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END access_checker; status: %d", ret);

    return ret;
}



static int rum_check_user_id(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG check_user_id hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::CheckUserId;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END check_user_id; status: %d", ret);

    return ret;
}



static int rum_auth_checker(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG auth_checker hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::AuthChecker;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END auth_checker; status: %d", ret);

    return ret;
}



static int rum_type_checker(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG type_checker hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::TypeChecker;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END type_checker; status: %d", ret);

    return ret;
}



static int rum_fixups(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG fixups hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::Fixups;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END fixups; status: %d", ret);

    return ret;
}



static void rum_insert_filter(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG insert_filter hook");

    ap_add_output_filter("RUM_OUTPUT", NULL, r, r->connection);

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::InsertFilter;
        reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END insert_filter");
}



static int rum_log_transaction(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG log_transaction hook");

    int ret = DECLINED;

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::LogTransaction;
        ret = reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(r->server->module_config, &rum_module));

    if (sc->fullLuaGC)
    {
        if (reqCtx)
        {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                          "calling Lua garbage collector");
            reqCtx->fullLuaGC();
        }
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END log_transaction; status: %d", ret);

    return ret;
}



static int rum_state_handler(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "handling rum-state");
    ap_set_content_type(r, "text/plain");

    rum_server_config *sc =
        static_cast<rum_server_config *>
        (ap_get_module_config(r->server->module_config, &rum_module));

    StrBuffer buf(r->pool);
    if ((sc->server == r->server) && sc->conf)
    {
        buf << "RUM state" << nl << nl << *(sc->conf) << nl;
    }
    else
    {
        buf << "RUM is not enabled for this server" << nl;
    }

    ap_rputs(buf, r);

    return OK;
}



static int rum_int_redirect_handler(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "pre rum-int-redirect: %s", r->filename);

    // assume filename begins with "redirect:"
    const char *new_uri = apr_pstrcat(r->pool, r->filename + 9,
                                      r->args ? "?" : NULL, r->args, NULL);

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "rum-int-redirect: %s", new_uri);

    // now do the internal redirect
    ap_internal_redirect(new_uri, r);

    return OK;
}



static int rum_handler(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "handling rum");

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "r->filename: %s", r->filename);

    if (strcmp(r->handler, "rum-state") == 0)
    {
        return rum_state_handler(r);
    }
    else if (strcmp(r->handler, "rum-int-redirect") == 0)
    {
        return rum_int_redirect_handler(r);
    }

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));
    if (reqCtx && reqCtx->handleReq())
    {
        return OK;
    }

    return DECLINED;
}



static int rum_post_config(apr_pool_t *p, apr_pool_t *pLog, apr_pool_t *pTmp,
                           server_rec *s)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s,
                 "BEG post_config hook");


    // since creating the RUM Config instance can be time consuming,
    // do it only once
    void *data;
    const char *userdata_key = "rum_post_config_only_once_key";
    apr_pool_userdata_get(&data, userdata_key, s->process->pool);
    if (!data) {
        apr_pool_userdata_set((const void *)1, userdata_key,
                              apr_pool_cleanup_null, s->process->pool);
        return OK;
    }


    rum_server_config *sc_main = NULL;
    server_rec *s2;
    for (s2 = s; s2; s2 = s2->next)
    {
        rum_server_config *sc2 =
            static_cast<rum_server_config *>
            (ap_get_module_config(s2->module_config, &rum_module));

        // is this the main server
        bool isMain = (s == s2);

        if (isMain)
        {
            sc_main = sc2;
        }

        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s2,
                     "RUM: processing "
                     "server: %s, defined at %s:%d",
                     s2->server_hostname, s2->defn_name,
                     s2->defn_line_number);

        if (sc2->useMain)
        {
            if (isMain)
            {
                ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                             "RumUseMain not allowed for main server");
                return HTTP_INTERNAL_SERVER_ERROR;
            }
            else
            {
                if (sc2->configFiles->size() > 0)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumConfigFile directive not allowed here "
                                 "because RumUseMain specified for virtual "
                                 "server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                if (sc2->baseDir)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumBaseDir directive not allowed here "
                                 "because RumUseMain specified for virtual "
                                 "server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                if (sc2->maxLookups)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumMaxLookups directive not allowed here "
                                 "because RumUseMain specified for virtual "
                                 "server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                if (sc2->fullLuaGC)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumFullLuaGC directive not allowed here "
                                 "because RumUseMain specified for virtual "
                                 "server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                if (sc2->initLuaStatePoolSize)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumInitialLuaStatePoolSize "
                                 "directive not allowed here "
                                 "because RumUseMain specified for virtual "
                                 "server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }

                // refer to the main server's RUM Config
                if (!sc_main->conf)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumConfigFile directive not specified "
                                 "for main server but "
                                 "RumUseMain specified for virtual "
                                 "server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s2,
                             "using main RUM config for "
                             "server: %s, defined at %s:%d",
                             s2->server_hostname, s2->defn_name,
                             s2->defn_line_number);
                sc2->conf = sc_main->conf;
            }
        }
        else
        {
            // RumUseMain not specified

            if ((sc2->server == s2) && (sc2->configFiles->size() > 0))
            {
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s2,
                             "configuring RUM for "
                             "server: %s, defined at %s:%d",
                             s2->server_hostname, s2->defn_name,
                             s2->defn_line_number);

                rum::ServerLogger *logger =
                    new (p) rum::ServerLogger(0,
#if RUM_AP22
                                              s2->loglevel,
#else
                                              s2->log.level,
                                              APLOG_MODULE_INDEX,
#endif
                                              s2);
                logger->destroyWithPool();

                const char *baseDir;
                if (sc2->baseDir)
                {
                    const char *rootpath;
                    const char *filepath = sc2->baseDir;
                    if (apr_filepath_root(&rootpath, &filepath,
                                          APR_FILEPATH_TRUENAME, pTmp) ==
                        APR_SUCCESS)
                    {
                        baseDir = sc2->baseDir;
                    }
                    else
                    {
                        // relative RumBaseDir is relative to server root
                        baseDir = rum_base_dir_relative(pTmp,
                                                        ap_server_root,
                                                        sc2->baseDir);
                    }

                }
                else
                {
                    baseDir = ap_server_root;
                }

                apr_ssize_t maxLookups =
                    sc2->maxLookups ? sc2->maxLookups : 10;

                LuaManager *luaManager =
                    new (p, PoolAllocated::UseSubPools)
                    LuaManager(0, logger, baseDir);
                luaManager->destroyWithPool();

                sc2->conf = new (p) Config(0, logger, pTmp, baseDir,
                                           *sc2->configFiles, maxLookups,
                                           luaManager);
                sc2->conf->destroyWithPool();

                if (sc2->conf->ctorError())
                {
                    return HTTP_INTERNAL_SERVER_ERROR;
                }

                // pre-initialize some Lua states
                if (sc2->initLuaStatePoolSize >= 0)
                {
                    apr_size_t ilsps = sc2->initLuaStatePoolSize;

                    // initialize at least one state so that run-time errors
                    // in the definitions section are caught
                    apr_size_t minInitStates = ilsps ? ilsps : 1;

                    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s2,
                                 "initializing %d Lua states for "
                                 "server: %s, defined at %s:%d",
                                 static_cast<int>(minInitStates),
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);

                    PtrVec<LuaManager::LuaRef *> *luaRefVec =
                        new (p, PoolAllocated::UseSubPools)
                        PtrVec<LuaManager::LuaRef *>(0, minInitStates);
                    apr_size_t i;
                    for (i = 0; i < minInitStates; i++)
                    {
                        LuaManager::LuaRef *luaRef =
                            new (p) LuaManager::LuaRef(p, luaManager);
                        if (! luaRef->isValid())
                        {
                            ap_log_error(APLOG_MARK, APLOG_ERR,
                                         0, s2,
                                         "failed to initialize Lua state for "
                                         "server: %s, defined at %s:%d",
                                         s2->server_hostname, s2->defn_name,
                                         s2->defn_line_number);
                            return HTTP_INTERNAL_SERVER_ERROR;
                        }
                        luaRefVec->push_back(luaRef);
                    }
                    delete luaRefVec;
                }
            }
            else
            {
                if ((sc2->server == s2) && sc2->baseDir)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumBaseDir "
                                 "directive not allowed here "
                                 "because RumConfigFile not specified "
                                 "for server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                if ((sc2->server == s2) && sc2->maxLookups)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumMaxLookups "
                                 "directive not allowed here "
                                 "because RumConfigFile not specified "
                                 "for server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                if ((sc2->server == s2) && sc2->fullLuaGC)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumFullLuaGC "
                                 "directive not allowed here "
                                 "because RumConfigFile not specified "
                                 "for server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }
                if ((sc2->server == s2) && sc2->initLuaStatePoolSize)
                {
                    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s2,
                                 "RumInitialLuaStatePoolSize "
                                 "directive not allowed here "
                                 "because RumConfigFile not specified "
                                 "for server: %s, defined at %s:%d",
                                 s2->server_hostname, s2->defn_name,
                                 s2->defn_line_number);
                    return HTTP_INTERNAL_SERVER_ERROR;
                }

                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s2,
                             "RUM not configured for "
                             "server: %s, defined at %s:%d",
                             s2->server_hostname, s2->defn_name,
                             s2->defn_line_number);
            }
        }
    }

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s,
                 "END post_config");

    return OK;
}



static apr_status_t rum_output_filter(ap_filter_t *f, apr_bucket_brigade *in)
{
    request_rec *r = f->r;

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "BEG output_filter hook");

    ReqCtx *reqCtx =
        static_cast<ReqCtx *>
        (ap_get_module_config(r->request_config, &rum_module));

    if (reqCtx)
    {
        Phases::Phase phase = Phases::OutputFilter;
        reqCtx->config().lookupAndRun(reqCtx, phase);
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "END output_filter hook");

    ap_remove_output_filter(f);
    return ap_pass_brigade(f->next,in);
}



static void register_hooks(apr_pool_t *p)
{
    // force translate name before mod_rewrite
    static const char *const aszSuccTN[] = {"mod_rewrite.c", NULL};

    ap_hook_post_config(rum_post_config, NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_translate_name(rum_translate_name_pre_rewrite,
                           NULL, aszSuccTN, APR_HOOK_FIRST);

    ap_hook_translate_name(rum_translate_name,
                           NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_map_to_storage(rum_map_to_storage,
                           NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_header_parser(rum_header_parser,
                          NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_access_checker(rum_access_checker,
                           NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_check_user_id(rum_check_user_id,
                          NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_auth_checker(rum_auth_checker,
                         NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_type_checker(rum_type_checker,
                         NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_fixups(rum_fixups,
                   NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_insert_filter(rum_insert_filter,
                          NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_handler(rum_handler, NULL, aszSuccTN, APR_HOOK_MIDDLE);

    ap_hook_log_transaction(rum_log_transaction, NULL, NULL, APR_HOOK_MIDDLE);

    ap_register_output_filter("RUM_OUTPUT", rum_output_filter, NULL,
                              AP_FTYPE_CONTENT_SET);
}



const command_rec rum_cmds[] =
{
    AP_INIT_TAKE1("RumConfigFile",
                  reinterpret_cast<cmd_func>(cmd_config_file),
                  NULL,
                  RSRC_CONF,
                  "RUM configuration file"),
    AP_INIT_TAKE1("RumMaxLookups",
                  reinterpret_cast<cmd_func>(cmd_max_lookups),
                  NULL,
                  RSRC_CONF,
                  "RUM maximum number of recursive lookups"),
    AP_INIT_TAKE1("RumBaseDir",
                  reinterpret_cast<cmd_func>(cmd_base_dir),
                  NULL,
                  RSRC_CONF,
                  "Base directory for relative paths used by RUM"),
    AP_INIT_TAKE1("RumInitialLuaStatePoolSize",
                  reinterpret_cast<cmd_func>(cmd_initial_lua_state_pool_size),
                  NULL,
                  RSRC_CONF,
                  "Initial size of Lua state pool"),
    AP_INIT_FLAG("RumFullLuaGC",
                 reinterpret_cast<cmd_func>(cmd_full_lua_gc),
                 NULL,
                 RSRC_CONF,
                 "Do full garbage collection in Lua after each request"),
    AP_INIT_FLAG("RumUseMain",
                 reinterpret_cast<cmd_func>(cmd_use_main),
                 NULL,
                 RSRC_CONF,
                 "Use main server's RUM configuration in virtual server"),
    RUM_AP_END_CMD
};
