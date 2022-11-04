/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // omit min()/max() macros (favor std::min/std::max)
#endif
#include <Windows.h>
#endif
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "asserts.hpp"
#include "strings.hpp"

using namespace iga;

void iga::OutputDebugConsoleImpl(const char *msg) {
#ifdef _WIN32
  OutputDebugStringA(msg);
#else
  //    TODO: gcc hooks or other system log semantics?
  //    std::cerr << msg;
#endif
}

void iga::FatalMessage(const char *msg) { std::cerr << msg; }

NORETURN_DECLSPEC void NORETURN_ATTRIBUTE iga::FatalExitProgram() {
#ifdef _WIN32
  if (IsDebuggerPresent()) {
    DebugBreak();
  }
#endif
  exit(EXIT_FAILURE);
}

void iga::DebugTrace(const char *msg) {
  OutputDebugConsoleImpl(msg);
  std::cerr << msg;
}

void iga::AssertFail(const char *file, int line, const char *expr,
                     const char *msg) {

  // prune ....\IGALibrary\Models\Models.cpp
  // down to
  //   IGALibrary\Models\Models.cpp
  const char *filesfx = file + iga::stringLength(file) - 1;
  while (filesfx > file) {
    if (strncmp(filesfx, "IGA/", 4) == 0 || strncmp(filesfx, "IGA\\", 4) == 0) {
      break;
    }
    filesfx--;
  }
  auto fmtdMsg =
      expr ? iga::format(filesfx, ":", line, ": IGA_ASSERT(", expr, ", ", msg,
                         ")\n")
           : iga::format(filesfx, ":", line, ": IGA_ASSERT_FALSE(", msg, ")\n");
  FatalMessage(fmtdMsg.c_str());
}
