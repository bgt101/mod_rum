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



#include "LuaTable.H"
#include "lua.hpp"
#include "StrPtrMap.H"
#include "StaticStrBuffer.H"
#include "FuncFieldHandler.H"
#include "VirtualFieldHandler.H"
#include "MemberFieldHandler.H"
#include "util_lua.H"
#include "util_misc.H"
#include "apr_lib.h"
#include "apr_tables.h"
#include "LuaAction.H"



#define RUM_REGISTER_TABLE_FIELD(map, name, type) \
    RUM_REGISTER_MEMBER_FIELD(apr_table_t, true, map, name, type)



namespace rum
{

    // APR table methods
    static const struct luaL_Reg tableMethods[] =
    {
        {"add", LuaTable::tableAdd},
        {"merge", LuaTable::tableMerge},
        {"set", LuaTable::tableSet},
        {"get", LuaTable::tableGet},
        {"mget", LuaTable::tableMget},
        {"raw_get", LuaTable::tableRawGet},
        {"raw_mget", LuaTable::tableRawMget},
        {"keys", LuaTable::tableKeys},
        {"unset", LuaTable::tableUnset},
        {"clear", LuaTable::tableClear},
        {"dedupe_keys", LuaTable::tableDedupeKeys},
        {"compress", LuaTable::tableCompress},
        {0, 0}
    };



    // APR table metamethods
    static const struct luaL_Reg tableMetaMethods[] =
    {
        {"__index", LuaTable::tableIndex},
        {"__newindex", LuaTable::tableNewindex},
        {"__len", LuaTable::tableLen},
        {"__tostring", LuaTable::tableTostring},
        {0, 0}
    };



    // APR table virtual field methods
    static const struct VirtualFieldHandler::fieldReg tableVFMethods[] =
    {
        {0, 0, 0}
    };



    // called when a new Lua interpreter is created
    void LuaTable::luaPrepEnv(lua_State *L, Logger *logger__,
                                apr_pool_t *pool__)
    {
        RUM_LOG_COND(logger__, APLOG_DEBUG,
                     "creating field handler map for apr_table_t");

        StrPtrMap<FieldHandler *> *fhm =
            new (pool__) StrPtrMap<FieldHandler *>(0);
        fhm->destroyWithPool();

        // register functions of table
        const luaL_Reg *funcReg = tableMethods;
        while (funcReg && funcReg->name)
        {
            fhm->insert(funcReg->name,
                        new (fhm->pool()) FuncFieldHandler(0, funcReg->name,
                                                           funcReg->func));
            funcReg++;
        }

        // add field handler map to Lua registry
        lua_pushlightuserdata(L, fhmRegKey());
        lua_pushlightuserdata(L, fhm);
        lua_settable(L, LUA_REGISTRYINDEX);

        // create metatable for (apr_table_t *), add it to the Lua
        // registry, and register the metamethods
        luaL_newmetatable(L, RUM_PTR_REGISTRY_NAME(apr_table_t));
        luaL_register(L, 0, tableMetaMethods);
        lua_pop(L, 1);
    }



    int LuaTable::tableAdd(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);
        const char *val = luaL_checkstring(L, 3);
        apr_table_add(table, key, val);
        return 0;
    }



    int LuaTable::tableMerge(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);
        const char *val = luaL_checkstring(L, 3);
        apr_table_merge(table, key, val);

        return 0;
    }



    int LuaTable::tableSet(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);
        const char *val = luaL_checkstring(L, 3);
        apr_table_set(table, key, val);
        return 0;
    }



    int LuaTable::tableGet(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);
        const char *val = apr_table_get(table, key);
        if (val == NULL)
        {
            lua_pushnil(L);
        }
        else
        {
            const char *end = rum_strchrnul(val, ',');
            lua_pushlstring(L, val, end - val);
        }
        return 1;
    }



    int LuaTable::tableMget(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);

        // return an array only if key exists in table, nil otherwise
        const char *val = apr_table_get(table, key);
        if (val)
        {
            lua_newtable(L);
            TableCBRec tableCBRec(L, true);
            apr_table_do(tableMgetCB, &tableCBRec, table, key, NULL);
        }
        else
        {
            lua_pushnil(L);
        }
        return 1;
    }



    int LuaTable::tableRawGet(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);
        const char *val = apr_table_get(table, key);
        if (val == 0)
        {
            lua_pushnil(L);
        }
        else
        {
            lua_pushstring(L, val);
        }
        return 1;
    }



    int LuaTable::tableRawMget(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);

        // return an array only if key exists in table, nil otherwise
        const char *val = apr_table_get(table, key);
        if (val)
        {
            lua_newtable(L);
            TableCBRec tableCBRec(L, false);
            apr_table_do(tableMgetCB, &tableCBRec, table, key, NULL);
        }
        else
        {
            lua_pushnil(L);
        }
        return 1;
    }



    int LuaTable::tableKeys(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        lua_newtable(L);
        TableCBRec tableCBRec(L, false);
        apr_table_do(tableKeysCB, &tableCBRec, table, NULL);
        return 1;
    }



    int LuaTable::tableUnset(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const char *key = luaL_checkstring(L, 2);
        apr_table_unset(table, key);
        return 0;
    }



    int LuaTable::tableClear(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        apr_table_clear(table);
        return 0;
    }



    int LuaTable::tableDedupeKeys(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        apr_table_compress(table, APR_OVERLAP_TABLES_SET);
        return 0;
    }



    int LuaTable::tableCompress(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        apr_table_compress(table, APR_OVERLAP_TABLES_MERGE);
        return 0;
    }



    int LuaTable::tableIndex(lua_State *L)
    {
        // get field handler map from Lua registry
        StrPtrMap<FieldHandler *> *fhm = NULL;
        lua_pushlightuserdata(L, fhmRegKey());
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (lua_islightuserdata(L, -1))
        {
            fhm = static_cast<StrPtrMap<FieldHandler *> *>
                  (lua_touserdata(L, -1));
        }
        if (!fhm)
        {
            luaL_error(L, "unable to retrieve field handler map");
        }
        lua_pop(L, 1);

        // call appropriate field handler
        int rc = 0;
        const char *key = luaL_checkstring(L, 2);
        const FieldHandler *fh = fhm->find(key);
        if (fh)
        {
            rc = fh->get(L);
        }
        else
        {
            luaL_error(L, "invalid table field: %s", key);
        }

        return rc;
    }



    int LuaTable::tableNewindex(lua_State *L)
    {
        // get field handler map from Lua registry
        StrPtrMap<FieldHandler *> *fhm = NULL;
        lua_pushlightuserdata(L, fhmRegKey());
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (lua_islightuserdata(L, -1))
        {
            fhm = static_cast<StrPtrMap<FieldHandler *> *>
                  (lua_touserdata(L, -1));
        }
        if (!fhm)
        {
            luaL_error(L, "unable to retrieve field handler map");
        }
        lua_pop(L, 1);

        // call appropriate field handler
        int rc = 0;
        const char *key = luaL_checkstring(L, 2);
        const FieldHandler *fh = fhm->find(key);
        if (fh)
        {
            rc = fh->set(L);
        }
        else
        {
            luaL_error(L, "invalid table field: %s", key);
        }

        return rc;
    }



    int LuaTable::tableLen(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        const apr_array_header_t *ah = apr_table_elts(table);
        int n = 0;
        if (ah)
        {
            n = ah->nelts;
        }
        lua_pushinteger(L, n);
        return 1;
    }



    int LuaTable::tableTostring(lua_State *L)
    {
        apr_table_t *table = RUM_CHECK_NON_NULL_PTR(apr_table_t, L, 1);
        StaticStrBuffer<80> buf;
        buf << "apr_table_t: " << (void *)table;
        lua_pushstring(L, buf);
        return 1;
    }



    int LuaTable::tableKeysCB(void *rec, const char *key,
                              const char * /*value*/)
    {
        TableCBRec *tableCBRec = static_cast<TableCBRec *>(rec);

        // only add key to table if not already there
        size_t nelts = lua_objlen(tableCBRec->L, -1);
        size_t i;
        bool found = false;
        for (i = 1; !found && (i <= nelts); i++)
        {
            lua_pushinteger(tableCBRec->L, i);
            lua_gettable(tableCBRec->L, -2);
            if (lua_isstring(tableCBRec->L, -1))
            {
                const char *s = lua_tostring(tableCBRec->L, -1);
                if (strcmp(key, s) == 0)
                {
                    found = true;
                }
            }
            lua_pop(tableCBRec->L, 1);
        }

        if (!found)
        {
            lua_pushinteger(tableCBRec->L, tableCBRec->nextIdx++);
            lua_pushstring(tableCBRec->L, key);
            lua_settable(tableCBRec->L, -3);
        }

        return 1;
    }



    int LuaTable::tableMgetCB(void *rec, const char * /*key*/,
                              const char *value)
    {
        TableCBRec *tableCBRec = static_cast<TableCBRec *>(rec);

        if (tableCBRec->cooked)
        {
            const char *cur = value;
            const char *next;
            const char *end = value + strlen(value);

            do
            {
                next = rum_strchrnul(cur, ',');

                // skip leading and trailing whitespace
                const char *cp1 = cur;
                const char *cp2 = next;
                while ((cp1 < cp2) && apr_isspace(*cp1))
                {
                    cp1++;
                }
                while ((cp1 < cp2) && apr_isspace(*cp2))
                {
                    cp2--;
                }

                lua_pushinteger(tableCBRec->L, tableCBRec->nextIdx++);
                lua_pushlstring(tableCBRec->L, cp1, cp2 - cp1);
                lua_settable(tableCBRec->L, -3);

                cur = next + 1;
            }
            while (cur < end);
        }
        else
        {
            lua_pushinteger(tableCBRec->L, tableCBRec->nextIdx++);
            lua_pushstring(tableCBRec->L, value);
            lua_settable(tableCBRec->L, -3);
        }

        return 1;
    }



    char LuaTable::fhmRegKey_useMyAddress_;
}
