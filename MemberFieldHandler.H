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



#ifndef RUM_MEMBERFIELDHANDLER_H
#define RUM_MEMBERFIELDHANDLER_H


#include "FieldHandler.H"
#include "lua.hpp"
#include "util_lua.H"
#include <stddef.h> // for offsetof()



#define RUM_REGISTER_MEMBER_FIELD(type_name, has_pool, map, name, type) \
    (map->insert(#name,                                                 \
                 new (map->pool())                                      \
                 MemberFieldHandler<type_name, has_pool>                \
                 (0,                                                    \
                  RUM_PTR_REGISTRY_NAME(type_name),                     \
                  #name,                                                \
                  offsetof(type_name, name),                            \
                  MemberFieldHandler<type_name, has_pool>::type)))



namespace rum
{
    template <typename T, bool hasPool>
    class MemberFieldHandler : public FieldHandler
    {
    public:



        // since C++ doesn't allow function templates to be partially
        // specialized, create a wrapper class which is partially
        // specialized, and stick static methods into it
        template <typename T2, bool hasPool2>
        struct Specializer
        {
            static apr_pool_t *getPool(T2 *base)
                {
                    return base->pool;
                }
        };



        // partially specialized version of Specializer class where
        // the main type doesn't have a pool
        template <typename T2>
        struct Specializer<T2, false>
        {
            static apr_pool_t *getPool(T2 * /*base*/)
                {
                    return 0;
                }
        };



        // return typed address of data member given base and offset
        template <typename T3>
        static T3 *memberPtr(T *base, apr_size_t offset)
            {
                return (reinterpret_cast<T3 *>
                        (reinterpret_cast<char *>(base) + offset));
            }



        enum FieldType
        {
            Int,
            Long,
            UInt16,
            Int64,
            CharPtr,
            RequestRec,
            ConnRec,
            ServerRec,
            SockAddr,
            Table
        };



        MemberFieldHandler(apr_pool_t *p, const char *ptrTypeRegKey__,
                           const char* name__, apr_size_t offset__,
                           FieldType type__)
            : FieldHandler(p, name__),
              ptrTypeRegKey_(ptrTypeRegKey__),
              offset_(offset__),
              type_(type__)
            { }



        int set(lua_State *L) const
            {
                T *base = checkPtr<T>(L, 1, ptrTypeRegKey_);

                if (type_ == MemberFieldHandler::CharPtr)
                {
                    char **member_ptr = memberPtr<char *>(base, offset_);

                    if (lua_type(L, 3) == LUA_TNIL)
                    {
                        *member_ptr = NULL;
                    }
                    else
                    {
                        if (hasPool)
                        {
                            apr_pool_t *p =
                                Specializer<T, hasPool>::getPool(base);
                            *member_ptr =
                                apr_pstrdup(p, luaL_checkstring(L, 3));
                        }
                        else
                        {
                            luaL_error(L, "attemp to set read-only field: %s",
                                       name());
                        }
                    }
                }
                else if (type_ == MemberFieldHandler::Int)
                {
                    int *member_ptr = memberPtr<int>(base, offset_);
                    *member_ptr = luaL_checkint(L, 3);
                }
                else if (type_ == MemberFieldHandler::Long)
                {
                    long *member_ptr = memberPtr<long>(base, offset_);
                    *member_ptr = luaL_checklong(L, 3);
                }
                else if (type_ == MemberFieldHandler::UInt16)
                {
                    apr_uint16_t *member_ptr =
                        memberPtr<apr_uint16_t>(base, offset_);
                    *member_ptr =
                        static_cast<short unsigned int>(luaL_checkint(L, 3));
                }
                else if (type_ == MemberFieldHandler::Int64)
                {
                    apr_int64_t *member_ptr =
                        memberPtr<apr_int64_t>(base, offset_);
                    *member_ptr = luaL_checkint(L, 3);
                }
                else if (type_ == MemberFieldHandler::RequestRec)
                {
                    request_rec **member_ptr =
                        memberPtr<request_rec *>(base, offset_);

                    if (lua_type(L, 3) == LUA_TNIL)
                    {
                        *member_ptr = NULL;
                    }
                    else
                    {
                        *member_ptr = RUM_CHECK_PTR(request_rec, L, 3);
                    }
                }
                else if (type_ == MemberFieldHandler::ServerRec)
                {
                    server_rec **member_ptr =
                        memberPtr<server_rec *>(base, offset_);

                    if (lua_type(L, 3) == LUA_TNIL)
                    {
                        *member_ptr = NULL;
                    }
                    else
                    {
                        *member_ptr = RUM_CHECK_PTR(server_rec, L, 3);
                    }
                }
                else if (type_ == MemberFieldHandler::ConnRec)
                {
                    conn_rec **member_ptr =
                        memberPtr<conn_rec *>(base, offset_);

                    if (lua_type(L, 3) == LUA_TNIL)
                    {
                        *member_ptr = NULL;
                    }
                    else
                    {
                        *member_ptr = RUM_CHECK_PTR(conn_rec, L, 3);
                    }
                }
                else if (type_ == MemberFieldHandler::SockAddr)
                {
                    apr_sockaddr_t **member_ptr =
                        memberPtr<apr_sockaddr_t *>(base, offset_);

                    if (lua_type(L, 3) == LUA_TNIL)
                    {
                        *member_ptr = NULL;
                    }
                    else
                    {
                        *member_ptr = RUM_CHECK_PTR(apr_sockaddr_t, L, 3);
                    }
                }
                else if (type_ == MemberFieldHandler::Table)
                {
                    apr_table_t **member_ptr =
                        memberPtr<apr_table_t *>(base, offset_);

                    if (lua_type(L, 3) == LUA_TNIL)
                    {
                        *member_ptr = NULL;
                    }
                    else
                    {
                        *member_ptr = RUM_CHECK_PTR(apr_table_t, L, 3);
                    }
                }
                else
                {
                    luaL_error(L, "unhandled field: %s", name());
                }

                return 0;
            }



        int get(lua_State *L) const
            {
                T *base = checkPtr<T>(L, 1, ptrTypeRegKey_);

                if (type_ == MemberFieldHandler::CharPtr)
                {
                    char *member = *memberPtr<char *>(base, offset_);

                    if (member)
                    {
                        lua_pushstring(L, member);
                    }
                    else
                    {
                        lua_pushnil(L);
                    }
                }
                else if (type_ == MemberFieldHandler::Int)
                {
                    int member = *memberPtr<int>(base, offset_);
                    lua_pushinteger(L, member);
                }
                else if (type_ == MemberFieldHandler::Long)
                {
                    long member = *memberPtr<long>(base, offset_);
                    lua_pushinteger(L, member);
                }
                else if (type_ == MemberFieldHandler::UInt16)
                {
                    apr_uint16_t member =
                        *memberPtr<apr_uint16_t>(base, offset_);
                    lua_pushinteger(L, member);
                }
                else if (type_ == MemberFieldHandler::Int64)
                {
                    apr_int64_t member =
                        *memberPtr<apr_int64_t>(base, offset_);
                    lua_pushinteger(L, member);
                }
                else if (type_ == MemberFieldHandler::RequestRec)
                {
                    request_rec *member =
                        *memberPtr<request_rec *>(base, offset_);

                    if (member)
                    {
                        RUM_PUSH_PTR(request_rec, L, member);
                    }
                    else
                    {
                        lua_pushnil(L);
                    }
                }
                else if (type_ == MemberFieldHandler::ServerRec)
                {
                    server_rec *member =
                        *memberPtr<server_rec *>(base, offset_);

                    if (member)
                    {
                        RUM_PUSH_PTR(server_rec, L, member);
                    }
                    else
                    {
                        lua_pushnil(L);
                    }
                }
                else if (type_ == MemberFieldHandler::ConnRec)
                {
                    conn_rec *member =
                        *memberPtr<conn_rec *>(base, offset_);

                    if (member)
                    {
                        RUM_PUSH_PTR(conn_rec, L, member);
                    }
                    else
                    {
                        lua_pushnil(L);
                    }
                }
                else if (type_ == MemberFieldHandler::SockAddr)
                {
                    apr_sockaddr_t *member =
                        *memberPtr<apr_sockaddr_t *>(base, offset_);

                    if (member)
                    {
                        RUM_PUSH_PTR(apr_sockaddr_t, L, member);
                    }
                    else
                    {
                        lua_pushnil(L);
                    }
                }
                else if (type_ == MemberFieldHandler::Table)
                {
                    apr_table_t *member =
                        *memberPtr<apr_table_t *>(base, offset_);

                    if (member)
                    {
                        RUM_PUSH_PTR(apr_table_t, L, member);
                    }
                    else
                    {
                        lua_pushnil(L);
                    }
                }
                else
                {
                    luaL_error(L, "unhandled field: %s", name());
                }

                return 1;
            }



    private:
        const char *ptrTypeRegKey_;
        apr_size_t offset_;
        FieldType type_;



        MemberFieldHandler(const MemberFieldHandler& from)
            : FieldHandler(0, 0),
              ptrTypeRegKey_(from.ptrTypeRegKey_),
              offset_(from.offset_),
              type_(from.type_)
            { }



        MemberFieldHandler& operator=(const MemberFieldHandler& that)
            {
                FieldHandler::operator=(that);
                ptrTypeRegKey_ = that.ptrTypeRegKey_;
                offset_ = that.offset_;
                type_ = that.type_;
                return *this;
            }
    };
}



#endif // RUM_MEMBERFIELDHANDLER_H
