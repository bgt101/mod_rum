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



#ifndef RUM_DEBUG_MACROS_H
#define RUM_DEBUG_MACROS_H



#ifdef RUM_TRACE


#  ifdef RUM_TRACE_ALL

#    ifndef RUM_TRACE_EXP
#      define RUM_TRACE_EXP
#    endif

#    ifndef RUM_TRACE_MSG
#      define RUM_TRACE_MSG
#    endif

#    ifndef RUM_TRACE_POOL
#      define RUM_TRACE_POOL
#    endif

#    ifndef RUM_TRACE_CONFIG
#      define RUM_TRACE_CONFIG
#    endif

#    ifndef RUM_TRACE_RULE
#      define RUM_TRACE_RULE
#    endif

#    ifndef RUM_TRACE_ACTION
#      define RUM_TRACE_ACTION
#    endif

#    ifndef RUM_TRACE_MATCH
#      define RUM_TRACE_MATCH
#    endif

#    ifndef RUM_TRACE_TOKMATCH
#      define RUM_TRACE_TOKMATCH
#    endif

#    ifndef RUM_TRACE_VECTOR
#      define RUM_TRACE_VECTOR
#    endif

#    ifndef RUM_TRACE_MAP
#      define RUM_TRACE_MAP
#    endif

#    ifndef RUM_TRACE_RULESMAP
#      define RUM_TRACE_RULESMAP
#    endif

#    ifndef RUM_TRACE_CONDMODMAP
#      define RUM_TRACE_CONDMODMAP
#    endif

#    ifndef RUM_TRACE_STRSTRMAP
#      define RUM_TRACE_STRSTRMAP
#    endif

#    ifndef RUM_TRACE_REGEX
#      define RUM_TRACE_REGEX
#    endif

#    ifndef RUM_TRACE_COND
#      define RUM_TRACE_COND
#    endif

#  endif // RUM_TRACE_ALL



#  ifdef RUM_TRACE_EXP
#    define RUM_PTRC_EXP(p, x)                                  \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << #x;                                          \
        buf____ << ":  ";                                       \
        buf____ << x;                                           \
        fprintf(stderr, "[exprn]:  %s\n", buf____.asStr());     \
    }

#    define RUM_STRC_EXP(s, x)                                  \
    {                                                           \
        rum::StaticStrBuffer<s> buf____;                        \
        buf____ << x;                                           \
        fprintf(stderr, "[exprn]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_MSG
#    define RUM_PTRC_MSG(p, x)                                  \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[messg]:  %s\n", buf____.asStr());     \
    }

#    define RUM_STRC_MSG(s, x)                                  \
    {                                                           \
        rum::StaticStrBuffer<s> buf____;                        \
        buf____ << x;                                           \
        fprintf(stderr, "[messg]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_POOL
#    define RUM_PTRC_POOL(p, x)                                 \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[pools]:  %s\n", buf____.asStr());     \
    }

#    define RUM_STRC_POOL(s, x)                                 \
    {                                                           \
        rum::StaticStrBuffer<s> buf____;                        \
        buf____ << x;                                           \
        fprintf(stderr, "[pools]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_CONFIG
#    define RUM_PTRC_CONFIG(p, x)                               \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[confg]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_RULE
#    define RUM_PTRC_RULE(p, x)                                 \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[ rule]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_ACTION
#    define RUM_PTRC_ACTION(p, x)                               \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[actio]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_MATCH
#    define RUM_PTRC_MATCH(p, x)                                \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[match]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_TOKMATCH
#    define RUM_PTRC_TOKMATCH(p, x)                             \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[tokmt]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_VECTOR
#    define RUM_PTRC_VECTOR(p, x)                               \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[vectr]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_MAP
#    define RUM_PTRC_MAP(p, x)                                  \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[  map]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_RULESMAP
#    define RUM_PTRC_RULESMAP(p, x)                             \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[rlmap]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_CONDMODMAP
#    define RUM_PTRC_CONDMODMAP(p, x)                           \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[cmmap]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_STRSTRMAP
#    define RUM_PTRC_STRSTRMAP(p, x)                            \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[strmp]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_REGEX
#    define RUM_PTRC_REGEX(p, x)                                \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[regex]:  %s\n", buf____.asStr());     \
    }
#  endif


#  ifdef RUM_TRACE_COND
#    define RUM_PTRC_COND(p, x)                                 \
    {                                                           \
        rum::StrBuffer buf____(const_cast<apr_pool_t *>(p));    \
        buf____ << x;                                           \
        fprintf(stderr, "[ cond]:  %s\n", buf____.asStr());     \
    }
#  endif


#endif // RUM_TRACE



#ifndef RUM_PTRC_EXP
#  define RUM_PTRC_EXP(p, x)
#endif



#ifndef RUM_STRC_EXP
#  define RUM_STRC_EXP(s, x)
#endif



#ifndef RUM_PTRC_MSG
#  define RUM_PTRC_MSG(p, x)
#endif



#ifndef RUM_STRC_MSG
#  define RUM_STRC_MSG(s, x)
#endif



#ifndef RUM_PTRC_POOL
#  define RUM_PTRC_POOL(p, x)
#endif



#ifndef RUM_STRC_POOL
#  define RUM_STRC_POOL(s, x)
#endif



#ifndef RUM_PTRC_CONFIG
#  define RUM_PTRC_CONFIG(p, x)
#endif



#ifndef RUM_PTRC_RULE
#  define RUM_PTRC_RULE(p, x)
#endif



#ifndef RUM_PTRC_ACTION
#  define RUM_PTRC_ACTION(p, x)
#endif



#ifndef RUM_PTRC_MATCH
#  define RUM_PTRC_MATCH(p, x)
#endif



#ifndef RUM_PTRC_TOKMATCH
#  define RUM_PTRC_TOKMATCH(p, x)
#endif



#ifndef RUM_PTRC_VECTOR
#  define RUM_PTRC_VECTOR(p, x)
#endif



#ifndef RUM_PTRC_MAP
#  define RUM_PTRC_MAP(p, x)
#endif



#ifndef RUM_PTRC_RULESMAP
#  define RUM_PTRC_RULESMAP(p, x)
#endif



#ifndef RUM_PTRC_CONDMODMAP
#  define RUM_PTRC_CONDMODMAP(p, x)
#endif



#ifndef RUM_PTRC_STRSTRMAP
#  define RUM_PTRC_STRSTRMAP(p, x)
#endif



#ifndef RUM_PTRC_REGEX
#  define RUM_PTRC_REGEX(p, x)
#endif



#ifndef RUM_PTRC_COND
#  define RUM_PTRC_COND(p, x)
#endif



#ifndef RUM_PTRC_ALLOC
#  define RUM_PTRC_ALLOC(p, x)
#endif










#define RUM_DO_LOG_MSG
#ifdef RUM_DO_LOG_MSG
#define RUM_LOG_MSG(logger__, level__, x__)                             \
    {                                                                   \
        if ((logger__)->level() >= (level__))                           \
        {                                                               \
            rum::StrBuffer                                              \
                buf____(const_cast<apr_pool_t *>((logger__)->pool()));  \
            buf____ << "[msg] " << x__;                                 \
            (logger__)->log("rum/" __FILE__, __LINE__, (level__),       \
                            buf____);                                   \
        }                                                               \
    }
#else
#define RUM_LOG_MSG(logger__, level__, x__)
#endif



#define RUM_DO_LOG_ACTION
#ifdef RUM_DO_LOG_ACTION
#define RUM_LOG_ACTION(logger__, level__, x__)                          \
    {                                                                   \
        if ((logger__)->level() >= (level__))                           \
        {                                                               \
            rum::StrBuffer                                              \
                buf____(const_cast<apr_pool_t *>((logger__)->pool()));  \
            buf____ << "[action] " << x__;                              \
            (logger__)->log("rum/" __FILE__, __LINE__, (level__),       \
                            buf____);                                   \
        }                                                               \
    }
#else
#define RUM_LOG_ACTION(logger__, level__, x__)
#endif



#define RUM_DO_LOG_CONFIG
#ifdef RUM_DO_LOG_CONFIG
#define RUM_LOG_CONFIG(logger__, level__, x__)                          \
    {                                                                   \
        if ((logger__)->level() >= (level__))                           \
        {                                                               \
            rum::StrBuffer                                              \
                buf____(const_cast<apr_pool_t *>((logger__)->pool()));  \
            buf____ << "[config] " << x__;                              \
            (logger__)->log("rum/" __FILE__, __LINE__, (level__),       \
                            buf____);                                   \
        }                                                               \
    }
#else
#define RUM_LOG_CONFIG(logger__, level__, x__)
#endif



#define RUM_DO_LOG_COND
#ifdef RUM_DO_LOG_COND
#define RUM_LOG_COND(logger__, level__, x__)                            \
    {                                                                   \
        if ((logger__)->level() >= (level__))                           \
        {                                                               \
            rum::StrBuffer                                              \
                buf____(const_cast<apr_pool_t *>((logger__)->pool()));  \
            buf____ << "[cond] " << x__;                                \
            (logger__)->log("rum/" __FILE__, __LINE__, (level__),       \
                            buf____);                                   \
        }                                                               \
    }
#else
#define RUM_LOG_COND(logger__, level__, x__)
#endif



#define RUM_DO_LOG_REGEX
#ifdef RUM_DO_LOG_REGEX
#define RUM_LOG_REGEX(logger__, level__, x__)                           \
    {                                                                   \
        if ((logger__)->level() >= (level__))                           \
        {                                                               \
            rum::StrBuffer                                              \
                buf____(const_cast<apr_pool_t *>((logger__)->pool()));  \
            buf____ << "[regex] " << x__;                               \
            (logger__)->log("rum/" __FILE__, __LINE__, (level__),       \
                            buf____);                                   \
        }                                                               \
    }
#else
#define RUM_LOG_REGEX(logger__, level__, x__)
#endif



#define RUM_DO_LOG_TOKMATCH
#ifdef RUM_DO_LOG_TOKMATCH
#define RUM_LOG_TOKMATCH(logger__, level__, x__)                        \
    {                                                                   \
        if ((logger__)->level() >= (level__))                           \
        {                                                               \
            rum::StrBuffer                                              \
                buf____(const_cast<apr_pool_t *>((logger__)->pool()));  \
            buf____ << "[tokmatch] " << x__;                            \
            (logger__)->log("rum/" __FILE__, __LINE__, (level__),       \
                            buf____);                                   \
        }                                                               \
    }
#else
#define RUM_LOG_TOKMATCH(logger__, level__, x__)
#endif



#endif // RUM_DEBUG_MACROS_H
