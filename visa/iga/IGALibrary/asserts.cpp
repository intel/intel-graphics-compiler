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

#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <string.h>

#include "asserts.hpp"
#include "strings.hpp"

using namespace iga;

static void OutputDebugConsoleImplVA(const char *pat, va_list &ap)
{
#ifdef _WIN32
    OutputDebugStringA(formatF(pat, ap).c_str());
#else
//    TODO: gcc hooks or MAC semantics?
//    fprintf(stderr, "%s", msg);
#endif
}


void iga::OutputDebugConsoleImpl(const char *pat, ...)
{
    va_list ap;
    va_start(ap, pat);
    OutputDebugConsoleImplVA(pat,ap);
    va_end(ap);
}


void iga::FatalMessage(const char *pat, ...)
{
    va_list ap;
    va_start(ap, pat);
    vfprintf(stderr, pat, ap);
    va_end(ap);
}


NORETURN_DECLSPEC void NORETURN_ATTRIBUTE iga::FatalExitProgram()
{
#ifdef _WIN32
  if (IsDebuggerPresent()) {
    DebugBreak();
  }
#endif
  exit(EXIT_FAILURE);
}


void iga::DebugTrace(const char *pat, ...)
{
    va_list ap;
    va_start(ap, pat);
    OutputDebugConsoleImplVA(pat,ap);
    vfprintf(stderr, pat, ap);
    va_end(ap);
}

void iga::AssertFail(
    const char *file,
    int line,
    const char *expr,
    const char *msg)
{

    // prune ....\gfx_Development\Tools\IGAToolChain\IGA\Models\Models.cpp
    // down to
    //   IGA\Models\Models.cpp
    const char *filesfx = file + strlen(file) - 1;
    while (filesfx > file) {
        if (strncmp(filesfx,"IGA/",4) == 0 ||
            strncmp(filesfx,"IGA\\",4) == 0)
        {
            break;
        }
        filesfx--;
    }
    if (expr) {
        iga::FatalMessage("%s:%d: IGA_ASSERT(%s, %s)\n", filesfx, line, expr, msg);
    } else {
        iga::FatalMessage("%s:%d: IGA_ASSERT_FALSE(%s)\n", filesfx, line, msg);
    }
    // va_list va;
    // va_start(va,&pat);
    // va_end(va);
    // assert(false && (M));
}
