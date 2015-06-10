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



#ifndef RUM_LUAMANAGER_H
#define RUM_LUAMANAGER_H


#include "PoolAllocated.H"
#include "TmpPool.H"
#include "SmplVec.H"
#include "PtrVec.H"



// forward declarations
class lua_State;
class apr_thread_rwlock_t;
class apr_thread_mutex_t;



namespace rum
{
    // forward declarations
    class Logger;
    class StrVec;
    class Blob;



    typedef void (*PrepEnvCBFunc)(lua_State *, Logger *, apr_pool_t *);



    class LuaManager : public PoolAllocated
    {
    public:
        // structure used by chunkWriter()
        struct ChunkWriterData
        {
            apr_pool_t *pool;
            const char *chunkData;
            apr_ssize_t chunkSize;
            Logger *logger;
        };



        // long lived class containing a Lua state along with the
        // version of the global definitions loaded into it
        class LuaSlot : public PoolAllocated
        {
        public:
            LuaSlot(apr_pool_t *p);

            virtual ~LuaSlot();

            lua_State *luaState()
                {
                    return L_;
                }

            int defnVer() const
                {
                    return defnVer_;
                }

            bool isValid() const
                {
                    return defnVer_ != 0;
                }

            void store(lua_State *L, int defnVer__)
                {
                    L_ = L;
                    defnVer_ = defnVer__;
                }

        private:
            lua_State *L_;
            int defnVer_;

            LuaSlot(const LuaSlot& from)
                : PoolAllocated(from),
                  L_(from.L_),
                  defnVer_(from.defnVer_)
                { }

            LuaSlot& operator=(const LuaSlot& that)
                {
                    PoolAllocated::operator=(that);
                    L_ = that.L_;
                    defnVer_ = that.defnVer_;
                    return *this;
                }
        };



        // short lived class whose instance acquires a long lived
        // LuaSlot during its lifetime
        class LuaRef : public PoolAllocatedLight
        {
        public:
            LuaRef(apr_pool_t *p, LuaManager *luaManager__);

            virtual ~LuaRef();

            bool isValid() const
                {
                    return luaSlot_->isValid();
                }

            lua_State *state()
                {
                    return luaSlot_->luaState();
                }

        private:
            LuaManager *luaManager_;
            LuaSlot *luaSlot_;

            LuaRef(const LuaRef& from)
                : PoolAllocatedLight(from),
                  luaManager_(from.luaManager_),
                  luaSlot_(from.luaSlot_)
                { }

            LuaRef& operator=(const LuaRef& that)
                {
                    PoolAllocatedLight::operator=(that);
                    luaManager_ = that.luaManager_;
                    luaSlot_ = that.luaSlot_;
                    return *this;
                }
        };



        LuaManager(apr_pool_t *p, Logger *logger__, const char *baseDir__);



        virtual ~LuaManager();



        LuaSlot *acquire();



        void release(LuaSlot *);



        apr_status_t loadDefinitionsIntoLuaState(lua_State *L) const;



        void updateDefinitions(const StrVec& defnScripts,
                               const StrVec& defnScriptFiles);



        const char *srPkgPath() const
            {
                return srPkgPath_;
            }



        const char *srPkgCpath() const
            {
                return srPkgCpath_;
            }



        apr_status_t compileScript(apr_pool_t *chunkPool, const char *script,
                                   const Blob **chunk);



        apr_status_t compileScriptFile(apr_pool_t *chunkPool, const char *file,
                                       const Blob **chunk);



        Logger *logger()
            {
                return logger_;
            }



        apr_thread_mutex_t *poolMutex()
            {
                return poolMutex_;
            }



        void registerPrepEnvCB(PrepEnvCBFunc);



        const SmplVec<PrepEnvCBFunc>& cbVec() const
            {
                return cbVec_;
            }



        static void fullLuaGC(lua_State *L);



    private:
        Logger *logger_;
        const char *baseDir_;
        PtrVec<LuaManager::LuaSlot *> luaSlotFreeVec_;
        int luaSlotNumFree_;
        apr_thread_mutex_t *luaSlotFreeVecMutex_;
        StrVec *defnScripts_;
        StrVec *defnScriptFiles_;
        int defnVer_;
        apr_thread_rwlock_t *defnsLock_;
        apr_thread_mutex_t *poolMutex_;
        const char *srPkgPath_;
        const char *srPkgCpath_;
        lua_State *compilerLuaState_;
        SmplVec<PrepEnvCBFunc> cbVec_;



        LuaManager(const LuaManager& from)
            : PoolAllocated(from),
              logger_(from.logger_),
              baseDir_(from.baseDir_),
              luaSlotFreeVec_(0),
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
              cbVec_(0)
            { }



        LuaManager& operator=(const LuaManager& that)
            {
                PoolAllocated::operator=(that);
                return *this;
            }



        // called by lua_dump
        static int chunkWriter(lua_State *, const void *, size_t, void *);
    };

}



#endif // RUM_LUAMANAGER_H
