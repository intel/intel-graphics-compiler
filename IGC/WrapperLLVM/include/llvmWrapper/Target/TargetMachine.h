/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TARGET_TARGETMACHINE_H
#define IGCLLVM_TARGET_TARGETMACHINE_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#if LLVM_VERSION_MAJOR >= 22
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#endif
#include "IGC/common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Support/CodeGen.h"

// LLVM 22 renamed llvm::LLVMTargetMachine to llvm::CodeGenTargetMachineImpl and
// moved it to llvm/CodeGen/CodeGenTargetMachineImpl.h.
namespace IGCLLVM {
#if LLVM_VERSION_MAJOR >= 22
using LLVMTargetMachineBase = llvm::CodeGenTargetMachineImpl;
#else
using LLVMTargetMachineBase = llvm::LLVMTargetMachine;
#endif
} // namespace IGCLLVM

#if LLVM_VERSION_MAJOR < 15
#define LLVM_GET_TTI_API_QUAL
#else // LLVM_VERSION_MAJOR
#define LLVM_GET_TTI_API_QUAL const
#endif // LLVM_VERSION_MAJOR

namespace IGCLLVM {
class TargetMachine : public llvm::TargetMachine {
public:
  using CodeGenFileType = llvm::CodeGenFileType;

protected:
  TargetMachine(const llvm::Target &T, llvm::StringRef DataLayoutString, const llvm::Triple &TargetTriple,
                llvm::StringRef CPU, llvm::StringRef FS, const llvm::TargetOptions &Options)
      : llvm::TargetMachine(T, DataLayoutString, TargetTriple, CPU, FS, Options) {}

private:
  bool addPassesToEmitFile(llvm::PassManagerBase &PM, llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
                           llvm::CodeGenFileType FileType, bool DisableVerify,
                           llvm::MachineModuleInfoWrapperPass *MMIWP) override {
    llvm::MachineModuleInfo *MMI = nullptr;
    if (MMIWP) {
      MMI = &MMIWP->getMMI();
      PM.add(MMIWP);
    }
    return addPassesToEmitFile(PM, o, pi, FileType, DisableVerify, MMI);
  }

public:
  virtual bool addPassesToEmitFile(llvm::PassManagerBase &PM, llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
                                   CodeGenFileType FileType, bool DisableVerify, llvm::MachineModuleInfo *MMI) {
    return true;
  }
};

class LLVMTargetMachine : public LLVMTargetMachineBase {
public:
  using CodeGenFileType = llvm::CodeGenFileType;

protected:
  LLVMTargetMachine(const llvm::Target &T, llvm::StringRef DataLayoutString, const llvm::Triple &TargetTriple,
                    llvm::StringRef CPU, llvm::StringRef FS, const llvm::TargetOptions &Options, llvm::Reloc::Model RM,
                    llvm::CodeModel::Model CM, IGCLLVM::CodeGenOptLevel OL)
      : LLVMTargetMachineBase(T, DataLayoutString, TargetTriple, CPU, FS, Options, RM, CM, OL) {}

private:
  bool addPassesToEmitFile(llvm::PassManagerBase &PM, llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
                           llvm::CodeGenFileType FileType, bool DisableVerify,
                           llvm::MachineModuleInfoWrapperPass *MMIWP) override {
    llvm::MachineModuleInfo *MMI = nullptr;
    if (MMIWP) {
      MMI = &MMIWP->getMMI();
      PM.add(MMIWP);
    }
    return addPassesToEmitFile(PM, o, pi, FileType, DisableVerify, MMI);
  }

public:
  virtual bool addPassesToEmitFile(llvm::PassManagerBase &PM, llvm::raw_pwrite_stream &o, llvm::raw_pwrite_stream *pi,
                                   CodeGenFileType FileType, bool DisableVerify, llvm::MachineModuleInfo *MMI) {
    return true;
  }
};
} // namespace IGCLLVM

#endif
