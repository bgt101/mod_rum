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



#ifndef RUM_LUATABLE_H
#define RUM_LUATABLE_H


#include "PoolAllocated.H"
#include "Logger.H"
#include "debug.H"



// forward declarations
class lua_State;



namespace rum
{
    class LuaTable : public PoolAllocated
    {
    public:
        LuaTable(apr_pool_t *p, Logger *l)
            : PoolAllocated(p),
              logger_(l)
            { }



        virtual ~LuaTable()
            { }



        Logger *logger()
            {
                return logger_;
            }



        static void *fhmRegKey()
            {
                return &fhmRegKey_useMyAddress_;
            }



        static void luaPrepEnv(lua_State *L, Logger *logger, apr_pool_t *);



        // APR table methods
        static int tableAdd(lua_State *L);
        static int tableMerge(lua_State *L);
        static int tableSet(lua_State *L);
        static int tableRawGet(lua_State *L);
        static int tableGet(lua_State *L);
        static int tableRawMget(lua_State *L);
        static int tableMget(lua_State *L);
        static int tableKeys(lua_State *L);
        static int tableUnset(lua_State *L);
        static int tableClear(lua_State *L);
        static int tableDedupeKeys(lua_State *L);
        static int tableCompress(lua_State *L);



        // APR table metamethods
        static int tableIndex(lua_State *L);
        static int tableNewindex(lua_State *L);
        static int tableTostring(lua_State *L);
        static int tableLen(lua_State *L);



    private:
        Logger *logger_;
        static char fhmRegKey_useMyAddress_;



        LuaTable(const LuaTable& from)
            : PoolAllocated(from),
              logger_(from.logger_)
            { }



        LuaTable& operator=(const LuaTable& that)
            {
                PoolAllocated::operator=(that);
                logger_ = that.logger_;
                return *this;
            }



        // struct used as the user data of the apr_table_do() call back
        struct TableCBRec
        {
            TableCBRec(lua_State *L__, bool cooked__)
                : L(L__),
                  cooked(cooked__),
                  nextIdx(1)
                { }

            TableCBRec(const TableCBRec& from)
                : L(from.L),
                  cooked(from.cooked),
                  nextIdx(from.nextIdx)
                {}

            TableCBRec& operator=(const TableCBRec& that)
                {
                    L = that.L;
                    cooked = that.cooked;
                    nextIdx = that.nextIdx;
                    return *this;
                }


            lua_State *L;
            bool cooked;
            int nextIdx;
        };



        // call back function for apr_table_do() from tableKeys()
        static int tableKeysCB(void *rec, const char *key, const char *value);



        // call back function for apr_table_do() from tableMget()
        static int tableMgetCB(void *rec, const char *key, const char *value);
    };
}


#endif // RUM_LUATABLE_H
