/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// vim:ts=2:sw=2::et:

#ifndef LEGALIZER_PeepholeTypeLegalizer_H
#define LEGALIZER_PeepholeTypeLegalizer_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/IRBuilder.h"
#include "common/Types.hpp"

namespace IGC {

namespace Legalizer {

using namespace llvm;

class PeepholeTypeLegalizer : public FunctionPass, public InstVisitor<PeepholeTypeLegalizer> {
  IGCLLVM::IRBuilder<> *m_builder;
  Module *TheModule;
  Function *TheFunction;

public:
  static char ID;

  PeepholeTypeLegalizer();

  bool runOnFunction(Function &F) override;

  void visitInstruction(Instruction &I);
  void visitLoadInst(LoadInst &LI);
  void visitStoreInst(StoreInst &SI);
  void legalizePhiInstruction(Instruction &I);
  void legalizeUnaryInstruction(Instruction &I);
  void legalizeBinaryOperator(Instruction &I);
  void legalizeExtractElement(Instruction &I);
  void cleanupZExtInst(Instruction &I);
  void cleanupTruncInst(Instruction &I);
  void cleanupBitCastInst(Instruction &I);
  void cleanupBitCastTruncInst(Instruction &I);

private:
  bool NonBitcastInstructionsLegalized;
  bool CastInst_ZExtWithIntermediateIllegalsEliminated;
  bool CastInst_TruncWithIntermediateIllegalsEliminated;
  bool Bitcast_BitcastWithIntermediateIllegalsEliminated;
  bool Changed;

  bool MustLegalizeScratch = false;

  const DataLayout *DL;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  StringRef getPassName() const override { return "PeepholeTypeLegalizer"; }

  LLVMContext &getContext() const { return TheModule->getContext(); }
  Module *getModule() const { return TheModule; }
  Function *getFunction() const { return TheFunction; }
  bool isLegalInteger(unsigned int bitWidth) {
    if (bitWidth == 64)
      return true;
    else
      return DL->isLegalInteger(bitWidth);
  }
};
} // namespace Legalizer

} // namespace IGC

#endif // LEGALIZER_TYPELEGALIZER_H
