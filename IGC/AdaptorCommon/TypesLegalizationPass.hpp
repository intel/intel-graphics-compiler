/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>
#include "Compiler/IGCPassSupport.h"

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class TypesLegalizationPass : public llvm::InstVisitor<TypesLegalizationPass> {

public:
  TypesLegalizationPass() {}
  ~TypesLegalizationPass() {}

  static llvm::StringRef getPassName() { return "Types Legalization Pass"; }

  bool LegalizeTypes();
  bool run(llvm::Function &function);
  void visitStoreInst(llvm::StoreInst &I);
  void visitExtractValueInst(llvm::ExtractValueInst &I);
  void visitPHINode(llvm::PHINode &I);
  void ResolveStoreInst(llvm::StoreInst *st);
  void ResolvePhiNode(llvm::PHINode *phi);
  void ResolveExtractValue(llvm::ExtractValueInst *extractVal);
  void ResolveStoreInst(llvm::StoreInst *st, llvm::Type *ty, llvm::SmallVector<unsigned, 8> &index);
  bool CheckNullArray(llvm::Instruction *storeInst);
  llvm::Value *ResolveValue(llvm::Instruction *st, llvm::Value *arg, llvm::SmallVector<unsigned, 8> &index);
  llvm::Value *CreateGEP(IGCLLVM::IRBuilder<> &builder, llvm::Type *Ty, llvm::Value *ptr,
                         llvm::SmallVector<unsigned, 8> &indices);
  llvm::Value *CreateGEP(IGCLLVM::IRBuilder<> &builder, llvm::Value *ptr, llvm::SmallVector<unsigned, 8> &indices);
  llvm::AllocaInst *CreateAlloca(llvm::Instruction *phi);

  llvm::SmallVector<llvm::StoreInst *, 10> m_StoreInst;
  llvm::SmallVector<llvm::ExtractValueInst *, 10> m_ExtractValueInst;
  llvm::SmallVector<llvm::PHINode *, 10> m_PhiNodes;
  llvm::SmallVector<unsigned, 8> Indicies;
};

// Legacy Pass Manager wrapper.
class TypesLegalizationPassLPM : public llvm::FunctionPass {
public:
  static char ID;

  TypesLegalizationPassLPM();
  ~TypesLegalizationPassLPM() {}

  llvm::StringRef getPassName() const override { return TypesLegalizationPass::getPassName(); }

  bool runOnFunction(llvm::Function &function) override { return TypesLegalizationPass().run(function); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper around TypesLegalizationPass. Add it to a module pass
// manager via createModuleToFunctionPassAdaptor(...). isRequired() so it runs on
// optnone functions too, matching the legacy path: the pass intentionally
// legalizes (and cleans up) optnone functions rather than skipping them.
class TypesLegalizationPassNPM : public llvm::PassInfoMixin<TypesLegalizationPassNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "types-legalization-pass"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
