/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _FATAL_HPP_
#define _FATAL_HPP_

#include <cstdint>
#include <cstdio>
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

[[noreturn]] static void fatalExit() { exit(EXIT_FAILURE); }

static void fatalMessage(const char *str) {
  std::string s = IGA_EXE;
  s += ": ";
  s += str;
  s += '\n';

  std::cerr << s;
}

template <typename T1, typename T2 = const char *, typename T3 = const char *,
          typename T4 = const char *, typename T5 = const char *>
[[noreturn]] static void fatalExitWithMessage(T1 t1, T2 t2 = "", T3 t3 = "",
                                              T4 t4 = "", T5 t5 = "") {
  std::stringstream ss;
  ss << t1 << t2 << t3 << t4 << t5;
  fatalMessage(ss.str().c_str());
  fatalExit();
}

#endif
