/*========================== begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "system.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // omit min()/max() macros (favor std::min/std::max)
#endif
#include <Windows.h>
#include <io.h>
#define IS_STDERR_TTY (_isatty(_fileno(stderr)) != 0)
#define IS_STDOUT_TTY (_isatty(_fileno(stdout)) != 0)
#include <fcntl.h>
#else
#define IS_STDERR_TTY (isatty(STDERR_FILENO) != 0)
#define IS_STDOUT_TTY (isatty(STDOUT_FILENO) != 0)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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

bool iga::IsStdoutTty()
{
    return IS_STDOUT_TTY;
}

bool iga::IsStderrTty()
{
    return IS_STDERR_TTY;
}

bool iga::IsTty(const std::ostream &os)
{
    if (&os == &std::cout)
        return IsStdoutTty();
    else if (&os == &std::cerr)
        return IsStderrTty();
    return false;
}

void iga::SetStdinBinary()
{
#ifdef _WIN32
    (void)_setmode(_fileno(stdin), _O_BINARY);
    // #else Linux doesn't make a distinction
#endif
}

bool iga::DoesFileExist(const char *path)
{
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributesA(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat sb = {0};
    if (stat(path, &sb) != 0) {
        return false;
    }
    return S_ISREG(sb.st_mode);
#endif
}


// Use the color API's below.
//   emitRedText(std::ostream&,const T&)
//   emit###Text(std::ostream&,const T&)
//
// An RAII type used by the above APIs.
// Use this within a scope where you want colored text.
// The destructor restores or resets the state (even upon exception exit
// out of the scope.
//
// Note, the non-Windows version of this stomps (ANSI reset)'s the terminal.
// If stderr is not a TTY, this has no effect.
//
struct StreamColorSetter {
    enum Color { RED, GREEN, YELLOW };
    std::ostream &stream;

    bool isStderr() const {
        return &stream == &std::cerr;
    }
    bool isStdout() const {
        return &stream == &std::cout;
    }
    bool isTty() const {
        return iga::IsTty(stream);
    }

    void setColor(StreamColorSetter::Color c) {
        if (colorNeedsRestore || !isTty() || (!isStderr() && !isStdout())) {
            return;
        }
        stream.flush();
#ifdef _WIN32
        HANDLE h = GetStdHandle(isStderr() ?
            STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
        WORD w = FOREGROUND_INTENSITY;
        if (c == StreamColorSetter::Color::RED) {
            w |= FOREGROUND_RED;
        } else if (c == StreamColorSetter::Color::GREEN) {
            w |= FOREGROUND_GREEN;
        } else {
            w |= FOREGROUND_RED | FOREGROUND_GREEN;
        }
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(h, &csbi)) {
            return;
        }
        csbiAttrs = csbi.wAttributes;
        if (!SetConsoleTextAttribute(h, w)) {
            // failed to set
            return;
        }
#else
        if (c == StreamColorSetter::Color::RED) {
            stream << "\033[31;1m"; // ESC[ 31 means red, 1 means intense
        } else if (c == StreamColorSetter::Color::GREEN) {
            stream << "\033[32;1m"; // green
        } else {
            stream << "\033[33;1m"; // yellow
        }
#endif
        colorNeedsRestore = true;
    }
    void restoreColor() {
        if (colorNeedsRestore) {
#ifdef _WIN32
            // restore color attributes
            (void)SetConsoleTextAttribute(
                GetStdHandle(isStderr() ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE),
                csbiAttrs);
#else
            // ANSI reset; techinally not consistent with Windows impl. which
            // restores the old attributes; this clobbers them all
            stream << "\033[0m";
            stream.flush();
            colorNeedsRestore = false;
#endif
        }
    }

    // requires explicit setting
    StreamColorSetter(std::ostream &os)
        : stream(os)
        , colorNeedsRestore(false)
    {
    }
    // autosets the color in the constructor
    StreamColorSetter(std::ostream &os, StreamColorSetter::Color c)
        : stream(os)
        , colorNeedsRestore(false)
    {
        setColor(c);
    }
    ~StreamColorSetter() {
        stream.flush();
        restoreColor();
    }

    bool colorNeedsRestore;
#ifdef _WIN32
    WORD csbiAttrs;
#endif
};

void iga::EmitRedText(std::ostream &os, const std::string &s) {
    StreamColorSetter scs(os, StreamColorSetter::RED);
    os << s;
}
void iga::EmitGreenText(std::ostream &os, const std::string &s) {
    StreamColorSetter scs(os, StreamColorSetter::GREEN);
    os << s;
}
void iga::EmitYellowText(std::ostream &os, const std::string &s) {
    StreamColorSetter scs(os, StreamColorSetter::YELLOW);
    os << s;
}
