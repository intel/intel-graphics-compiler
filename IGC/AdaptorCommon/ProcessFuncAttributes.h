/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
class ModulePass;
}

llvm::ModulePass *createProcessFuncAttributesPass();

llvm::ModulePass *createProcessBuiltinMetaDataPass();

llvm::ModulePass *createInsertDummyKernelForSymbolTablePass();

llvm::ModulePass *createCleanupIndirectlyReferencedFunctionsPass();

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrappers. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ProcessFuncAttributesNPM : public llvm::PassInfoMixin<ProcessFuncAttributesNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-process-func-attributes"; }
  static bool isRequired() { return true; }
};

class ProcessBuiltinMetaDataNPM : public llvm::PassInfoMixin<ProcessBuiltinMetaDataNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-process-builtin-metaData"; }
  static bool isRequired() { return true; }
};

class InsertDummyKernelForSymbolTableNPM : public llvm::PassInfoMixin<InsertDummyKernelForSymbolTableNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-insert-dummy-kernel-for-symbol-table"; }
  static bool isRequired() { return true; }
};

class CleanupIndirectlyReferencedFunctionsNPM : public llvm::PassInfoMixin<CleanupIndirectlyReferencedFunctionsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-cleanup-indirectly-referenced-functions"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
