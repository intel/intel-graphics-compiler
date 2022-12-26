/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_ASSERTION_H
#define VISA_ASSERTION_H

#include <string>


// use errorMsg to state where the error is detected (internal, input,
// unreachable state)
// use causeMsg for descriptive reason behind assertion failure
void assert_and_exit(bool check, std::string errorMsg, std::string causeMsg, ...);
void assert_and_exit_generic(bool check);

// main assert flavor
// other subflavors like visa_assert, visa_assert_unreachable etc call this
// assert macro with different error and cause messages
// DO NOT USE visa_assert_top MACRO DIRECTLY IN vISA CODEBASE
#define visa_assert_top(x, errormsg, causemsg, ...) \
  assert_and_exit(x, errormsg, causemsg, ##__VA_ARGS__)

// vASSERT -- shorthand version that takes only assertion condition
// vISA_ASSERT -- use this assert to catch incorrect logic within vISA
// vISA_ASSERT_UNREACHABLE -- use this assert to catch unreachable state within
// vISA logic
// vISA_ASSERT_INPUT -- use this assert when outside interface (IGC) provides
// incorrect data to vISA
// TODO: have different exit codes based on the visa assert flavor?
#if defined (_DEBUG) || !defined(DLL_MODE) || !defined(NDEBUG)
#define vASSERT(x) \
  visa_assert_top(x, "error, assertion failed", "")
#define vISA_ASSERT(x, causemsg, ...) \
  visa_assert_top(x, "internal error, assertion failed", causemsg, ##__VA_ARGS__)
#define vISA_ASSERT_UNREACHABLE(causemsg, ...) \
  visa_assert_top(false, "unreachable state, assertion failed", causemsg, ##__VA_ARGS__)
#define vISA_ASSERT_INPUT(x, causemsg, ...) \
  visa_assert_top(x, "input error, assertion failed", causemsg, ##__VA_ARGS__)

#else // (_DEBUG) || !(DLL_MODE) || !(NDEBUG)
#define vASSERT(x) ((void)(0))
#define vISA_ASSERT(x, causemsg, ...) ((void)(0))
#define vISA_ASSERT_UNREACHABLE(causemsg, ...) ((void)(0))
#define vISA_ASSERT_INPUT(x, causemsg, ...) \
  assert_and_exit_generic(x)
#endif // _DEBUG

#endif // VISA_ASSERTION_H
