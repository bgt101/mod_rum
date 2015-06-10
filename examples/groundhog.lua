-- Copyright 2015 CBS Interactive Inc.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--      http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
--
-- CBS Interactive accepts contributions to software products and free
-- and open-source projects owned, licensed, managed, or maintained by
-- CBS Interactive submitted under the terms of the CBS Interactive
-- Contribution License Agreement (the "Contribution Agreement"); you may
-- not submit software to CBS Interactive for inclusion in a CBS
-- Interactive product or project unless you agree to the terms of the
-- CBS Interactive Contribution License Agreement or have executed a
-- separate agreement with CBS Interactive governing the use of such
-- submission. A copy of the Contribution Agreement should have been
-- included with the software. You may also obtain a copy of the
-- Contribution Agreement at
-- http://www.cbsinteractive.com/cbs-interactive-software-grant-and-contribution-license-agreement/.



local rum_core = rum.core
rum_core.log_info("BEG groundhog.lua")
rum_core.set_content_type("text/plain")
rum_core.puts("Good morning Punxsutawney Phil!")
rum_core.log_info("END groundhog.lua")
