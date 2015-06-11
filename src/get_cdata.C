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



/*
 * rum_xml_get_cdata is a copy of dav_xml_get_cdata from:
 * httpd-2.2.8/modules/dav/main/util.c
 *
 * made fix to broken trailing whitespace trimming (do before leading ws trim)
 *
 */


#include "apr_xml.h"
#include "apr_lib.h"


/* gather up all the CDATA into a single string */
const char * rum_xml_get_cdata(const apr_xml_elem *elem, apr_pool_t *pool,
                               int strip_white)
{
    apr_size_t len = 0;
    apr_text *scan;
    const apr_xml_elem *child;
    char *cdata;
    char *s;
    apr_size_t tlen;
    const char *found_text = NULL; /* initialize to avoid gcc warning */
    int found_count = 0;

    for (scan = elem->first_cdata.first; scan != NULL; scan = scan->next) {
        found_text = scan->text;
        ++found_count;
        len += strlen(found_text);
    }

    for (child = elem->first_child; child != NULL; child = child->next) {
        for (scan = child->following_cdata.first;
             scan != NULL;
             scan = scan->next) {
            found_text = scan->text;
            ++found_count;
            len += strlen(found_text);
        }
    }

    /* some fast-path cases:
     * 1) zero-length cdata
     * 2) a single piece of cdata with no whitespace to strip
     */
    if (len == 0)
        return "";
    if (found_count == 1) {
        if (!strip_white
            || (!apr_isspace(*found_text)
                && !apr_isspace(found_text[len - 1])))
            return found_text;
    }

    cdata = s = (char *)apr_palloc(pool, len + 1);

    for (scan = elem->first_cdata.first; scan != NULL; scan = scan->next) {
        tlen = strlen(scan->text);
        memcpy(s, scan->text, tlen);
        s += tlen;
    }

    for (child = elem->first_child; child != NULL; child = child->next) {
        for (scan = child->following_cdata.first;
             scan != NULL;
             scan = scan->next) {
            tlen = strlen(scan->text);
            memcpy(s, scan->text, tlen);
            s += tlen;
        }
    }

    *s = '\0';

    if (strip_white) {
        /* trim trailing whitespace */
        while (len-- > 0 && apr_isspace(cdata[len]))
            continue;
        cdata[len + 1] = '\0';

        /* trim leading whitespace */
        while (apr_isspace(*cdata))     /* assume: return false for '\0' */
            ++cdata;
    }

    return cdata;
}
