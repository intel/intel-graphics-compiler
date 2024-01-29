/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class SerializePrintMetaDataPass : public llvm::ModulePass {
private:
  llvm::raw_ostream *Stream;
  std::vector<llvm::MDNode *> allMD;

public:
  static char ID;

  SerializePrintMetaDataPass(llvm::raw_ostream *Stream = nullptr);
  bool runOnModule(llvm::Module &M) override;

  void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
    AU.setPreservesCFG();
    AU.addPreserved<MetaDataUtilsWrapper>();
  }

private:
  void PrintAllMD(llvm::Module *pM);
  void PrintNamedMD(llvm::Module *M);

  void CollectModuleMD(llvm::Module *M);
  void CollectFunctionMD(llvm::Function *Func);
  void CollectValueMD(llvm::Value *Val);
  void CollectInsideMD(llvm::MDNode *Node);
  void CollectInsideMD(llvm::Metadata *Node);
};
void initializeSerializePrintMetaDataPassPass(llvm::PassRegistry&);
} // namespace IGC
