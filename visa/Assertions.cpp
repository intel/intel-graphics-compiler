/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "llvm/Support/raw_ostream.h"

void assert_and_exit(bool check, std::string errorMsg,
    std::string customMsg) {
  if (!check) {
    llvm::errs() << errorMsg << ": " << check << ", " << customMsg << "\n";
    std::abort();
  }
}

void assert_and_exit_generic(bool check) {
  if (!check) {
    llvm::errs() << "internal compiler error, abnormal program termination\n";
    std::abort();
  }
}
