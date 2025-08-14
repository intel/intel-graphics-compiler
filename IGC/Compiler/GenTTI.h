/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvmWrapper/Analysis/TargetTransformInfo.h"
#include "common/LLVMWarningsPop.hpp"

namespace llvm {

// This implementation allows us to define our own costs for the GenIntrinsics
// Did not use BasicTTIImplBase because the overloaded constructors have TragetMachine as an argument,
// so I inherited from its parent which has only DL as its arguments
class GenIntrinsicsTTIImpl : public IGCLLVM::TTIImplCRTPBase<GenIntrinsicsTTIImpl> {
  typedef IGCLLVM::TTIImplCRTPBase<GenIntrinsicsTTIImpl> BaseT;
  typedef TargetTransformInfo TTI;
  friend BaseT;
  IGC::CodeGenContext *ctx;

public:
  GenIntrinsicsTTIImpl(IGC::CodeGenContext *pCtx) : BaseT(pCtx->getModule()->getDataLayout()), ctx(pCtx) {}

  DenseMap<Value *, bool> isGEPLoopInduction;

  bool shouldBuildLookupTables();

  bool isLoweredToCall(const Function *F);

  void *getAdjustedAnalysisPointer(const void *ID);

  // [POC] Implemented to use InferAddressSpaces pass after
  // PrivateMemoryToSLM pass to propagate ADDRESS_SPACE_PRIVATE
  // from variables to memory operations.
  unsigned getFlatAddressSpace();

  bool isGEPLoopConstDerived(GetElementPtrInst *GEP, const Loop *L, ScalarEvolution &SE);

  void getUnrollingPreferences(Loop *L, ScalarEvolution &SE, TTI::UnrollingPreferences &UP,
                               OptimizationRemarkEmitter *ORE);

  void getPeelingPreferences(Loop *L, ScalarEvolution &SE, TTI::PeelingPreferences &PP);

  bool isProfitableToHoist(Instruction *I);

  llvm::InstructionCost getUserCost(const User *U, ArrayRef<const Value *> Operands, TTI::TargetCostKind CostKind);

  llvm::InstructionCost getInstructionCost(const User *U, ArrayRef<const Value *> Operands,
                                           TTI::TargetCostKind CostKind);

private:
  llvm::InstructionCost internalCalculateCost(const User *U, ArrayRef<const Value *> Operands,
                                              TTI::TargetCostKind CostKind);
};

unsigned getLoopSize(const Loop *L, const TargetTransformInfo &TTI);

} // namespace llvm
