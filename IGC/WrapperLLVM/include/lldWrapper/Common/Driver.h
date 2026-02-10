/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_LLD_COMMON_DRIVER_H
#define IGCLLVM_LLD_COMMON_DRIVER_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/raw_ostream.h"
#include "IGC/common/LLVMWarningsPop.hpp"

#include "lld/Common/Driver.h"
#include "lld/Common/CommonLinkerContext.h"

#if LLVM_VERSION_MAJOR >= 17 && !defined(IGC_LLVM_TRUNK_REVISION)
LLD_HAS_DRIVER(elf)
#endif

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
