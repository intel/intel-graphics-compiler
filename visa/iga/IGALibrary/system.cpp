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
#include "system.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <iostream>

using namespace iga;


#ifdef _WIN32
static void EnableColoredIO()
{
    static bool enabled = false;
    if (enabled)
        return;
    // TODO: should only need to do this on Windows 10 Threshold 2 (TH2),
    // "November Update": version 1511 and has the build number 10586
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
    auto enableOnHandle = [] (DWORD H_CODE) {
        DWORD mode;
        HANDLE h = GetStdHandle(H_CODE);
        GetConsoleMode(h, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(h, mode);
    };
    enableOnHandle(STD_ERROR_HANDLE);
    enableOnHandle(STD_OUTPUT_HANDLE);
    enabled = true;
}

// create a static constructor
struct dummy
{
    dummy() {
        EnableColoredIO();
    };
};
static dummy _dummy;

#else
// unix
//
// nothing needed here
#endif


bool iga::IsTty(const std::ostream &os)
{
    if (&os == &std::cout)
        return IS_STDOUT_TTY;
    else if (&os == &std::cerr)
        return IS_STDERR_TTY;
    return false;
}
