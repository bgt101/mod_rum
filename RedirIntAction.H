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



#ifndef RUM_REDIRINTACTION_H
#define RUM_REDIRINTACTION_H


#include "Action.H"

#include "apr_strings.h"



namespace rum
{

    class RedirIntAction : public Action
    {
    public:
        RedirIntAction(apr_pool_t *p, Logger *l, Phases::Phase phase__,
                       const char *uri)
            : Action(p, l, phase__),
              uri_(apr_pstrdup(pool(), uri))
            {
                RUM_PTRC_ACTION(pool(), "RedirIntAction::RedirIntAction("
                                "apr_pool_t *p, const char *uri)");
            }



        virtual ~RedirIntAction()
            {
                RUM_PTRC_ACTION(pool(), "RedirIntAction::~RedirIntAction()");
            }



        virtual apr_status_t run(ActionCtx *) const;



        virtual const char *actionName() const
            {
                return actionName_;
            }



        virtual StrBuffer& write(StrBuffer& sb) const;



    protected:
        RedirIntAction(const RedirIntAction& from)
            : Action(from),
              uri_(0)
            { }



    private:
        const char *uri_;
        static const char *actionName_;



        RedirIntAction& operator=(const RedirIntAction& that)
            {
                // this method is private and should not be used
                Action::operator=(that);
                return *this;
            }

    };

}


#endif // RUM_REDIRINTACTION_H
