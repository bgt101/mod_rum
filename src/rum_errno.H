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



#ifndef RUM_ERRNO_H
#define RUM_ERRNO_H


#include "apr_errno.h"



// status codes used in RUM
#define RUM_LAST_ACTION (APR_OS_START_USERERR + 1)
#define RUM_RELOOKUP (APR_OS_START_USERERR + 2)
#define RUM_DEFAULT (APR_OS_START_USERERR + 3)
#define RUM_OK (APR_OS_START_USERERR + 4)
#define RUM_DELAYED_OK (APR_OS_START_USERERR + 5)
#define RUM_DECLINED (APR_OS_START_USERERR + 6)



#endif // RUM_ERRNO_H
