/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/RematChainsAnalysis.hpp"
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

char RematChainsAnalysis::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-remat-chain-analysis"
#define PASS_DESCRIPTION "Recognizes rematerialization chain patterns"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(RematChainsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RematChainsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

RematChainsAnalysis::RematChainsAnalysis() : FunctionPass(ID) {
  initializeRematChainsAnalysisPass(*PassRegistry::getPassRegistry());
};

static bool hasRematMetadata(llvm::Value *V) {
  if (auto *I = dyn_cast<Instruction>(V)) {
    if ([[maybe_unused]] auto *MD = I->getMetadata("remat")) {
      return true;
    }
  }
  return false;
}

Value *getRematedOperand(llvm::Instruction *I) {
  if (!I)
    return nullptr;

  // Check if the instruction is a Load or Store and return the address operand
  if (auto *LI = dyn_cast<LoadInst>(I)) {
    return LI->getPointerOperand();
  } else if (auto *SI = dyn_cast<StoreInst>(I)) {
    return SI->getPointerOperand();
  } else if (auto *SelI = dyn_cast<SelectInst>(I)) {
    // For SelectInst, return the condition operand
    return SelI->getCondition();
  }

  // If it's not a Load or Store, return nullptr
  return nullptr;
}

// There are many passes that could change the remat chain,
// So we consider not only the instructions that has remat metadata,
// But also the instructions that have only one undroppable user,
// which will be the load/store or the instruction that is rematerialized
RematChainSet getRematChain(Value *V, Instruction *User) {
  if (!V)
    return {};
  Instruction *I = dyn_cast<Instruction>(V);
  if (!I)
    return {};
  if (!User)
    return {};

  if (!isa<IntToPtrInst>(I) && !isa<AddrSpaceCastInst>(I) && !isa<BitCastInst>(I) && !isa<GetElementPtrInst>(I) &&
      !isa<BinaryOperator>(I) && !isa<UnaryOperator>(I) && !isa<CmpInst>(I)) {
    return {};
  }

  if (I->getParent() != User->getParent()) {
    return {};
  }

  RematChainSet Chain;
  if ((IGCLLVM::getUniqueUndroppableUser(I) == User) || (hasRematMetadata(I))) {
    Chain.insert(I);

    for (auto &Op : I->operands()) {
      Value *OpV = Op.get();
      if (auto *OpI = dyn_cast<Instruction>(OpV)) {
        auto SubChain = getRematChain(OpI, I);
        Chain.insert(SubChain.begin(), SubChain.end());
      }
    }
  }

  return Chain;
}

bool RematChainsAnalysis::runOnFunction(llvm::Function &F) {
  for (auto &BB : F) {
    for (auto IIt = BB.rbegin(), IE = BB.rend(); IIt != IE; ++IIt) {
      Instruction &I = *IIt;

      Value *Operand = getRematedOperand(&I);
      if (!Operand)
        continue;

      Instruction *AI = dyn_cast<Instruction>(Operand);
      if (!AI)
        continue;

      if (AI->getParent() != &BB)
        continue;

      if (ValueToRematChainMap.find(AI) != ValueToRematChainMap.end())
        continue;

      RematChainSet Chain = getRematChain(Operand, &I);

      if (!Chain.empty()) {
        RematChainPatterns.push_back(std::make_unique<RematChainPattern>(Chain, AI, &I));
        for (auto *Inst : Chain) {
          ValueToRematChainMap[Inst] = RematChainPatterns.back().get();
        }
      }
    }
    for (Instruction &I : BB) {
      if (RematChainPattern *Pattern = getRematChainPattern(&I)) {
        if (Pattern->getFirstInst() == nullptr) {
          Pattern->setFirstInst(&I);
        }
      }
    }
  }

#ifdef _DEBUG
  for (const auto &Pattern : RematChainPatterns) {
    IGC_ASSERT(Pattern->getFirstInst() != nullptr && "Remat chain pattern must have a first instruction");
    IGC_ASSERT(Pattern->getLastInst() != nullptr && "Remat chain pattern must have a last instruction");
    IGC_ASSERT(Pattern->getRematTargetInst() != nullptr && "Remat chain pattern must have a remat target instruction");
  }
#endif

  return true;
}