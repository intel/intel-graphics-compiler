/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <GenISAIntrinsics/GenIntrinsicInst.h>
#include "WaveAllJointReduction.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#define DEBUG_TYPE "igc-wave-all-joint-reduction"

using namespace IGC;
using namespace llvm;

namespace IGC {
class WaveAllJointReductionImpl : public InstVisitor<WaveAllJointReductionImpl> {
public:
  WaveAllJointReductionImpl(Function &F) : F(F) {}
  bool run();
  void visitCallInst(CallInst &callInst);

private:
  Value *createInsertElements(SmallVector<WaveAllIntrinsic *, 16> &mergeList);
  void createExtractElements(SmallVector<WaveAllIntrinsic *, 16> &mergeList, WaveAllIntrinsic *waveAllJoint);
  Function &F;
  DenseSet<WaveAllIntrinsic *> ToDelete;
  bool Changed = false;
};

class WaveAllJointReduction : public FunctionPass {
public:
  static char ID;
  WaveAllJointReduction() : FunctionPass(ID) {}

  llvm::StringRef getPassName() const override { return "WaveAllJointReduction"; }
  bool runOnFunction(Function &F) override;
};

FunctionPass *createWaveAllJointReduction() { return new WaveAllJointReduction(); }
} // namespace IGC

Value *WaveAllJointReductionImpl::createInsertElements(SmallVector<WaveAllIntrinsic *, 16> &mergeList) {
  IRBuilder<> builder(mergeList.front());
  auto *vecType = VectorType::get(mergeList.front()->getSrc()->getType(), mergeList.size(), false);
  auto *vec =
      builder.CreateInsertElement(UndefValue::get(vecType), mergeList.front()->getSrc(), (uint64_t)0, "waveAllSrc");
  for (uint64_t i = 1; i < mergeList.size(); i++) {
    vec = builder.CreateInsertElement(vec, mergeList[i]->getSrc(), i, "waveAllSrc");
  }
  return vec;
}

void WaveAllJointReductionImpl::createExtractElements(SmallVector<WaveAllIntrinsic *, 16> &mergeList,
                                                      WaveAllIntrinsic *waveAllJoint) {
  IRBuilder<> builder(mergeList.front());
  for (uint64_t i = 0; i < mergeList.size(); i++) {
    auto *res = builder.CreateExtractElement(waveAllJoint, i, "waveAllDst");
    mergeList[i]->replaceAllUsesWith(res);
  }
}

void WaveAllJointReductionImpl::visitCallInst(CallInst &callInst) {

  if (auto *waveAllInst = dyn_cast<WaveAllIntrinsic>(&callInst)) {
    // marked as delete because it was already merged with prior insts
    if (ToDelete.count(waveAllInst)) {
      return;
    }

    // Optimization already happened, first operand is already vector
    if (waveAllInst->getSrc()->getType()->isVectorTy()) {
      return;
    }

    SmallVector<WaveAllIntrinsic *, 16> mergeList{waveAllInst};

    // For locality, only look at consecutive instructions since non-consecutive instructions may require sinking the
    // final vector WaveAll instruction to where the last joined WaveAll is to satisfy proper domination of each
    // WaveAll's Src
    // TODO: If needed, a complicated analysis could find non-consecutive WaveAll instructions that are able to
    // participate in WaveAll joint reduction, but seems like an edge case for now
    Instruction *I = waveAllInst->getNextNode();
    while (I != waveAllInst->getParent()->getTerminator()) {
      auto *nextWaveAllInst = dyn_cast<WaveAllIntrinsic>(I);
      // TODO: Can check helper lane mode here if necessary, unsure whether that changes anything
      if (!nextWaveAllInst || nextWaveAllInst->getSrc()->getType()->isVectorTy() ||
          nextWaveAllInst->getSrc()->getType() != waveAllInst->getSrc()->getType() ||
          nextWaveAllInst->getOpKind() != waveAllInst->getOpKind() ||
          nextWaveAllInst->getPredicate() != waveAllInst->getPredicate()) {
        break;
      }

      mergeList.push_back(nextWaveAllInst);

      I = I->getNextNode();
    }

    if (mergeList.size() > 1) {
      // Multiple WaveAll operations eligible to participate in joint operation
      auto *arg0 = createInsertElements(mergeList);
      IRBuilder<> builder(mergeList.front());
      Type *funcType[] = {arg0->getType()};
      Function *waveAllJointFunc =
          GenISAIntrinsic::getDeclaration(mergeList.front()->getModule(), GenISAIntrinsic::GenISA_WaveAll, funcType);

      auto *waveAllJoint = builder.CreateCall(
          waveAllJointFunc, {arg0, waveAllInst->getOperand(1), waveAllInst->getOperand(2), waveAllInst->getOperand(3)},
          "waveAllJoint");
      createExtractElements(mergeList, cast<WaveAllIntrinsic>(waveAllJoint));

      // Mark merged WaveAll ops participating in joint operation for deletion
      for (auto *mergedInst : mergeList) {
        ToDelete.insert(mergedInst);
      }
      Changed = true;
    }
  }
}

bool WaveAllJointReductionImpl::run() {
  visit(F);
  for (auto *mergedWaveAllInst : ToDelete) {
    mergedWaveAllInst->eraseFromParent();
  }
  return Changed;
}

bool WaveAllJointReduction::runOnFunction(Function &F) {
  WaveAllJointReductionImpl WorkerInstance(F);
  return WorkerInstance.run();
}

char WaveAllJointReduction::ID = 0;

#define PASS_FLAG "igc-wave-all-joint-reduction"
#define PASS_DESCRIPTION "WaveAllJointReduction"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WaveAllJointReduction, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(WaveAllJointReduction, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
