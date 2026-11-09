/*========================== begin_copyright_notice ============================

Copyright (C) 2025-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SinkPointerConstAdd.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/SmallVector.h"
#include "common/LLVMWarningsPop.hpp"

// Simple pass which sinks constant add operations in pointer calculations
// It changes following pattern:
//
//   %p1 = <base1> + const
//   %p2 = <base2> + %p1
//   %ptr = inttoptr %p2
// to
//   %p1 = <base1> + <base2>
//   %p2 = %p1 + const
//   %ptr = inttoptr %p2
//
// This helps other optimizations like GVN to eliminate redundant calculations.

using namespace llvm;

class SinkPointerConstAddPass : public llvm::FunctionPass {
public:
  SinkPointerConstAddPass() : llvm::FunctionPass(ID) {
    initializeSinkPointerConstAddPassPass(*llvm::PassRegistry::getPassRegistry());
  }
  virtual bool runOnFunction(llvm::Function &F) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }
  virtual llvm::StringRef getPassName() const override { return "SinkPointerConstAdd"; }
  static char ID;
};

#define PASS_FLAG "igc-sink-ptr-const-add"
#define PASS_DESCRIPTION "Sink pointer const add"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(SinkPointerConstAddPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// Type the searched values are in: the inttoptr operand, where wrapping is
// harmless, or a value below a zext / sext, which must not wrap.
enum class Domain { Wide, ZExt, SExt };

// Change of a value once the constant is removed: dir is its unsigned sign,
// nuwExact / nswExact tell whether the value's users keep nuw / nsw.
struct ValueShift {
  int dir = 0;
  bool nuwExact = false;
  bool nswExact = false;
};

// constInst is replaced by keptOp; afterwards old root == new root - delta.
// keepFlags lists the instructions up to the root with their valid flags.
struct SinkPlan {
  Instruction *constInst = nullptr;
  Value *keptOp = nullptr;
  int64_t delta = 0;
  ValueShift shift;
  SmallVector<std::pair<Instruction *, ValueShift>, 4> keepFlags;
};

// Lowering a sum or raising a difference cannot make an unsigned add/sub wrap.
static bool shrinksSumOrGrowsDifference(unsigned opcode, int64_t dir) {
  return (dir < 0 && opcode == Instruction::Add) || (dir > 0 && opcode == Instruction::Sub);
}

// Shift of the add/sub I once its constant operand is removed.
static ValueShift shiftOfRemovedConstant(Instruction *I) {
  ValueShift shift;
  shift.dir = I->getOpcode() == Instruction::Add ? -1 : 1;
  shift.nuwExact = I->hasNoUnsignedWrap();
  shift.nswExact = shift.nuwExact && I->hasNoSignedWrap();
  return shift;
}

// Shift of the add/sub I once its operand opIdx changes by opShift. nsw is
// only kept on top of nuw, which bounds the operands' signs.
static ValueShift shiftThroughOperand(Instruction *I, unsigned opIdx, const ValueShift &opShift) {
  auto opcode = I->getOpcode();
  ValueShift shift;
  shift.dir = (opcode == Instruction::Sub && opIdx == 1) ? -opShift.dir : opShift.dir;
  shift.nuwExact = opShift.nuwExact && I->hasNoUnsignedWrap() && shrinksSumOrGrowsDifference(opcode, shift.dir);
  shift.nswExact = shift.nuwExact && opShift.nswExact && I->hasNoSignedWrap();
  return shift;
}

static bool findConstant(Value *value, Domain domain, SinkPlan &plan);

static bool findConstantThroughCast(Instruction *I, SinkPlan &plan) {
  // Only sink within the same basic block.
  auto *opI = dyn_cast<Instruction>(I->getOperand(0));
  if (!opI || opI->getParent() != I->getParent())
    return false;

  const bool isZExt = I->getOpcode() == Instruction::ZExt;
  if (!findConstant(opI, isZExt ? Domain::ZExt : Domain::SExt, plan))
    return false;

  // A zext result stays non-negative, so its change is exact. A sext result
  // may go from -1 to 0, so nothing is proven.
  plan.shift = ValueShift();
  if (isZExt) {
    plan.shift.dir = (plan.delta > 0) - (plan.delta < 0); // 1 - 0 or 0 - 1 or 0 - 0
    plan.shift.nuwExact = true;
    plan.shift.nswExact = true;
  }
  return true;
}

// Finds the first constant in the add/sub tree at value that can be sunk,
// without modifying the IR. Below a cast, no add/sub on the way may wrap once
// it is removed.
static bool findConstant(Value *value, Domain domain, SinkPlan &plan) {
  // The value is rewritten in place, so it must have no other users.
  if (!value->hasOneUse())
    return false;

  auto *I = dyn_cast<Instruction>(value);
  if (!I)
    return false;

  const auto opcode = I->getOpcode();
  if (domain == Domain::Wide && (opcode == Instruction::ZExt || opcode == Instruction::SExt))
    return findConstantThroughCast(I, plan);

  if (opcode != Instruction::Add && opcode != Instruction::Sub)
    return false;

  if ((domain == Domain::ZExt && !I->hasNoUnsignedWrap()) || (domain == Domain::SExt && !I->hasNoSignedWrap()))
    return false;

  auto widenConstant = [domain](ConstantInt *c) -> int64_t {
    return domain == Domain::ZExt ? static_cast<int64_t>(c->getZExtValue()) : c->getSExtValue();
  };

  // I is replaced by an existing value, so nothing can wrap at this level.
  if (auto *CO = dyn_cast<ConstantInt>(I->getOperand(1))) {
    const int64_t c = widenConstant(CO);
    plan.constInst = I;
    plan.keptOp = I->getOperand(0);
    plan.delta = opcode == Instruction::Add ? -c : c;
    plan.shift = shiftOfRemovedConstant(I);
    return true;
  }

  if (auto *CO = dyn_cast<ConstantInt>(I->getOperand(0))) {
    // Cannot sink constant on the left side of subtraction
    if (opcode == Instruction::Sub)
      return false;

    plan.constInst = I;
    plan.keptOp = I->getOperand(1);
    plan.delta = -widenConstant(CO);
    plan.shift = shiftOfRemovedConstant(I);
    return true;
  }

  for (unsigned idx = 0; idx < 2; ++idx) {
    auto *opI = dyn_cast<Instruction>(I->getOperand(idx));
    if (!opI)
      continue;

    // This is a simple pass, only sink within the same basic block
    if (opI->getParent() != I->getParent())
      continue;

    const size_t keepFlagsSize = plan.keepFlags.size();
    if (!findConstant(opI, domain, plan))
      continue;

    // A change of the subtrahend changes the difference the opposite way.
    const int64_t delta = (opcode == Instruction::Sub && idx == 1) ? -plan.delta : plan.delta;
    const ValueShift shift = shiftThroughOperand(I, idx, plan.shift);

    // Below a cast, I must keep the flag matching the extension.
    const bool keepsExtFlag = domain == Domain::Wide || (domain == Domain::ZExt && shift.nuwExact) ||
                              (domain == Domain::SExt && shift.nswExact);
    if (!keepsExtFlag) {
      plan.keepFlags.resize(keepFlagsSize);
      continue;
    }

    plan.delta = delta;
    plan.shift = shift;
    plan.keepFlags.push_back({I, shift});
    return true;
  }

  return false;
}

// Sinks one constant from the tree at root into offset.
static bool sinkOneConstant(Value *root, int64_t &offset) {
  SinkPlan plan;
  if (!findConstant(root, Domain::Wide, plan))
    return false;

  for (auto &[I, keep] : plan.keepFlags) {
    if (!keep.nuwExact)
      I->setHasNoUnsignedWrap(false);
    if (!keep.nswExact)
      I->setHasNoSignedWrap(false);
  }

  plan.constInst->replaceAllUsesWith(plan.keptOp);
  plan.constInst->eraseFromParent();
  offset -= plan.delta;
  return true;
}

bool SinkPointerConstAddPass::runOnFunction(llvm::Function &F) {
  bool changed = false;
  std::vector<llvm::IntToPtrInst *> intToPtrInsts;
  intToPtrInsts.reserve(50);

  // Collect inttoptr instructions first.
  for (llvm::BasicBlock &BB : F) {
    for (llvm::Instruction &inst : BB) {
      if (llvm::IntToPtrInst *intrinsic = llvm::dyn_cast<llvm::IntToPtrInst>(&inst)) {
        intToPtrInsts.push_back(intrinsic);
      }
    }
  }

  for (auto &intrinsic : intToPtrInsts) {
    int64_t offset = 0;
    // Keep sinking constant adds until no more can be sunk
    while (sinkOneConstant(intrinsic->getOperand(0), offset)) {
      changed = true;
    }

    // If we found any constant offset, create new pointer calculation
    if (offset != 0) {
      llvm::IRBuilder<> builder(intrinsic);
      llvm::Value *newPtr = builder.CreateIntToPtr(
          builder.CreateAdd(intrinsic->getOperand(0),
                            llvm::ConstantInt::getSigned(intrinsic->getOperand(0)->getType(), offset)),
          intrinsic->getType());
      intrinsic->replaceAllUsesWith(newPtr);
      intrinsic->eraseFromParent();
    }
  }

  return changed;
}

char SinkPointerConstAddPass::ID = 0;

llvm::FunctionPass *createSinkPointerConstAddPass() { return new SinkPointerConstAddPass(); }
