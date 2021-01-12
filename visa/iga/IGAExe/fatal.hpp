/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#ifndef _FATAL_HPP_
#define _FATAL_HPP_

#ifdef WIN32
// DebugBreak, IsDebuggerPresent, OutputDebugStringA
#include <Windows.h>
#endif

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#if INTPTR_MAX == INT32_MAX
#define IGA_EXE "iga32"
#else
#define IGA_EXE "iga"
#endif


[[noreturn]]
static void fatalExit()
{
#ifdef _WIN32
    if (IsDebuggerPresent()) {
        DebugBreak();
    }
#endif
    exit(EXIT_FAILURE);
}


static void fatalMessage(const char *str)
{
    std::string s = IGA_EXE;
    s += ": ";
    s += str;
    s += '\n';

    std::cerr << s;
#ifdef _WIN32
    OutputDebugStringA(s.c_str());
#endif
}


template <
    typename T1,
    typename T2 = const char *,
    typename T3 = const char *,
    typename T4 = const char *,
    typename T5 = const char *>
[[noreturn]]
static void fatalExitWithMessage(
    T1 t1, T2 t2 = "", T3 t3 = "", T4 t4 = "", T5 t5 = "")
{
    std::stringstream ss;
    ss << t1 << t2 << t3 << t4 << t5;
    fatalMessage(ss.str().c_str());
    fatalExit();
}

#endif
