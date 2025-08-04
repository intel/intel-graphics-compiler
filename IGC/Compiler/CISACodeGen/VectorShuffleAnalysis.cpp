/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/VectorShuffleAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/debug/Debug.hpp"

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Value.h"
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace IGC::Debug;

char VectorShuffleAnalysis::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-vector-shuffle-analysis"
#define PASS_DESCRIPTION "Recognizes lowered vector shuffle and vectorization patterns"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(VectorShuffleAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(VectorShuffleAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

VectorShuffleAnalysis::VectorShuffleAnalysis() : FunctionPass(ID) {
  initializeVectorShuffleAnalysisPass(*PassRegistry::getPassRegistry());
};

std::unique_ptr<DestVector>
VectorShuffleAnalysis::tryCreatingDestVectorForShufflePattern(llvm::InsertElementInst *DestVec) {
  ExtractElementInst *EE = dyn_cast<ExtractElementInst>(DestVec->getOperand(1));
  if (!EE)
    return nullptr;
  Instruction *Source = dyn_cast<Instruction>(EE->getVectorOperand());
  if (!Source)
    return nullptr;
  auto SourceVectorType = dyn_cast<IGCLLVM::FixedVectorType>(Source->getType());
  if (!SourceVectorType)
    return nullptr;
  auto SourceElemType = SourceVectorType->getElementType();
  if (!SourceElemType->isSingleValueType())
    return nullptr;
  auto DestVectorType = dyn_cast<IGCLLVM::FixedVectorType>(DestVec->getType());
  if (!DestVectorType)
    return nullptr;

  auto VecSize = DestVectorType->getNumElements();
  std::vector<int> ShuffleMask(VecSize, -1);
  std::vector<InsertElementInst *> IEs;
  std::vector<ExtractElementInst *> EEs;

  InsertElementInst *CurrentIE = DestVec;

  for (;;) {
    auto *Idx = dyn_cast<ConstantInt>(CurrentIE->getOperand(2));
    if (!Idx)
      break;
    auto IdxVal = Idx->getZExtValue();
    if (ShuffleMask[IdxVal] != -1)
      return nullptr;

    ExtractElementInst *CurrentEE = dyn_cast<ExtractElementInst>(CurrentIE->getOperand(1));
    if (!CurrentEE || (CurrentEE->getOperand(0) != Source) || !CurrentEE->hasOneUse())
      return nullptr;

    auto *IdxEE = dyn_cast<ConstantInt>(CurrentEE->getOperand(1));
    if (!IdxEE)
      break;
    auto IdxEEVal = IdxEE->getZExtValue();

    ShuffleMask[IdxVal] = IdxEEVal;
    IEs.push_back(CurrentIE);
    EEs.push_back(CurrentEE);

    User *U = IGCLLVM::getUniqueUndroppableUser(CurrentIE);
    if (!U)
      break;

    CurrentIE = dyn_cast<InsertElementInst>(U);
    if (!CurrentIE)
      break;
  }

  if (std::all_of(ShuffleMask.begin(), ShuffleMask.end(), [](int i) { return i != -1; })) {
    IGC_ASSERT(ShuffleMask.size() == VecSize);
    IGC_ASSERT(ShuffleMask.size() > 0);
    return std::make_unique<DestVector>(Source, DestVec, ShuffleMask, IEs, EEs, std::vector<Value *>{});
  }

  return nullptr;
}

std::unique_ptr<DestVector>
VectorShuffleAnalysis::tryCreatingDestVectorForVectorization(llvm::InsertElementInst *DestVec) {
  // Group of InsertElement instructions, each inserted value is of a scalar type (no ExtractElement)
  // and the index is a constant integer.

  auto DestVectorType = dyn_cast<IGCLLVM::FixedVectorType>(DestVec->getType());
  if (!DestVectorType)
    return nullptr;

  auto VecSize = DestVectorType->getNumElements();

  std::vector<InsertElementInst *> IEs;
  std::vector<Value *> Scalars;
  std::vector<int> ShuffleMask(VecSize, -1);

  InsertElementInst *CurrentIE = DestVec;

  for (;;) {
    auto *Idx = dyn_cast<ConstantInt>(CurrentIE->getOperand(2));
    if (!Idx)
      break;
    auto IdxVal = Idx->getZExtValue();
    if (ShuffleMask[IdxVal] != -1)
      return nullptr;

    llvm::Value *Scalar = CurrentIE->getOperand(1);

    if (!Scalar->getType()->isSingleValueType())
      return nullptr;

    if (Instruction *EE = dyn_cast<ExtractElementInst>(Scalar)) {
      // allow only vector of 1 element
      Type *EEVectorType = EE->getOperand(0)->getType();
      auto EEVectorTypeVec = dyn_cast<IGCLLVM::FixedVectorType>(EEVectorType);
      IGC_ASSERT(EEVectorTypeVec);
      if (EEVectorTypeVec->getNumElements() != 1)
        return nullptr;
    }

    ShuffleMask[IdxVal] = IdxVal;
    IEs.push_back(CurrentIE);
    Scalars.push_back(Scalar);

    User *U = IGCLLVM::getUniqueUndroppableUser(CurrentIE);
    if (!U)
      break;

    CurrentIE = dyn_cast<InsertElementInst>(U);
    if (!CurrentIE)
      break;
  }

  if (std::all_of(ShuffleMask.begin(), ShuffleMask.end(), [](int i) { return i != -1; })) {
    IGC_ASSERT(ShuffleMask.size() == VecSize);
    IGC_ASSERT(ShuffleMask.size() > 0);
    return std::make_unique<DestVector>(nullptr, DestVec, ShuffleMask, IEs, std::vector<llvm::ExtractElementInst *>{},
                                        Scalars);
  }

  return nullptr;
}

std::unique_ptr<VectorToScalarsPattern>
VectorShuffleAnalysis::tryCreatingVectorToScalarsPattern(llvm::ExtractElementInst *EE) {
  auto *Vec = EE->getVectorOperand();
  if (!Vec)
    return nullptr;

  auto DestVectorType = dyn_cast<IGCLLVM::FixedVectorType>(Vec->getType());
  if (!DestVectorType)
    return nullptr;

  auto VecSize = DestVectorType->getNumElements();

  std::vector<int> ShuffleMask(VecSize, -1);
  std::vector<ExtractElementInst *> EEs;
  bool AllUsesAreScalars = true;

  // iterate over uses
  for (auto *U : Vec->users()) {
    auto *EE = dyn_cast<ExtractElementInst>(U);
    if (!EE) {
      AllUsesAreScalars = false;
      continue;
    }

    auto Idx = dyn_cast<ConstantInt>(EE->getIndexOperand());
    if (!Idx)
      return nullptr;

    auto IdxVal = Idx->getZExtValue();
    if (IdxVal >= VecSize || ShuffleMask[IdxVal] != -1)
      return nullptr;

    ShuffleMask[IdxVal] = IdxVal;
    EEs.push_back(EE);

    // Check if the ExtractElementInst is used by an InsertElementInst
    for (auto *EEU : EE->users()) {
      if (isa<InsertElementInst>(EEU)) {
        return nullptr;
      }
    }
  }

  // Check if all indices are covered
  for (unsigned i = 0; i < VecSize; ++i) {
    if (ShuffleMask[i] == -1) {
      return nullptr;
    }
  }

  return std::make_unique<VectorToScalarsPattern>(Vec, EEs, AllUsesAreScalars);
}

bool VectorShuffleAnalysis::runOnFunction(llvm::Function &F) {
  for (auto &BB : F) {
    for (Instruction &I : BB) {
      if (auto *IE = dyn_cast<InsertElementInst>(&I)) {
        if (!isa<UndefValue>(IE->getOperand(0)) && !isa<ConstantAggregateZero>(IE->getOperand(0)))
          continue;

        std::unique_ptr<DestVector> DV = tryCreatingDestVectorForShufflePattern(IE);
        if (!DV)
          DV = tryCreatingDestVectorForVectorization(IE);

        if (!DV)
          continue;

        DestVectors.push_back(std::move(DV));
      } else if (auto *EE = dyn_cast<ExtractElementInst>(&I)) {
        std::unique_ptr<VectorToScalarsPattern> V2S = tryCreatingVectorToScalarsPattern(EE);
        if (!V2S)
          continue;

        VectorToScalarsPatterns.push_back(std::move(V2S));
      }
    }
  }

  // Populate ValueToDestVecMap
  for (auto &DV : DestVectors) {
    for (auto *IE : DV->IEs) {
      ValueToDestVecMap[cast<Value>(IE)] = DV.get();
    }
    for (auto *EE : DV->EEs) {
      ValueToDestVecMap[cast<Value>(EE)] = DV.get();
    }
  }

  // Populate DestVectorsForSourceVector
  for (auto &DV : DestVectors) {
    if (DV->isVectorShuffle()) {
      auto SourceVec = DV->getSourceVec();
      DestVectorsForSourceVector[SourceVec].push_back(DV.get());
    }
  }

  // Populate VectorToScalarsPatternsMap
  for (auto &V2S : VectorToScalarsPatterns) {
    for (auto *EE : V2S->getEEs()) {
      VectorToScalarsPatternsMap[cast<Value>(EE)] = V2S.get();
    }
  }

  return true;
}
