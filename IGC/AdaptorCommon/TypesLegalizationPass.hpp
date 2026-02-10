/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>
#include "Compiler/IGCPassSupport.h"

class TypesLegalizationPass : public llvm::FunctionPass, public llvm::InstVisitor<TypesLegalizationPass> {

public:
  TypesLegalizationPass();
  ~TypesLegalizationPass() {}

  virtual llvm::StringRef getPassName() const override { return "Types Legalization Pass"; }

  bool LegalizeTypes();
  virtual bool runOnFunction(llvm::Function &function) override;
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

  static char ID;
  llvm::SmallVector<llvm::StoreInst *, 10> m_StoreInst;
  llvm::SmallVector<llvm::ExtractValueInst *, 10> m_ExtractValueInst;
  llvm::SmallVector<llvm::PHINode *, 10> m_PhiNodes;
  llvm::SmallVector<unsigned, 8> Indicies;
};
