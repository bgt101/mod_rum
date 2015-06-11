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



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include "httpd.h"
#include "apr_signal.h"
#include "apr_time.h"
#include "apr_tables.h"
#include "apr_xml.h"
#include "rum_errno.H"
#include "debug.H"
#include "Config.H"
#include "LuaManager.H"
#include "ReqCtx.H"
#include "StrBuffer.H"
#include "FStreamLogger.H"
#include "get_cdata.H"



using namespace rum;



char *parseXMLStr(apr_pool_t *pool, Logger *logger, apr_xml_elem *element)
{
    char *s = apr_pstrdup(pool, rum_xml_get_cdata(element, pool, 1));
    RUM_LOG_MSG(logger, APLOG_DEBUG, "value: " << s);
    return s;
}



int parseXMLInt(apr_pool_t *pool, Logger *logger, apr_xml_elem *element)
{
    const char *s = rum_xml_get_cdata(element, pool, 1);
    int i = atoi(s);
    RUM_LOG_MSG(logger, APLOG_DEBUG, "value: " << i);
    return i;
}



void parseXMLTable(apr_pool_t *pool, Logger *logger, apr_xml_elem *element,
                   apr_table_t *table)
{
    const char *name = 0;
    const char *value = 0;

    apr_xml_elem *childElem;
    for (childElem = element->first_child;
         childElem != NULL;
         childElem = childElem->next) {
        if (strcmp(childElem->name, "Name") == 0)
        {
            name = apr_pstrdup(pool, rum_xml_get_cdata(childElem, pool, 1));
            RUM_LOG_MSG(logger, APLOG_DEBUG, "name: " << name);
        }
        else if (strcmp(childElem->name, "Value") == 0)
        {
            value = apr_pstrdup(pool, rum_xml_get_cdata(childElem, pool, 1));
            RUM_LOG_MSG(logger, APLOG_DEBUG, "value: " << value);
        }
        else
        {
            RUM_LOG_MSG(logger, APLOG_ERR, "unrecognized element in "
                        << element->name << ": " << childElem->name);
            continue;
        }
    }

    if (name && value)
    {
        apr_table_addn(table, name, value);
    }
    else
    {
        RUM_LOG_MSG(logger, APLOG_ERR, "missing \"Name\" and/or \"Value\" "
                    << "elements in " << element->name);
    }
}



apr_status_t parseRequestXML(apr_pool_t *pool, ReqCtx *reqCtx,
                             const char *xmlReqFile)
{
    // load the XML and then parse it
    apr_file_t *fd;
    apr_status_t status;
    apr_xml_parser *parser;
    apr_xml_doc *xmlDoc;
    request_rec *r = reqCtx->req();

    status = apr_file_open(&fd, xmlReqFile, (APR_READ | APR_BUFFERED),
                           APR_OS_DEFAULT, pool);
    if (status != APR_SUCCESS)
    {
        RUM_LOG_MSG(reqCtx->logger(), APLOG_ERR,
                    "unable to open XML request file: " << xmlReqFile);
        return status;
    }

    apr_xml_parse_file(pool, &parser, &xmlDoc, fd, 2000);
    if (parser != NULL)
    {
        RUM_LOG_MSG(reqCtx->logger(), APLOG_ERR,
                    "unable to parse XML request file: " << xmlReqFile);
        return APR_EGENERAL;
    }


    if (strcmp(xmlDoc->root->name, "Request") != 0)
    {
        RUM_LOG_MSG(reqCtx->logger(), APLOG_ERR,
                    "unrecognized root name in XML : "
                    << xmlDoc->root->name);
        return APR_EGENERAL;
    }


    // parse XML elements
    apr_xml_elem *element;
    for (element = xmlDoc->root->first_child;
         element != NULL;
         element = element->next) {

        RUM_LOG_MSG(reqCtx->logger(), APLOG_DEBUG, "element: <"
                    << element->name << ">");

        if (strcmp(element->name, "HostName") == 0)
        {
            r->hostname = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "URI") == 0)
        {
            r->uri = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "Status") == 0)
        {
            r->status = parseXMLInt(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "Main") == 0)
        {
            r->main =
                reinterpret_cast<request_rec *>
                (parseXMLInt(pool, reqCtx->logger(), element));
        }
        else if (strcmp(element->name, "Prev") == 0)
        {
            r->prev =
                reinterpret_cast<request_rec *>
                (parseXMLInt(pool, reqCtx->logger(), element));
        }
        else if (strcmp(element->name, "Method") == 0)
        {
            r->method = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "Protocol") == 0)
        {
            r->protocol = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "ContentType") == 0)
        {
            r->content_type = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "ContentEncoding") == 0)
        {
            r->content_encoding = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "User") == 0)
        {
            r->user = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "AuthType") == 0)
        { 
            r->ap_auth_type = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "UnparsedURI") == 0)
        {
            r->unparsed_uri = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "FileName") == 0)
        {
            r->filename = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "CanonicalFileName") == 0)
        {
            r->canonical_filename = parseXMLStr(pool, reqCtx->logger(),
                                                element);
        }
        else if (strcmp(element->name, "PathInfo") == 0)
        {
            r->path_info = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "Args") == 0)
        {
            r->args = parseXMLStr(pool, reqCtx->logger(), element);
        }
        else if (strcmp(element->name, "HeadersIn") == 0)
        {
            parseXMLTable(pool, reqCtx->logger(), element, r->headers_in);
        }
        else if (strcmp(element->name, "HeadersOut") == 0)
        {
            parseXMLTable(pool, reqCtx->logger(), element, r->headers_out);
        }
        else if (strcmp(element->name, "ErrHeadersOut") == 0)
        {
            parseXMLTable(pool, reqCtx->logger(), element, r->err_headers_out);
        }
        else if (strcmp(element->name, "Env") == 0)
        {
            parseXMLTable(pool, reqCtx->logger(), element, r->subprocess_env);
        }
        else if (strcmp(element->name, "Notes") == 0)
        {
            parseXMLTable(pool, reqCtx->logger(), element, r->notes);
        }
        else
        {
            RUM_LOG_MSG(reqCtx->logger(), APLOG_ERR, "unrecognized XML in <"
                        << xmlDoc->root->name << ">: <"
                        << element->name << ">");
            continue;
        }
    }

    return APR_SUCCESS;
}



apr_status_t fillRequest(apr_pool_t *pool, ReqCtx *reqCtx,
                         const char *xmlReqFile,
                         const char *hostname__, const char *uri__,
                         const char *args__)
{
    RUM_PTRC_MSG(pool, "BEG initialize request");

    request_rec *r = reqCtx->req();
    r->pool = pool;

    // set the connection's members -- ideally from the XML in the future
    r->connection->pool = pool;
#if RUM_AP22
    r->connection->remote_ip = apr_pstrdup(r->pool, "11.22.33.44");
#else
    r->connection->client_ip = apr_pstrdup(r->pool, "11.22.33.44");
#endif
    r->connection->local_addr = static_cast<apr_sockaddr_t *>
                                (apr_pcalloc(pool, sizeof(apr_sockaddr_t)));
    r->connection->local_addr->port = 8000;

    // set the server_rec -- ideally from the XML in the future
    r->server = static_cast<server_rec *>
                (apr_pcalloc(pool, sizeof(server_rec)));

    // create the request's tables
    r->headers_in = apr_table_make(pool, 0);
    r->headers_out = apr_table_make(pool, 0);
    r->err_headers_out = apr_table_make(pool, 0);
    r->subprocess_env = apr_table_make(pool, 0);
    r->notes = apr_table_make(pool, 0);

    // fill request from request.xml if provided
    if (xmlReqFile)
    {
        parseRequestXML(pool, reqCtx, xmlReqFile);
    }

    // overrides
    if (hostname__)
    {
        r->hostname = apr_pstrdup(pool, hostname__);
    }

    if (uri__)
    {
        r->uri = apr_pstrdup(pool, uri__);
    }

    if (args__)
    {
        r->args = apr_pstrdup(pool, args__);
    }


    if (!r->uri)
    {
        r->uri = apr_pstrdup(pool, "/");
    }

    if (!r->hostname)
    {
        r->hostname = "news.cnet.com";
    }


    RUM_PTRC_MSG(pool, "END initialize request");

    return APR_SUCCESS;
}



int main(int argc, char *argv[])
{
    const char *host = 0;
    const char *uri = 0;
    const char *args = 0;
    const char *xmlReqFile = 0;
    int c;
    int logLevel = APLOG_NOTICE;
    apr_ssize_t maxLookups = 10;
    long loopCount = 1;
    const char *usage = "Usage: %s [-b base-dir] "
                        "-c confxml [-r reqxml] [-h host] [-u uri] [-a args] "
                        "[-l log-level] [-m max-lookups] [-L loop-count]\n";


    // RUM base directory
    const char *baseDir = ".";


    // start using APR
    apr_initialize();


    // allow broken pipes to be handled in Lua
    apr_signal(SIGPIPE, SIG_IGN);


    // create server pool
    apr_pool_t *sPool;
    apr_pool_create(&sPool, NULL);
    RUM_PTRC_MSG(sPool, "main: sPool: " << sPool);

    StrVec *configFiles = new (sPool) StrVec(0);
    configFiles->destroyWithPool();



    opterr = 0;
    while ((c = getopt(argc, argv, "h:u:a:b:c:r:l:m:L:")) != -1)
    {
        switch (c)
        {
        case 'h':
            host = optarg;
            break;
        case 'u':
            uri = optarg;
            break;
        case 'a':
            args = optarg;
            break;
        case 'b':
            baseDir = optarg;
            break;
        case 'c':
            configFiles->push_back(optarg);
            break;
        case 'r':
            xmlReqFile = optarg;
            break;
        case 'l':
            logLevel = atoi(optarg);
            break;
        case 'm':
            maxLookups = atol(optarg);
            break;
        case 'L':
            loopCount = atol(optarg);
            break;
        default:
            fprintf(stderr, usage, basename(argv[0]));
            apr_terminate();
            exit(1);
        }
    }


    // rumconf.xml is required
    if (configFiles->size() == 0)
    {
        fprintf(stderr, usage, basename(argv[0]));
        apr_terminate();
        exit(2);
    }


    // create server logger
    FStreamLogger *sLogger =
        new (sPool) FStreamLogger(0, logLevel, stderr);
    sLogger->destroyWithPool();

    // create Lua manager
    LuaManager *luaManager = new (sPool, PoolAllocated::UseSubPools)
                             LuaManager(0, sLogger, baseDir);
    luaManager->destroyWithPool();

    // create temp pool
    apr_pool_t *tPool;
    apr_pool_create(&tPool, NULL);

    // create Config
    RUM_PTRC_MSG(sPool, "BEG initialize Config");
    Config *conf = new (sPool) Config(0, sLogger, tPool, baseDir, *configFiles,
                                      maxLookups, luaManager);

    // destroy temp pool
    apr_pool_destroy(tPool);

    conf->destroyWithPool();
    if (conf->ctorError())
    {
        RUM_LOG_MSG(sLogger, APLOG_ERR, "config error, exiting");
        apr_terminate();
        exit(3);
    }
    RUM_PTRC_MSG(sPool, "END initialize Config");


    // print out Config
    StrBuffer sb1(sPool);
    sb1 << "[" << static_cast<long>(apr_time_now()) << "] ";
    StrBuffer sb2(sPool);
    sb2 << "Config, line tag: " << sb1
        << setlinetag(sb1)
        << setindentwidth(4)
        << nl
        << *conf;
    RUM_PTRC_MSG(sPool, sb2);


    for (c = 0; c < loopCount; c++)
    {
        // create request pool
        apr_pool_t *rPool;
        apr_pool_create(&rPool, NULL);
        RUM_PTRC_MSG(rPool, "main: rPool: " << rPool);

        // create request logger
        FStreamLogger *rLogger =
            new (rPool) FStreamLogger(0, logLevel, stderr);
        rLogger->destroyWithPool();

        // create request
        request_rec *r =
            static_cast<request_rec *>
            (apr_pcalloc(rPool, sizeof(request_rec)));
        // create connection
        r->connection =
            static_cast<conn_rec *>(apr_pcalloc(rPool, sizeof(conn_rec)));
        // create request context
        ReqCtx *reqCtx = new (rPool) ReqCtx(0, rLogger, r, *conf);
        reqCtx->destroyWithPool();

        apr_status_t status = fillRequest(rPool, reqCtx, xmlReqFile,
                                          host, uri, args);
        if (status != APR_SUCCESS)
        {
            RUM_LOG_MSG(sLogger, APLOG_ERR, "filling request failed");
        }

        // perform the lookups and run actions for each phase
        apr_ssize_t n = Phases::numPhases();
        apr_ssize_t i;
        for (i = 0; i < n; i++)
        {
            Phases::Phase phase = static_cast<Phases::Phase>(i);
            if (conf->phaseUsage(phase))
            {
                RUM_LOG_MSG(sLogger, APLOG_INFO, "processing phase: "
                            << Phases::enum2str(phase));
                conf->lookupAndRun(reqCtx, phase);
            }
            else
            {
                RUM_LOG_MSG(sLogger, APLOG_INFO, "skipping phase: "
                            << Phases::enum2str(phase));
            }
        }

        // destroy request pool and therefore objects registered with it
        RUM_PTRC_MSG(rPool, "main: destroying rPool");
        apr_pool_destroy(rPool);
    }

    // we're done with APR
    RUM_STRC_MSG(32, "main: calling apr_terminate()");
    apr_terminate();


    // bye, bye
    RUM_STRC_MSG(32, "main: exiting");
    return 0;
}
