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



#ifndef RUM_STRBUFFER_H
#define RUM_STRBUFFER_H


#include <stdio.h>
#include "apr_strings.h"
#include "PoolAllocated.H"



namespace rum
{


    class StrBuffer : public PoolAllocatedLight
    {
    public:
        StrBuffer(apr_pool_t *p)
            : PoolAllocatedLight(p),
              str_(apr_pstrdup(pool(), "")),
              lineTag_(apr_pstrdup(pool(), "")),
              indentLevel_(0),
              indentWidth_(2),
              lineStart_(false)
            { }



        StrBuffer(apr_pool_t *p, const char *from)
            : PoolAllocatedLight(p),
              str_(apr_pstrdup(pool(), from)),
              lineTag_(apr_pstrdup(pool(), "")),
              indentLevel_(0),
              indentWidth_(2),
              lineStart_(false)
            { }



        virtual ~StrBuffer()
            { }



        StrBuffer& operator=(const StrBuffer& that)
            {
                PoolAllocatedLight::operator=(that);

                if (this != &that)
                {
                    str_ = apr_pstrdup(pool(), that.str_);
                }

                return *this;
            }



        void append(const char *from)
            {
                str_ = apr_pstrcat(pool(), str_, from ? from : "(nil)", NULL);
            }



        void append(const StrBuffer& from)
            {
                append(from.asStr());
            }



        void append(char c)
            {
                char buf[2];
                buf[0] = c;
                buf[1] = 0;
                append(buf);
            }



        void append(bool b)
            {
                if (b)
                {
                    append("true");
                }
                else
                {
                    append("false");
                }
            }



        void append(int i)
            {
                append(apr_itoa(pool(), i));
            }



        void append(long l)
            {
                append(apr_ltoa(pool(), l));
            }



        void append(unsigned int i)
            {
                const int sz = 41;
                char buf[sz];
                snprintf(buf, sz - 1, "%u", i);
                append(buf);
            }



        void append(unsigned long l)
            {
                const int sz = 41;
                char buf[sz];
                snprintf(buf, sz - 1, "%lu", l);
                append(buf);
            }



        void append(const void *p)
            {
                const int sz = 36;
                char buf[sz];
                snprintf(buf, sz - 1, "%p", p);
                append(buf);
            }



        apr_size_t size() const
            {
                return strlen(str_);
            }



        const char *asStr() const
            {
                return str_;
            }



        operator const char*() const
            {
                return asStr();
            }



        // this function is called from the output stream operators,
        // and is used to add the line tag and indentation at the
        // beginning of lines
        void streamOutput()
            {
                if (lineStart_)
                {
                    lineStart_ = false;
                    apr_ssize_t lineTagLen = strlen(lineTag_);
                    apr_ssize_t numSpaces = indentLevel_ * indentWidth_;
                    char buf[lineTagLen + numSpaces + 1];
                    memcpy(buf, lineTag_, lineTagLen);
                    memset(buf + lineTagLen, ' ', numSpaces);
                    buf[lineTagLen + numSpaces] = '\0';
                    append(buf);
                }
            }



        // template class encapsulating a manipulator function which
        // takes a parameter
        template <typename T>
        class Manip
        {
        public:
            Manip(StrBuffer& (*func)(StrBuffer&, T), T arg)
                : func_(func),
                  arg_(arg)
                { }

            Manip(const Manip& from)
                : func_(from.func_),
                  arg_(from.arg_)
                { }

            Manip& operator=(const Manip& that)
                {
                    PoolAllocatedLight::operator=(that);

                    if (this != &that)
                    {
                        func_ = that.func_;
                        arg_ = that.arg_;
                    }

                    return *this;
                }

            StrBuffer& (*func_)(StrBuffer&, T);
            T arg_;
        };



    private:
        char *str_;
        char *lineTag_;
        apr_ssize_t indentLevel_;
        apr_ssize_t indentWidth_;
        bool lineStart_;



        StrBuffer(const StrBuffer &from)
            : PoolAllocatedLight(from),
              str_(0),
              lineTag_(0),
              indentLevel_(0),
              indentWidth_(0),
              lineStart_(false)
            {
                // this method is private and should not be used
            }



        // manipulators are our friends
        friend StrBuffer& manip_setindentwidth(StrBuffer& sb, apr_ssize_t i);

        friend StrBuffer& manip_setindentlevel(StrBuffer& sb, apr_ssize_t i);

        friend StrBuffer& manip_setlinetag(StrBuffer& sb, const char *lt);

        friend StrBuffer& indent(StrBuffer& sb);

        friend StrBuffer& outdent(StrBuffer& sb);

        friend StrBuffer& nl(StrBuffer& sb);

    };



    inline StrBuffer& operator<<(StrBuffer& sb, const void *p)
    {
        sb.streamOutput();
        sb.append(p);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, const char *s)
    {
        sb.streamOutput();
        sb.append(s);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, const StrBuffer& src_sb)
    {
        sb.streamOutput();
        sb.append(src_sb);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, char c)
    {
        sb.streamOutput();
        sb.append(c);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, bool b)
    {
        sb.streamOutput();
        sb.append(b);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, int i)
    {
        sb.streamOutput();
        sb.append(i);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, long l)
    {
        sb.streamOutput();
        sb.append(l);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, unsigned int i)
    {
        sb.streamOutput();
        sb.append(i);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb, unsigned long l)
    {
        sb.streamOutput();
        sb.append(l);
        return sb;
    }



    inline StrBuffer& operator<<(StrBuffer& sb,
                                 StrBuffer& (*manip)(StrBuffer& sb))
    {
        return (*manip)(sb);
    }



    template <typename T>
    inline StrBuffer& operator<<(StrBuffer& sb,
                                 const StrBuffer::Manip<T>& m)
    {
        return (*(m.func_))(sb, m.arg_);
    }




    inline StrBuffer& manip_setindentlevel(StrBuffer& sb, apr_ssize_t i)
    {
        sb.indentLevel_ = i;
        return sb;
    }



    inline StrBuffer& manip_setindentwidth(StrBuffer& sb, apr_ssize_t i)
    {
        sb.indentWidth_ = i;
        return sb;
    }



    inline StrBuffer& manip_setlinetag(StrBuffer& sb, const char *lt)
    {
        sb.lineTag_ = apr_pstrdup(sb.pool(), lt);
        return sb;
    }



    inline StrBuffer::Manip<apr_ssize_t> setindentlevel(apr_ssize_t i)
    {
        return StrBuffer::Manip<apr_ssize_t>(manip_setindentlevel, i);
    }



    inline StrBuffer::Manip<apr_ssize_t> setindentwidth(apr_ssize_t i)
    {
        return StrBuffer::Manip<apr_ssize_t>(manip_setindentwidth, i);
    }



    inline StrBuffer::Manip<const char *> setlinetag(const char *lt)
    {
        return StrBuffer::Manip<const char *>(manip_setlinetag, lt);
    }



    inline StrBuffer& indent(StrBuffer& sb)
    {
        sb.indentLevel_++;
        return sb;
    }



    inline StrBuffer& outdent(StrBuffer& sb)
    {
        if (sb.indentLevel_ > 0)
        {
            sb.indentLevel_--;
        }
        return sb;
    }



    inline StrBuffer& nl(StrBuffer& sb)
    {
        sb.append('\n');
        sb.lineStart_ = true;
        return sb;
    }


}



#endif // RUM_STRBUFFER_H
