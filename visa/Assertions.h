/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_ASSERTION_H
#define VISA_ASSERTION_H

#include <string>


// use errorMsg to state where the error is detected (internal, input,
// unreachable state)
// use customMsg for descriptive reason behind assertion failure
void assert_and_exit(std::string errorMsg, const char* const fileName,
                     const char* const functionName, const unsigned line,
                     std::string customMsg, ...);
void assert_and_exit_generic(bool check);

// main assert flavor
// other subflavors like visa_assert, visa_assert_unreachable etc call this
// assert macro with different error and cause messages
// DO NOT USE visa_assert_top MACRO DIRECTLY IN vISA CODEBASE
#define visa_assert_top(x, errormsg, custommsg, ...) \
  do { \
    if (!(x)) \
      assert_and_exit(errormsg, __FILE__, __FUNCTION__, __LINE__, custommsg, ##__VA_ARGS__); \
  } while (0)

// vASSERT -- shorthand version that takes only assertion condition
// vISA_ASSERT -- use this assert to catch incorrect logic within vISA
// vISA_ASSERT_UNREACHABLE -- use this assert to catch unreachable state within
// vISA logic
// vISA_ASSERT_INPUT -- use this assert when outside interface (IGC) provides
// incorrect data to vISA
// TODO: have different exit codes based on the visa assert flavor?
#if defined(_DEBUG) || (!defined(DLL_MODE) && defined(_INTERNAL)) || !defined(NDEBUG)
#define vASSERT(x) \
  visa_assert_top(x, "error, assertion failed", "")
#define vISA_ASSERT(x, custommsg, ...) \
  visa_assert_top(x, "internal error, assertion failed", custommsg, ##__VA_ARGS__)
#define vISA_ASSERT_UNREACHABLE(custommsg, ...) \
  visa_assert_top(false, "unreachable state, assertion failed", custommsg, ##__VA_ARGS__)
#define vISA_ASSERT_INPUT(x, custommsg, ...) \
  visa_assert_top(x, "input error, assertion failed", custommsg, ##__VA_ARGS__)

#else // (_DEBUG) || (!(DLL_MODE) && (_INTERNAL)) || !(NDEBUG)
#define vASSERT(x) ((void)(0))
#define vISA_ASSERT(x, custommsg, ...) ((void)(0))
#define vISA_ASSERT_UNREACHABLE(custommsg, ...) ((void)(0))
#define vISA_ASSERT_INPUT(x, custommsg, ...) \
  assert_and_exit_generic(x)
#endif // _DEBUG

#endif // VISA_ASSERTION_H
