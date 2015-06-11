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



#include "string.h"
#include "Phases.H"



static const char *InvalidPhaseStr = "invalid_phase";
static const char *TranslateNamePreRewriteStr = "translate_name:pre_rewrite";
static const char *TranslateNameStr = "translate_name";
static const char *MapToStorageStr = "map_to_storage";
static const char *HeaderParserStr = "header_parser";
static const char *AccessCheckerStr = "access_checker";
static const char *CheckUserIdStr = "check_user_id";
static const char *AuthCheckerStr = "auth_checker";
static const char *TypeCheckerStr = "type_checker";
static const char *FixupsStr = "fixups";
static const char *InsertFilterStr = "insert_filter";
static const char *OutputFilterStr = "output_filter";
static const char *HandlerStr = "handler";
static const char *LogTransactionStr = "log_transaction";



namespace rum
{
    Phases::Phase Phases::str2enum(const char *s)
    {
        Phase phase;

        if (strcmp(s, TranslateNamePreRewriteStr) == 0)
        {
            phase = TranslateNamePreRewrite;
        }
        else if (strcmp(s, TranslateNameStr) == 0)
        {
            phase = TranslateName;
        }
        else if (strcmp(s, MapToStorageStr) == 0)
        {
            phase = MapToStorage;
        }
        else if (strcmp(s, HeaderParserStr) == 0)
        {
            phase = HeaderParser;
        }
        else if (strcmp(s, AccessCheckerStr) == 0)
        {
            phase = AccessChecker;
        }
        else if (strcmp(s, CheckUserIdStr) == 0)
        {
            phase = CheckUserId;
        }
        else if (strcmp(s, AuthCheckerStr) == 0)
        {
            phase = AuthChecker;
        }
        else if (strcmp(s, TypeCheckerStr) == 0)
        {
            phase = TypeChecker;
        }
        else if (strcmp(s, FixupsStr) == 0)
        {
            phase = Fixups;
        }
        else if (strcmp(s, InsertFilterStr) == 0)
        {
            phase = InsertFilter;
        }
        else if (strcmp(s, OutputFilterStr) == 0)
        {
            phase = OutputFilter;
        }
        else if (strcmp(s, HandlerStr) == 0)
        {
            phase = Handler;
        }
        else if (strcmp(s, LogTransactionStr) == 0)
        {
            phase = LogTransaction;
        }
        else
        {
            phase = InvalidPhase;
        }

        return phase;
    }



    const char *Phases::enum2str(Phase phase__)
    {
        const char *s;

        switch (phase__)
        {
        case TranslateNamePreRewrite:
            s = TranslateNamePreRewriteStr;
            break;
        case TranslateName:
            s = TranslateNameStr;
            break;
        case MapToStorage:
            s = MapToStorageStr;
            break;
        case HeaderParser:
            s = HeaderParserStr;
            break;
        case AccessChecker:
            s = AccessCheckerStr;
            break;
        case CheckUserId:
            s = CheckUserIdStr;
            break;
        case AuthChecker:
            s = AuthCheckerStr;
            break;
        case TypeChecker:
            s = TypeCheckerStr;
            break;
        case Fixups:
            s = FixupsStr;
            break;
        case InsertFilter:
            s = InsertFilterStr;
            break;
        case OutputFilter:
            s = OutputFilterStr;
            break;
        case Handler:
            s = HandlerStr;
            break;
        case LogTransaction:
            s = LogTransactionStr;
            break;
        default:
            s = InvalidPhaseStr;
        }

        return s;
    }
}
