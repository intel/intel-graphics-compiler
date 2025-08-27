/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace IGC;

namespace IGC {

typedef llvm::SmallSet<llvm::Instruction *, 8> RematChainSet;

class RematChainPattern {
public:
  RematChainPattern(const RematChainSet& RematChain, llvm::Instruction *LastInst, llvm::Instruction *RematChainUser)
      : RematChain(RematChain), LastInstruction(LastInst), RematChainUser(RematChainUser) {
        IGC_ASSERT(!RematChain.empty() && "Remat chain cannot be empty");
        IGC_ASSERT(RematChainUser && "Remat chain user cannot be null");
      }

  llvm::Instruction *getFirstInst() const {
    return FirstInstruction;
  }

  llvm::Instruction *getLastInst() const {
    return LastInstruction;
  }

  llvm::Instruction *getRematTargetInst() const {
    return RematChainUser;
  }

  RematChainSet getRematChain() const {
    return RematChain;
  }

  bool isRematInst(llvm::Value *V) const {
    if (auto *Inst = llvm::dyn_cast<llvm::Instruction>(V)) {
      return RematChain.contains(Inst);
    }
    return false;
  }

  void setFirstInst(llvm::Instruction *Inst) {
    IGC_ASSERT(RematChain.count(Inst) && "First instruction must be part of the remat chain");
    FirstInstruction = Inst;
  }

private:
  RematChainSet RematChain;
  llvm::Instruction *LastInstruction = nullptr;
  llvm::Instruction *FirstInstruction = nullptr;
  llvm::Instruction *RematChainUser = nullptr;
};

class RematChainsAnalysis : public llvm::FunctionPass {
public:
  static char ID;
  virtual llvm::StringRef getPassName() const override { return "RematChainsAnalysis"; };
  RematChainsAnalysis();
  virtual ~RematChainsAnalysis() {}
  RematChainsAnalysis(const RematChainsAnalysis &) = delete;
  virtual bool runOnFunction(llvm::Function &F) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesAll(); }

  RematChainPattern *getRematChainPattern(llvm::Value *V) {
    auto It = ValueToRematChainMap.find(V);
    if (It != ValueToRematChainMap.end())
      return It->second;
    return nullptr;
  }

private:
  std::vector<std::unique_ptr<RematChainPattern>> RematChainPatterns;
  llvm::DenseMap<llvm::Value *, RematChainPattern *> ValueToRematChainMap;
};

}; // namespace IGC
