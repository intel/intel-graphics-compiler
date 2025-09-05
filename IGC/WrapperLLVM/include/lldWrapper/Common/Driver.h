/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_LLD_COMMON_DRIVER_H
#define IGCLLVM_LLD_COMMON_DRIVER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"

#include "lld/Common/Driver.h"
#include "lld/Common/CommonLinkerContext.h"

namespace IGCLLD {
namespace elf {
inline bool link(llvm::ArrayRef<const char *> Args, bool CanExitEarly, llvm::raw_ostream &stdoutOS,
                 llvm::raw_ostream &stderrOS) {
  bool r = lld::elf::link(Args, stdoutOS, stderrOS, CanExitEarly, false);
  lld::CommonLinkerContext::destroy();
  return r;
}
} // namespace elf
} // namespace IGCLLD

#endif // IGCLLVM_LLD_COMMON_DRIVER_H
