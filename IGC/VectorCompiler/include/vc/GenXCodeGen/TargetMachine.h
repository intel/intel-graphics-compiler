/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_LIB_GENXCODEGEN_TARGETMACHINE_H
#define VC_LIB_GENXCODEGEN_TARGETMACHINE_H

#include "llvmWrapper/Support/TargetRegistry.h"

#include "vc/Support/BackendConfig.h"

namespace vc {

std::unique_ptr<llvm::TargetMachine> createGenXTargetMachine(
    const llvm::Target &T, llvm::Triple TT, llvm::StringRef CPU,
    llvm::StringRef Features, const llvm::TargetOptions &Options,
    llvm::Optional<llvm::Reloc::Model> RM,
    llvm::Optional<llvm::CodeModel::Model> CM, llvm::CodeGenOpt::Level OL,
    std::unique_ptr<llvm::GenXBackendConfig> BC);

inline bool is32BitArch(llvm::Triple TT) {
  if (TT.getTriple().find("genx32") == 0)
    return true;
  return false;
}

} // namespace vc

#endif // VC_LIB_GENXCODEGEN_TARGETMACHINE_H
