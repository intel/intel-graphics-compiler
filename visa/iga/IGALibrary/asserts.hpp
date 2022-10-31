/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
#if defined(_DEBUG) || defined(_INTERNAL_ASSERTS)
#define IGA_ASSERT(C, M)                                                       \
  do {                                                                         \
    if (!(C)) {                                                                \
      ::iga::AssertFail(__FILE__, __LINE__, #C, M);                            \
      assert(false && (M));                                                    \
    }                                                                          \
  } while (0)
// A short hand for IGA_ASSERT(false, M)
#define IGA_ASSERT_FALSE(M)                                                    \
  do {                                                                         \
    ::iga::AssertFail(__FILE__, __LINE__, nullptr, M);                         \
    assert(false && (M));                                                      \
  } while (0)
// outputs a formatted message to the Windows debugger log
// (see OutputDebugString(MSDN)).
#define OUTPUT_DEBUG_CONSOLE(M) ::iga::OutputDebugConsoleImpl(M)

#else
// (void)'ing away the expression suppresses unused variable warnings
// E.g. int var = ...
//      IGA_ASSERT(var > 0, "...");
// Typically we use var elsewhere, but if we don't MSVC warning W4189
// pops in IGC and MDF.
#define IGA_ASSERT(C, M) (void)(C)
// A short hand for IGA_ASSERT(false, M)
#define IGA_ASSERT_FALSE(M) IGA_ASSERT(false, M)
#define OUTPUT_DEBUG_CONSOLE(M)
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
// and release mode
#define IGA_FATAL(M)                                                           \
  do {                                                                         \
    iga::FatalMessage(M);                                                      \
    iga::FatalExitProgram();                                                   \
  } while (0)

namespace iga {
NORETURN_DECLSPEC void NORETURN_ATTRIBUTE FatalExitProgram(); // IGA_FATAL
void FatalMessage(const char *msg);           // IGA_FATAL, IGA_ASSERT
void DebugTrace(const char *msg);             // for DEBUG_TRACE
void OutputDebugConsoleImpl(const char *msg); // IGA_FATAL, DEBUG_TRACE
void AssertFail(const char *file, int line, const char *expr, const char *msg);
} // namespace iga

// A trace routine used for debugging to stderr and the debug console.
// before including asserts.hpp, define DEBUG_TRACE_ENABLED
#ifdef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE(M) DebugTrace(M)
#else
#define DEBUG_TRACE(M)
#endif

#endif // ASSERTS_HPP
