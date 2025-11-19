/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

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

#include "GenISAIntrinsics/GenIntrinsicInst.h"

// Simple pass which sinks constant add operations in pointer calculations
// It changes following pattern:
// %addr_part1 = <base1> + const
// %addr_part2 = <base2> + %addr_part1
// %ptr = inttoptr %addr_part2
// to:
// %addr_part1 = <base1> + <base2>
// %addr_part2 = <addr_part1> + const
// %ptr = inttoptr %addr_part2

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
  bool getConstantOffset(llvm::Value *value, std::vector<llvm::Instruction *> &zexts, int &offset);
  void zextToSext(std::vector<llvm::Instruction *> &zexts);
  bool skipZextToSext(llvm::Instruction *op, llvm::BasicBlock *parentBB);
};

#define PASS_FLAG "igc-sink-ptr-const-add"
#define PASS_DESCRIPTION "Sink pointer const add"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(SinkPointerConstAddPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool SinkPointerConstAddPass::getConstantOffset(llvm::Value *value, std::vector<llvm::Instruction *> &zexts,
                                                int &offset) {
  // Recursively search for constant add operations - this will stop after the first const add found,
  // and should be called repeatedly until no more const adds can be sunk.

  if (value->getNumUses() > 1) {
    return false; // We cannot sink constant add if the value is used more than once.
  }

  if (llvm::Instruction *instr = llvm::dyn_cast<llvm::Instruction>(value)) {
    if (instr->getOpcode() == llvm::Instruction::Trunc || instr->getOpcode() == llvm::Instruction::ZExt ||
        instr->getOpcode() == llvm::Instruction::SExt) {
      // Skip cast instructions
      llvm::Instruction *op = llvm::dyn_cast<llvm::Instruction>(instr->getOperand(0));
      // This is a simple pass, only sink within the same basic block
      if (op && instr->getParent() == op->getParent()) {
        // Collect zext instructions for later processing
        if (instr->getOpcode() == llvm::Instruction::ZExt) {
          zexts.push_back(instr);
        }
        return getConstantOffset(instr->getOperand(0), zexts, offset);
      } else {
        return false;
      }
    } else if (instr->getOpcode() == llvm::Instruction::Add || instr->getOpcode() == llvm::Instruction::Sub) {
      if (llvm::ConstantInt *constInt = llvm::dyn_cast<llvm::ConstantInt>(instr->getOperand(1))) {
        offset = instr->getOpcode() == llvm::Instruction::Add ? offset + constInt->getSExtValue()
                                                              : offset - constInt->getSExtValue();
        instr->replaceAllUsesWith(instr->getOperand(0));
        instr->eraseFromParent();
        return true;
      } else if (llvm::ConstantInt *constInt = llvm::dyn_cast<llvm::ConstantInt>(instr->getOperand(0))) {
        if (instr->getOpcode() == llvm::Instruction::Sub) {
          // Cannot sink constant on the left side of subtraction
          return false;
        }
        offset += constInt->getSExtValue();
        instr->replaceAllUsesWith(instr->getOperand(1));
        instr->eraseFromParent();
        return true;
      } else {
        llvm::Instruction *op0 = llvm::dyn_cast<llvm::Instruction>(instr->getOperand(0));
        llvm::Instruction *op1 = llvm::dyn_cast<llvm::Instruction>(instr->getOperand(1));
        // This is a simple pass, only sink within the same basic block
        return (op0 && instr->getParent() == op0->getParent() && getConstantOffset(op0, zexts, offset)) ? true
               : (op1 && instr->getParent() == op1->getParent()) ? getConstantOffset(op1, zexts, offset)
                                                                 : false;
      }
    }
  }

  return false;
}

bool SinkPointerConstAddPass::skipZextToSext(llvm::Instruction *op, llvm::BasicBlock *parentBB) {
  // This is a simple pass, only sink within the same basic block
  if (op && parentBB == op->getParent()) {
    // Do not change zext of pushed constants or loaded values - UMD provides unsigned offsets
    if (llvm::GenIntrinsicInst *instr = llvm::dyn_cast<llvm::GenIntrinsicInst>(op)) {
      if (instr->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_RuntimeValue) {
        return true;
      }
    } else if (llvm::dyn_cast<llvm::Argument>(op) || llvm::dyn_cast<llvm::LoadInst>(op)) {
      return true;
    } else if (llvm::BinaryOperator *bo = llvm::dyn_cast<BinaryOperator>(op)) {
      llvm::ConstantInt *cOp0 = llvm::dyn_cast<llvm::ConstantInt>(bo->getOperand(0));
      llvm::ConstantInt *cOp1 = llvm::dyn_cast<llvm::ConstantInt>(bo->getOperand(1));
      if ((cOp0 && cOp0->isNegative()) || (cOp1 && cOp1->isNegative())) {
        return false;
      } else if (bo->getOpcode() == llvm::Instruction::Sub) {
        return false;
      } else {
        return (skipZextToSext(llvm::dyn_cast<llvm::Instruction>(bo->getOperand(0)), parentBB) &&
                skipZextToSext(llvm::dyn_cast<llvm::Instruction>(bo->getOperand(1)), parentBB));
      }
    } else {
      return false;
    }
  }
  return true;
}

void SinkPointerConstAddPass::zextToSext(std::vector<llvm::Instruction *> &zexts) {
  // Remove duplicates
  std::sort(zexts.begin(), zexts.end());
  zexts.erase(std::unique(zexts.begin(), zexts.end()), zexts.end());
  // Convert zext instructions to sext instructions
  for (auto &zext : zexts) {
    llvm::Instruction *op = llvm::dyn_cast<llvm::Instruction>(zext->getOperand(0));
    if (skipZextToSext(op, zext->getParent())) {
      continue;
    }
    llvm::IRBuilder<> builder(zext);
    llvm::Value *sext = builder.CreateSExt(zext->getOperand(0), zext->getType());
    zext->replaceAllUsesWith(sext);
    zext->eraseFromParent();
  }
}

bool SinkPointerConstAddPass::runOnFunction(llvm::Function &F) {
  bool changed = false;
  std::vector<llvm::IntToPtrInst *> intToPtrInsts;
  intToPtrInsts.reserve(50);

  // Collect all inttoptr instructions first
  for (llvm::BasicBlock &BB : F) {
    for (llvm::Instruction &inst : BB) {
      if (llvm::IntToPtrInst *intrinsic = llvm::dyn_cast<llvm::IntToPtrInst>(&inst)) {
        intToPtrInsts.push_back(intrinsic);
      }
    }
  }

  for (auto &intrinsic : intToPtrInsts) {
    int offset = 0;
    bool localChanged = false;
    std::vector<llvm::Instruction *> zexts;
    // Keep sinking constant adds until no more can be sunk
    while (getConstantOffset(intrinsic->getOperand(0), zexts, offset)) {
      localChanged = true;
      changed = true;
    }

    if (localChanged) {
      // In some cases, sinking constant add may introduce negative values in pointer calculations.
      // Convert affected zext instructions to sext instructions to avoid potential issues.
      zextToSext(zexts);
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