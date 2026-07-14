/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_MC_MCASMINFO_H
#define IGCLLVM_MC_MCASMINFO_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/MC/MCAsmInfo.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline llvm::StringRef getInternalSymbolPrefix(const llvm::MCAsmInfo &MAI) {
#if LLVM_VERSION_MAJOR < 23
  return MAI.getPrivateGlobalPrefix();
#else
  return MAI.getInternalSymbolPrefix();
#endif
}

} // namespace IGCLLVM

#endif // IGCLLVM_MC_MCASMINFO_H
