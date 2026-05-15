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

private:
  bool getConstantOffset(llvm::Value *value, int64_t &offset);
  bool sinkConstantThroughCast(llvm::Value *value, int64_t &offset, bool acrossSExt);
};

#define PASS_FLAG "igc-sink-ptr-const-add"
#define PASS_DESCRIPTION "Sink pointer const add"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(SinkPointerConstAddPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

enum class ExtensionKind {
  Signed,
  Zero,
};

template <typename Fn> static bool HandleAddSub(Instruction *I, int64_t &offset, ExtensionKind kind, Fn recurseFn) {

  auto widenConstant = [kind](ConstantInt *c) -> int64_t {
    return kind == ExtensionKind::Signed ? c->getSExtValue() : static_cast<int64_t>(c->getZExtValue());
  };

  auto opcode = I->getOpcode();

  if (auto *CO = dyn_cast<ConstantInt>(I->getOperand(1))) {

    offset += opcode == Instruction::Add ? widenConstant(CO) : -widenConstant(CO);
    I->replaceAllUsesWith(I->getOperand(0));
    I->eraseFromParent();
    return true;
  }

  if (auto *CO = dyn_cast<ConstantInt>(I->getOperand(0))) {

    if (opcode == Instruction::Sub)
      // Cannot sink constant on the left side of subtraction
      return false;

    offset += widenConstant(CO);
    I->replaceAllUsesWith(I->getOperand(1));
    I->eraseFromParent();
    return true;
  }

  for (auto *op : {I->getOperand(0), I->getOperand(1)}) {

    auto *opI = dyn_cast<Instruction>(op);
    if (!opI)
      continue;

    // This is a simple pass, only sink within the same basic block
    if (opI->getParent() != I->getParent())
      continue;

    if (!recurseFn(op, offset))
      continue;

    return true;
  }

  return false;
}

bool SinkPointerConstAddPass::getConstantOffset(Value *value, int64_t &offset) {
  // Recursively search for constant add operations - this will stop after the first const add found,
  // and should be called repeatedly until no more const adds can be sunk.

  if (!value->hasOneUse()) {
    return false; // We cannot sink constant add if the value is used more than once.
  }

  auto *I = dyn_cast<Instruction>(value);

  if (!I)
    return false;

  auto opcode = I->getOpcode();

  switch (opcode) {

  case Instruction::SExt:
  case Instruction::ZExt: {
    // check if we are leaving current block
    // if yes - bail, doing this across blocks is a net loss
    auto *operandInst = dyn_cast<Instruction>(I->getOperand(0));
    if (operandInst && operandInst->getParent() != I->getParent())
      return false;

    return sinkConstantThroughCast(I->getOperand(0), offset, opcode == Instruction::SExt);
  } break;
  case Instruction::Add:
  case Instruction::Sub:
    return HandleAddSub(I, offset, ExtensionKind::Signed,
                        [this](Value *value, int64_t &offset) { return getConstantOffset(value, offset); });
    break;
  default:
    break;
  }

  return false;
}

bool SinkPointerConstAddPass::sinkConstantThroughCast(Value *value, int64_t &offset, bool acrossSExt) {
  if (!value->hasOneUse())
    return false;

  auto *I = dyn_cast<Instruction>(value);
  if (!I)
    return false;

  auto opcode = I->getOpcode();
  if (opcode != Instruction::Add && opcode != Instruction::Sub)
    return false;

  const bool flagOK = acrossSExt ? I->hasNoSignedWrap() : I->hasNoUnsignedWrap();
  if (!flagOK)
    return false;

  return HandleAddSub(
      I, offset, acrossSExt ? ExtensionKind::Signed : ExtensionKind::Zero,
      [this, acrossSExt](Value *value, int64_t &offset) { return sinkConstantThroughCast(value, offset, acrossSExt); });
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
    while (getConstantOffset(intrinsic->getOperand(0), offset)) {
      changed = true;
    }

    // If we found any constant offset, create new pointer calculation
    if (offset != 0) {
      llvm::IRBuilder<> builder(intrinsic);
      llvm::Value *newPtr =
          builder.CreateIntToPtr(builder.CreateAdd(intrinsic->getOperand(0),
                                                   llvm::ConstantInt::get(intrinsic->getOperand(0)->getType(), offset)),
                                 intrinsic->getType());
      intrinsic->replaceAllUsesWith(newPtr);
      intrinsic->eraseFromParent();
    }
  }

  return changed;
}

char SinkPointerConstAddPass::ID = 0;

llvm::FunctionPass *createSinkPointerConstAddPass() { return new SinkPointerConstAddPass(); }
