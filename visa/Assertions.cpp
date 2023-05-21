/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdarg>

void assert_and_exit(std::string errorMsg, const char* const fileName,
    const char* const functionName, const unsigned int line, std::string customMsg, ...) {
  char causeMsg[1024];
  std::va_list vargs;
  va_start(vargs, customMsg);
  int writtenB =
    std::vsnprintf(causeMsg, sizeof(causeMsg), customMsg.c_str(), vargs);
  va_end(vargs);

  if (writtenB > 0) {
    // writtenB is not negative; succesfully written the cause msg with
    // variadic arguments
    llvm::errs() << errorMsg << ": " << std::string(causeMsg)
                 << "\nfile: " << std::string(fileName)
                 << "\nfunction name: " << std::string(functionName)
                 << "\nline: " << line << "\n";
  } else {
    // writtenB is negative, just print the custom msg as is
    llvm::errs() << errorMsg << ": " << customMsg
                 << "\nfile: " << std::string(fileName)
                 << "\nfunction name: " << std::string(functionName)
                 << "\nline: " << line << "\n";
  }
  std::abort();
}

void assert_and_exit_generic(bool check) {
  if (!check) {
    llvm::errs() << "internal compiler error, abnormal program termination\n";
    std::abort();
  }
}
