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



#include "http_core.h"
#include "http_protocol.h"
#include "http_request.h"
#include "httpd.h"
#include "apr_file_info.h"
#include <stdarg.h>
#include <stdio.h>



static void log_error_core(const char *file, int line, int level,
                           apr_status_t status, const server_rec *s,
                           const conn_rec *c,
                           const request_rec *r, apr_pool_t *pool,
                           const char *fmt, va_list args)
{
    vfprintf(stderr, fmt, args);
    fputs("\n", stderr);
}




extern "C" void ap_internal_redirect(const char *new_uri, request_rec *r)
{
}



#if RUM_AP22
extern "C" void ap_log_error(const char *file, int line,
                             int level,
#else
extern "C" void ap_log_error_(const char *file, int line,
                              int module_index, int level,
#endif
                             apr_status_t status, const server_rec *s,
                             const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_error_core(file, line, level, status, 0, 0, 0, 0, fmt, args);
    va_end(args);
}



#if RUM_AP22
extern "C" void ap_log_perror(const char *file, int line,
                               int level,
#else
extern "C" void ap_log_perror_(const char *file, int line,
                               int module_index, int level,
#endif
                              apr_status_t status, apr_pool_t *p,
                              const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_error_core(file, line, level, status, 0, 0, 0, 0, fmt, args);
    va_end(args);
}



#if RUM_AP22
extern "C" void ap_log_rerror(const char *file, int line, int level,
#else
extern "C" void ap_log_rerror_(const char *file, int line, int level,
#endif
                              apr_status_t status, const request_rec *r,
                              const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_error_core(file, line, level, status, 0, 0, 0, 0, fmt, args);
    va_end(args);
}



extern "C" void ap_log_cerror(const char *file, int line, int level,
                              apr_status_t status, const conn_rec *c,
                              const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_error_core(file, line, level, status, 0, 0, 0, 0, fmt, args);
    va_end(args);
}



extern "C" void ap_set_content_type(request_rec *r, const char *ct)
{
}



extern "C" int ap_rflush(request_rec *r)
{
    return 0;
}



#if RUM_AP22
extern "C" int ap_rputs(const char *str, request_rec *r)
{
    fputs(str, stdout);
    return 0;
}
#endif



extern "C" int ap_rwrite(const void *ptr, int nbyte, request_rec *r)
{
    fwrite(ptr, 1, nbyte, stdout);
    return 0;
}



extern "C" const char * ap_run_http_scheme(const request_rec *r)
{
    return "http";
}



extern "C" const char * ap_get_remote_logname(request_rec *r)
{
    return "bob";
}



extern "C" const char * ap_get_server_name(request_rec *r)
{
    return "some.where.com";
}



extern "C" apr_port_t ap_get_server_port(const request_rec *r)
{
    return 1234;
}



extern "C" const char * ap_default_type(request_rec *r)
{
    return "text/plain";
}



extern "C" int ap_is_initial_req(request_rec *r)
{
    return 1;
}



extern "C" const char *ap_server_root = ".";



extern "C" char *ap_server_root_relative(apr_pool_t *p, const char *file)
{
    char *newpath = NULL;
    apr_status_t rv;
#if 0
    char *ap_server_root;
    apr_filepath_get(&ap_server_root, 0, p);
#endif
    rv = apr_filepath_merge(&newpath, ap_server_root, file,
                            APR_FILEPATH_TRUENAME, p);
    if (newpath && (rv == APR_SUCCESS || APR_STATUS_IS_EPATHWILD(rv)
                    || APR_STATUS_IS_ENOENT(rv)
                    || APR_STATUS_IS_ENOTDIR(rv))) {
        return newpath;
    }
    else {
        return NULL;
    }
}
