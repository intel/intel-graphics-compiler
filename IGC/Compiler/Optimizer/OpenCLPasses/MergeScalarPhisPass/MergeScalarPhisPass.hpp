/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include <llvm/ADT/MapVector.h>
#include <llvm/IR/Instructions.h>

namespace IGC {

class MergeScalarPhisPass : public llvm::FunctionPass {
public:
  static char ID;

  MergeScalarPhisPass();

  virtual llvm::StringRef getPassName() const override {
    return "Merge scalar phis pass";
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  virtual bool runOnFunction(llvm::Function &F) override;

private:
  void collectPhiNodes(llvm::Function &F);
  void clearContainers();
  void cleanUpIR();
  bool makeChanges();

  llvm::MapVector<llvm::Value *, llvm::SmallVector<llvm::PHINode *, 4>>
      VectorToPhiNodesMap;
  llvm::SmallSet<llvm::PHINode *, 16> PhiNodesToRemove;
  llvm::SmallSet<llvm::ExtractElementInst *, 16> ExtrElementsToRemove;
};
} // namespace IGC