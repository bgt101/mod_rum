# Copyright 2015 CBS Interactive Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
# CBS Interactive accepts contributions to software products and free
# and open-source projects owned, licensed, managed, or maintained by
# CBS Interactive submitted under the terms of the CBS Interactive
# Contribution License Agreement (the "Contribution Agreement"); you may
# not submit software to CBS Interactive for inclusion in a CBS
# Interactive product or project unless you agree to the terms of the
# CBS Interactive Contribution License Agreement or have executed a
# separate agreement with CBS Interactive governing the use of such
# submission. A copy of the Contribution Agreement should have been
# included with the software. You may also obtain a copy of the
# Contribution Agreement at
# http://www.cbsinteractive.com/cbs-interactive-software-grant-and-contribution-license-agreement/.



HTTPD_DIR = /usr/local
LUA_DIR = /usr/local
LUA_INC_DIR = $(LUA_DIR)/include
LUA_LIB_NAME = lua
LIBTOOL = $(HTTPD_DIR)/build-1/libtool
APXS = $(HTTPD_DIR)/bin/apxs
APR_CONFIG = /usr/local/bin/apr-1-config
APU_CONFIG = /usr/local/bin/apu-1-config
EXTRA_LDFLAGS = -Wl,--enable-new-dtags


CC = gcc
CXX = g++


# run "make clean" after changing compiler flags


#DEBUGFLAGS = -DRUM_TRACE
#DEBUGFLAGS += -DRUM_TRACE_ALL
#DEBUGFLAGS += -DRUM_TRACE_EXP
#DEBUGFLAGS += -DRUM_TRACE_MSG
#DEBUGFLAGS += -DRUM_TRACE_MAP
# DEBUGFLAGS += -DRUM_TRACE_POOL
DEBUGFLAGS += -DRUM_TRACE_COND
DEBUGFLAGS += -DRUM_TRACE_CONFIG
DEBUGFLAGS += -DRUM_TRACE_ACTION
# DEBUGFLAGS += -DRUM_TRACE_VECTOR
DEBUGFLAGS += -DRUM_TRACE_RULE
#DEBUGFLAGS += -DRUM_TRACE_RULESMAP
# DEBUGFLAGS += -DRUM_TRACE_STRSTRMAP
#DEBUGFLAGS += -DRUM_TRACE_REGEX
#DEBUGFLAGS += -DRUM_TRACE_TOKMATCH



WARNFLAGS = \
	-Wall \
	-Wno-unused \
	-Wundef \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-align \
	-Wwrite-strings \
	-Wconversion \
	-Wmissing-noreturn \
	-Wmissing-format-attribute \
	-Wredundant-decls \
	-Wdisabled-optimization \
	-Weffc++ \
	-Woverloaded-virtual \
	-Wsign-promo \
	-Wsynth \
	-Wextra \
	-ansi



OPTFLAGS = \
	-ggdb3 \
	-O0 \
	-fno-default-inline \
	-fno-inline


APXS_LTCFLAGS := \
	$(shell $(APXS) -q SHLTCFLAGS) \
	$(shell $(APXS) -q CFLAGS) \
	$(shell $(APXS) -q NOTEST_CPPFLAGS) \
	$(shell $(APXS) -q EXTRA_CPPFLAGS) \
	$(shell $(APXS) -q EXTRA_CFLAGS) \
	-I$(shell $(APXS) -q includedir)


APU_LTCFLAGS := \
	$(shell $(APR_CONFIG) --includes) $(shell $(APU_CONFIG) --includes)


APU_LTLDFLAGS := \
	$(shell $(APR_CONFIG) --link-libtool) \
	$(shell $(APU_CONFIG) --link-libtool)


LTCFLAGS = \
	-I$(LUA_INC_DIR) \
	-DLINUX=2 -D_REENTRANT -D_GNU_SOURCE \
	-DRUM_AP22=$(shell grep -q '^\#define MODULE_MAGIC_COOKIE 0x41503232UL' $(HTTPD_DIR)/include/ap_mmn.h && echo 1 || echo 0) \
	$(APXS_LTCFLAGS) $(APU_LTCFLAGS) $(OPTFLAGS) $(WARNFLAGS) $(DEBUGFLAGS)


LTCXXFLAGS = \
	$(LTCFLAGS) \
	-fno-exceptions -fno-rtti


LTLDFLAGS = \
	$(EXTRA_LDFLAGS) \
	$(APU_LTLDFLAGS) \
	-L$(HTTPD_DIR)/lib \
	-L$(LUA_DIR)/lib \
	-l$(LUA_LIB_NAME) -lpcre


APXSLDFLAGS = \
	$(shell echo "$(LTLDFLAGS)" | \
	  perl -p -e 's!(^| )-Wl,! -Wl,-Wl,!g;' -e 's!(^| )-R! -Wl,-R!g;')



PGM = rumtest

MODNAME = mod_rum

all: $(PGM) $(MODNAME).so

asdf:
	: $(APXSLDFLAGS)



LIB_SRCS = \
	CondModule.C \
	Config.C \
	CoreCondModule.C \
	CoreServerNameFiltCond.C \
	FStreamLogger.C \
	LuaAction.C \
	LuaConnRec.C \
	LuaManager.C \
	LuaRequestRec.C \
	LuaServerRec.C \
	LuaSockAddr.C \
	LuaTable.C \
	PathCondModule.C \
	PathMatchedFiltCond.C \
	PathRegExFiltCond.C \
	PathRegExFiltCondMatch.C \
	PathReqData.C \
	PathSlashFiltCond.C \
	Phases.C \
	PoolAllocated.C \
	PoolLogger.C \
	QueryArgCondModule.C \
	QueryArgNameFiltCond.C \
	QueryArgNameValFiltCond.C \
	QueryArgReqData.C \
	RedirExtAction.C \
	RedirIntAction.C \
	ReqCtx.C \
	ReqLogger.C \
	RequestCondModule.C \
	RequestIntRedirFiltCond.C \
	RequestSubreqFiltCond.C \
	Rule.C \
	ServerLogger.C \
	TokenMatcher.C \
	TokenRulesMap.C \
	get_cdata.C \
	pure-virt.C \
	util_misc.C \
	util_uri.C



LIB_ONLY_SRCS = \
	interactive_lua_stub.C



EXE_SRCS = \
	$(PGM).C \
	httpd_stub.C \
	interactive_lua.C \
	util_pcre.C



MOD_SRCS = $(MODNAME).C



SRCS = $(LIB_SRCS) $(EXE_SRCS) $(MOD_SRCS)



-include $(SRCS:.C=.P)



%.lo : %.C
	$(LIBTOOL) --mode=compile $(CXX) $(LTCXXFLAGS) \
	  -MD -c -o $*.lo $<
	@sed 's!^\.libs/\([^:]*\)\.o:!\1.lo:!' < .libs/$*.d > $*.d; \
	  cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	    rm -f $*.d .libs/$*.d



$(PGM): $(EXE_SRCS:.C=.lo) $(LIB_SRCS:.C=.lo)
	$(LIBTOOL) --mode=link $(CC) $(LTLDFLAGS) -lreadline -o $(@) $(+)



$(MODNAME).so: $(LIB_SRCS:.C=.lo) $(LIB_ONLY_SRCS:.C=.lo) $(MOD_SRCS:.C=.lo)
	$(APXS) -c -o $(@) $(APXSLDFLAGS) $(+)
	rm -f $(@)
	ln -s .libs/$(@)



clean:
	/bin/rm -rf *.o *.so *.lo *.loT *.la *.d *.P $(PGM) .libs
