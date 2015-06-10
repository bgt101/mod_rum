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



#include "apr_thread_rwlock.h"
#include "apr_thread_mutex.h"
#include "lua.hpp"
#include "Blob.H"
#include "LuaManager.H"
#include "Logger.H"
#include "StrVec.H"
#include "debug.H"
#include "util_misc.H"
#include "TmpPool.H"



#define RUM_MAX_INT (static_cast<int>((static_cast<unsigned int>(1) << \
                                       (sizeof(int) * 8 - 1)) - 1))



namespace rum
{
    LuaManager::LuaManager(apr_pool_t *p, Logger *logger__,
                           const char *baseDir__)
        : PoolAllocated(p),
          logger_(logger__),
          baseDir_(baseDir__),
          luaSlotFreeVec_(pool()),
          luaSlotNumFree_(0),
          luaSlotFreeVecMutex_(0),
          defnScripts_(0),
          defnScriptFiles_(0),
          defnVer_(0),
          defnsLock_(0),
          poolMutex_(0),
          srPkgPath_(0),
          srPkgCpath_(0),
          compilerLuaState_(0),
          cbVec_(pool())
    {
        RUM_PTRC_CONFIG(pool(), "LuaManager::LuaManager(), "
                        "this: " << (void *)this);

        apr_thread_rwlock_create(&defnsLock_, pool());
        apr_thread_mutex_create(&poolMutex_, APR_THREAD_MUTEX_DEFAULT,
                                pool());
        apr_thread_mutex_create(&luaSlotFreeVecMutex_, APR_THREAD_MUTEX_DEFAULT,
                                pool());

        // create package.path and package.cpath entries for RumBaseDir
#ifdef WIN32
        srPkgPath_ = apr_pstrcat(pool(), baseDir_, "\\?.lua", NULL);
        srPkgCpath_ = apr_pstrcat(pool(), baseDir_, "\\?.dll", NULL);
#else
        srPkgPath_ = apr_pstrcat(pool(), baseDir_, "/?.lua", NULL);
        srPkgCpath_ = apr_pstrcat(pool(), baseDir_, "/?.so", NULL);
#endif
    }



    LuaManager::~LuaManager()
    {
        RUM_PTRC_CONFIG(pool(), "LuaManager::~LuaManager(), this: "
                        << (void *)this);

        apr_thread_rwlock_destroy(defnsLock_);
        apr_thread_mutex_destroy(poolMutex_);
        apr_thread_mutex_destroy(luaSlotFreeVecMutex_);

        if (compilerLuaState_)
        {
            lua_close(compilerLuaState_);
        }
    }



    LuaManager::LuaSlot *LuaManager::acquire()
    {
        LuaSlot *luaSlot;
        void *vp;
        apr_status_t rc;
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "acquiring Lua resource");

        apr_thread_mutex_lock(luaSlotFreeVecMutex_);
        if (luaSlotNumFree_ == 0)
        {
            apr_thread_mutex_lock(poolMutex());
            luaSlot = new (pool(), PoolAllocated::UseSubPools)
                      LuaManager::LuaSlot(0);
            apr_thread_mutex_unlock(poolMutex());
        }
        else
        {
            apr_size_t idx = --luaSlotNumFree_;
            luaSlot = luaSlotFreeVec_[idx];
            luaSlotFreeVec_[idx] = 0;
        }
        apr_thread_mutex_unlock(luaSlotFreeVecMutex_);

        apr_thread_rwlock_rdlock(defnsLock_);
        if ((luaSlot->luaState() == 0) || luaSlot->defnVer() != defnVer_)
        {
            if (luaSlot->luaState())
            {
                lua_close(luaSlot->luaState());
            }

            // create a new Lua state
            lua_State *luaState = luaL_newstate();
            luaL_openlibs(luaState);

            // push debug.traceback onto stack
            lua_getglobal(luaState, "debug");
            lua_getfield(luaState, -1, "traceback");
            lua_remove(luaState, -2);

            // disable some Lua methods which would have undesirable
            // effects if called
            lua_getglobal(luaState, "os");
            lua_pushliteral(luaState, "exit");
            lua_pushnil(luaState);
            lua_settable(luaState, -3);
            lua_pop(luaState, 1);

            // call registered callback functions
            apr_ssize_t sz = cbVec().size();
            apr_ssize_t i;
            int tos = lua_gettop(luaState);

            if (sz > 0)
            {
                for (i = 0; i < sz; i++)
                {
                    (cbVec()[i])(luaState, logger(), luaSlot->pool());
                    lua_pop(luaState, lua_gettop(luaState) - tos);
                }
            }

            // add entries for server root to package.path and
            // package.cpath
            const char *nPath;
            lua_getglobal(luaState, "package");
            lua_getfield(luaState, -1, "path");
            const char *lPath = lua_tostring(luaState, -1);
            if (lPath)
            {
                nPath = apr_pstrcat(luaSlot->pool(), srPkgPath(), ";",
                                    lPath, NULL);
            }
            else
            {
                nPath = srPkgPath();
            }
            lua_pop(luaState, 1);
            lua_pushliteral(luaState, "path");
            lua_pushstring(luaState, nPath);
            lua_settable(luaState, -3);

            lua_getfield(luaState, -1, "cpath");
            lPath = lua_tostring(luaState, -1);
            if (lPath)
            {
                nPath = apr_pstrcat(luaSlot->pool(), srPkgCpath(), ";",
                                    lPath, NULL);
            }
            else
            {
                nPath = srPkgCpath();
            }
            lua_pop(luaState, 1);
            lua_pushliteral(luaState, "cpath");
            lua_pushstring(luaState, nPath);
            lua_settable(luaState, -3);
            lua_pop(luaState, 1);

            // create rum proxy table plus its metabable
            lua_newtable(luaState);
            lua_newtable(luaState);
            lua_setmetatable(luaState, -2);
            lua_setglobal(luaState, "rum");

            // load global definitions into Lua slot
            rc = loadDefinitionsIntoLuaState(luaState);
            if (rc == APR_EGENERAL)
            {
                // load failed; luaState is not properly initialized,
                // so make sure it doesn't get used by setting its
                // definitions version to 0 which will cause its
                // isValid() method to return false
                RUM_LOG_CONFIG(logger_, APLOG_ERR,
                               "loading definitions into Lua state failed");
                luaSlot->store(luaState, 0);
            }
            else
            {
                // store newly initialized Lua state
                RUM_LOG_CONFIG(logger_, APLOG_DEBUG,
                               "loaded definitions into Lua state");
                luaSlot->store(luaState, defnVer_);
            }
        }
        apr_thread_rwlock_unlock(defnsLock_);

        return luaSlot;
    }



    void LuaManager::release(LuaManager::LuaSlot *luaSlot)
    {
        apr_status_t rc;
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "releasing Lua resource");

        apr_thread_mutex_lock(luaSlotFreeVecMutex_);
        apr_size_t idx = luaSlotNumFree_;
        luaSlotFreeVec_.grow_to(++luaSlotNumFree_);
        luaSlotFreeVec_[idx] = luaSlot;
        apr_thread_mutex_unlock(luaSlotFreeVecMutex_);
    }



    apr_status_t LuaManager::loadDefinitionsIntoLuaState(lua_State *luaState) const
    {
        apr_status_t lStatus;

        if (defnScripts_)
        {
            apr_ssize_t sz = defnScripts_->size();
            apr_ssize_t i;
            for (i = 0; i < sz; i++)
            {
                lStatus = luaL_loadstring(luaState, (*defnScripts_)[i]);
                if (lStatus)
                {
                    const char *msg = lua_tostring(luaState, -1);
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "failed to load definitions Lua chunk: "
                                   << msg);
                    return APR_EGENERAL;
                }
                else
                {
                    lStatus = lua_pcall(luaState, 0, 0, 1);
                    if (lStatus)
                    {
                        const char *msg = lua_tostring(luaState, -1);
                        RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                       "failed to execute definitions Lua chunk: "
                                       << msg);
                        return APR_EGENERAL;
                    }
                }
            }
        }

        if (defnScriptFiles_)
        {
            apr_ssize_t sz = defnScriptFiles_->size();
            apr_ssize_t i;
            for (i = 0; i < sz; i++)
            {
                const char *defnScriptFile = (*defnScriptFiles_)[i];
                RUM_LOG_CONFIG(logger_, APLOG_INFO,
                               "loading definition script file: "
                               << defnScriptFile);
                lStatus = luaL_loadfile(luaState, defnScriptFile);
                if (lStatus)
                {
                    const char *msg = lua_tostring(luaState, -1);
                    RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                   "failed to load definition script file: "
                                   << msg);
                    return APR_EGENERAL;
                }
                else
                {
                    lStatus = lua_pcall(luaState, 0, 0, 1);
                    if (lStatus)
                    {
                        const char *msg = lua_tostring(luaState, -1);
                        RUM_LOG_CONFIG(logger_, APLOG_ERR,
                                       "failed to execute definitions Lua chunk: "
                                       << msg);
                        return APR_EGENERAL;
                    }
                }
            }
        }

        return APR_SUCCESS;
    }



    void LuaManager::updateDefinitions(const StrVec& defnScripts,
                                       const StrVec& defnScriptFiles)
    {
        apr_thread_rwlock_wrlock(defnsLock_);

        if (defnScripts_)
        {
            delete defnScripts_;
        }
        defnScripts_ = new (pool(), PoolAllocated::UseSubPools)
                       StrVec(0, defnScripts);

        if (defnScriptFiles_)
        {
            delete defnScriptFiles_;
        }
        apr_ssize_t sz = defnScriptFiles.size();
        defnScriptFiles_ = new (pool(), PoolAllocated::UseSubPools)
                           StrVec(0, sz);
        apr_ssize_t i;
        for (i = 0; i < sz; i++)
        {
            const char *absDefnScriptFile =
                rum_base_dir_relative(defnScriptFiles_->pool(), baseDir_,
                                      defnScriptFiles[i]);
            defnScriptFiles_->push_back(absDefnScriptFile);
        }

        defnVer_++;

        apr_thread_rwlock_unlock(defnsLock_);
    }



    apr_status_t LuaManager::compileScript(apr_pool_t *chunkPool,
                                           const char *script,
                                           const Blob **chunk)
    {
        if (!compilerLuaState_)
        {
            compilerLuaState_ = luaL_newstate();
            luaL_openlibs(compilerLuaState_);
        }
        lua_pop(compilerLuaState_, lua_gettop(compilerLuaState_));

        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "compiling Lua chunk...");
        int lst = luaL_loadstring(compilerLuaState_, script);
        if (lst)
        {
            const char *msg = lua_tostring(compilerLuaState_, -1);
            RUM_LOG_CONFIG(logger_, APLOG_ERR, "failed to compile Lua chunk: "
                           << msg);
            return APR_EGENERAL;
        }

        TmpPool tmpPool(chunkPool);

        ChunkWriterData cwd;
        cwd.pool = tmpPool;
        cwd.chunkData = 0;
        cwd.chunkSize = 0;
        cwd.logger = logger_;

        lua_dump(compilerLuaState_, chunkWriter, &cwd);

        *chunk = new (chunkPool) Blob(chunkPool, cwd.chunkData, cwd.chunkSize);
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "compiled Lua chunk size: "
                       << cwd.chunkSize);

        return APR_SUCCESS;
    }



    apr_status_t LuaManager::compileScriptFile(apr_pool_t *chunkPool,
                                               const char *scriptFile,
                                               const Blob **chunk)
    {
        TmpPool tmpPool(chunkPool);

        const char *absScriptFile = rum_base_dir_relative(tmpPool, baseDir_,
                                                          scriptFile);

        if (!compilerLuaState_)
        {
            compilerLuaState_ = luaL_newstate();
            luaL_openlibs(compilerLuaState_);
        }
        lua_pop(compilerLuaState_, lua_gettop(compilerLuaState_));

        RUM_LOG_CONFIG(logger_, APLOG_INFO, "loading action script file: "
                       << absScriptFile);
        int lst = luaL_loadfile(compilerLuaState_, absScriptFile);
        if (lst)
        {
            const char *msg = lua_tostring(compilerLuaState_, -1);
            RUM_LOG_CONFIG(logger_, APLOG_ERR, "failed to compile Lua chunk: "
                           << msg);
            return APR_EGENERAL;
        }

        ChunkWriterData cwd;
        cwd.pool = tmpPool;
        cwd.chunkData = 0;
        cwd.chunkSize = 0;
        cwd.logger = logger_;

        lua_dump(compilerLuaState_, chunkWriter, &cwd);

        *chunk = new (chunkPool) Blob(chunkPool, cwd.chunkData, cwd.chunkSize);
        RUM_LOG_CONFIG(logger_, APLOG_DEBUG, "compiled Lua chunk size: "
                       << cwd.chunkSize);

        return APR_SUCCESS;
    }



    void LuaManager::registerPrepEnvCB(PrepEnvCBFunc func)
    {
        if (func)
        {
            cbVec_.push_back(func);
        }
    }



    int LuaManager::chunkWriter(lua_State *L, const void *buffer, size_t size,
                                void *ud)
    {
        ChunkWriterData *cwd = static_cast<ChunkWriterData *>(ud);
        char *d = static_cast<char *>
                  (apr_palloc(cwd->pool, cwd->chunkSize + size));
        memcpy(d, cwd->chunkData, cwd->chunkSize);
        memcpy(d + cwd->chunkSize, buffer, size);
        cwd->chunkData = d;
        cwd->chunkSize += size;

        return 0;
    }



    LuaManager::LuaSlot::LuaSlot(apr_pool_t *p)
        : PoolAllocated(p),
          L_(0),
          defnVer_(0)
    {
        RUM_PTRC_CONFIG(pool(), "LuaManager::LuaSlot::LuaSlot(), "
                        "this: " << (void *)this);
    }



    LuaManager::LuaSlot::~LuaSlot()
    {
        RUM_PTRC_CONFIG(pool(), "LuaManager::LuaSlot::~LuaSlot(), "
                        "this: " << (void *)this);
        if (L_)
        {
            lua_close(L_);
        }
    }



    LuaManager::LuaRef::LuaRef(apr_pool_t *p, LuaManager *luaManager__)
        : PoolAllocatedLight(p),
          luaManager_(luaManager__),
          luaSlot_(luaManager_->acquire())
    {
        RUM_PTRC_CONFIG(pool(), "LuaManager::LuaRef::LuaRef(), "
                        "this: " << (void *)this);
    }



    LuaManager::LuaRef::~LuaRef()
    {
        RUM_PTRC_CONFIG(pool(), "LuaManager::LuaRef::~LuaRef(), "
                        "this: " << (void *)this);
        luaManager_->release(luaSlot_);
    }



    void LuaManager::fullLuaGC(lua_State *L)
    {
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
}
