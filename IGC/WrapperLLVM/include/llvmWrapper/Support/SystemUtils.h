/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_SYSTEMUTILS_H
#define IGCLLVM_SUPPORT_SYSTEMUTILS_H

#include "llvm/Support/SystemUtils.h"

namespace IGCLLVM {
bool CheckBitcodeOutputToConsole(llvm::raw_ostream &stream_to_check, bool print_warning = true) {
  return llvm::CheckBitcodeOutputToConsole(stream_to_check);
}
} // namespace IGCLLVM

#endif // IGCLLVM_SUPPORT_SYSTEMUTILS_H