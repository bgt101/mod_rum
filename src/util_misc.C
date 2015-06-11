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



#include "util_misc.H"
#include "apr_file_info.h"



/* almost identical to ap_server_root_relative() */
char *rum_base_dir_relative(apr_pool_t *p, const char *base_dir,
                            const char *file)
{
    char *newpath = NULL;
    apr_status_t rv;
    rv = apr_filepath_merge(&newpath, base_dir, file,
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



/* not all platforms provide strchrnul() */
char *rum_strchrnul(const char *s, int c)
{
    char cc = c;
    while (*s && (*s != cc))
    {
        s++;
    }
    return const_cast<char *>(s);
}
