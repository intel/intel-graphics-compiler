/*========================== begin_copyright_notice ============================

Copyright (c) 2017-2021 Intel Corporation

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

#ifndef ASSERTS_HPP
#define ASSERTS_HPP

#include <cassert>

// This file defines some assertion routines that IGA modules can all share.
// Such things are called in extreme circumstances and are not to be used
// to indicate user or input error, but an internal program error.
//
// *   IGA_ASSERT(C, M) - asserts that some condition C is true
//   Upon failure, it logs an error message to both stderr and
//   OUTPUT_DEBUG_CONSOLE (see below).  It then forwards the failed assertion
//   to the underlying platforms runtime for failed assertions.
//
// *   IGA_ASSERT_FALSE(M) - indicates a failed assertion.  It is semantically
//   the same as IGA_ASSERT(false, M)
//
// *   IGA_FATAL(...) - similar to IGA_ASSERT_FALSE(), but terminates even in
//   release mode.
//
// *   OUTPUT_DEBUG_CONSOLE(...) a routine that takes a printf-style pattern
//   string and a variable number of arguments.  It converts emits that
//   information to the debugger's log (if a debugger is attached).
//   This should not be used for general error messages, but for temporary
//   debugging or reporting of severe circumstances.
//
// *   DEBUG_TRACE enabled via #define DEBUG_TRACE_ENABLED before including
//   asserts.hpp.  Sends tracing statements to the output console and stderr
//
#if (defined(_DEBUG) || defined(_INTERNAL_ASSERTS)) && !defined(BUILD_FOR_GTX)
#define IGA_ASSERT(C,M) \
    do { \
        if (!(C)) { \
            ::iga::AssertFail(__FILE__, __LINE__, #C, M); \
            assert(false && (M)); \
        } \
    } while(0)
// A short hand for IGA_ASSERT(false, M)
#define IGA_ASSERT_FALSE(M) \
    do { \
        ::iga::AssertFail(__FILE__, __LINE__, nullptr, M); \
        assert(false && (M)); \
    } while(0)
// outputs a formatted message to the Windows debugger log
// (see OutputDebugString(MSDN)).
#define OUTPUT_DEBUG_CONSOLE(...) \
    ::iga::OutputDebugConsoleImpl(__VA_ARGS__)


#else
// (void)'ing away the expression suppresses unused variable warnings
// E.g. int var = ...
//      IGA_ASSERT(var > 0, "...");
// Typically we use var elsewhere, but if we don't MSVC warning W4189
// pops in IGC and MDF.
#define IGA_ASSERT(C,M) \
    (void)(C)
// A short hand for IGA_ASSERT(false, M)
#define IGA_ASSERT_FALSE(M) \
    IGA_ASSERT(false, M)
#define OUTPUT_DEBUG_CONSOLE(...)
#endif




#ifdef _WIN32
#ifndef NORETURN_DECLSPEC
#define NORETURN_DECLSPEC __declspec(noreturn)
#endif
#ifndef NORETURN_ATTRIBUTE
#define NORETURN_ATTRIBUTE
#endif
#else
#ifndef NORETURN_DECLSPEC
#define NORETURN_DECLSPEC
#endif
#ifndef NORETURN_ATTRIBUTE
#define NORETURN_ATTRIBUTE __attribute__((noreturn))
#endif
#endif

// Similar to IGA_ASSERT_FALSE, but terminates the program in both debug
// and rease mode
#define IGA_FATAL(...) \
    do { \
       iga::FatalMessage(__VA_ARGS__); \
       iga::FatalExitProgram(); \
    } while (0)

namespace iga {
    NORETURN_DECLSPEC void NORETURN_ATTRIBUTE FatalExitProgram(); // IGA_FATAL
    void FatalMessage(const char *pat,...); // IGA_FATAL, IGA_ASSERT
    void DebugTrace(const char *pat,...); // for DEBUG_TRACE
    void OutputDebugConsoleImpl(const char *patt,...); // IGA_FATAL, DEBUG_TRACE
    void AssertFail(const char *file, int line, const char *expr, const char *msg);
}

// A trace routine used for debugging to stderr and the debug console.
// before including asserts.hpp, define DEBUG_TRACE_ENABLED
#ifdef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE(...) \
    DebugTrace(__VA_ARGS__)
#else
#define DEBUG_TRACE(...)
#endif


#endif // ASSERTS_HPP
