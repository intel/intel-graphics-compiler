/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/IntDivRemCombine.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/igc_regkeys.hpp"

#include <cmath>

using namespace llvm;

// This replaces the remainder of div/rem pairs with same operands with
// a multiply and subtract.  This presumes that integer division/remainder
// are significantly more expensive than multiplication and substract.
//  Given n/d:   n = q*d + r ==> r = n - q*d
//
//  I.e.
//   %q = sdiv i32 %n, %d
//   ... stuff
//   %r1 = srem i32 %n, %d
//   ...
//   %r2 = srem i32 %n, %d
// ==>
//
//   %q   = sdiv i32 %n, %d
//   %tmp = imul i32 %q, %d
//   %r1  = isub i32 %n, %tmp
//   %r2  = isub i32 %n, %tmp
//   ... stuff
//
// NOTE: this also handles the case where rem precedes div
// I.e.
//   %r = srem i32 %n, %d
//   ... stuff
//   %q = sdiv i32 %n, %d
//
// NOTE: we constrain this to a basic block at the moment
//
struct IntDivRemCombine : public FunctionPass {
  static char ID;

  int options;

  IntDivRemCombine();

  /// @brief  Provides name of pass
  virtual StringRef getPassName() const override { return "IntDivRemCombine"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    //    AU.addRequired<MetaDataUtilsWrapper>();
  }

  virtual bool runOnFunction(Function &F) override {
    bool changed = false;

    for (Function::iterator bi = F.begin(), bie = F.end(); bi != bie; ++bi) {
      SmallVector<BinaryOperator *, 4> divs;
      BasicBlock &BB = *bi;
      bool blockChanged;
      do {
        blockChanged = false;
        for (BasicBlock::iterator ii = BB.begin(), ie = BB.end(); ii != ie && !blockChanged; ii++) {
          Instruction *I = &*ii;
          switch (I->getOpcode()) {
          case Instruction::SDiv:
          case Instruction::UDiv:
            if (shouldSimplify(cast<BinaryOperator>(I))) {
              blockChanged |= replaceAllRemsInBlock(ii, ie);
              break;
            }
          case Instruction::SRem:
          case Instruction::URem:
            if (shouldSimplify(cast<BinaryOperator>(I))) {
              blockChanged |= hoistMatchingDivAbove(ii, ie);
              break;
            }
          }
        }
        changed |= blockChanged;
      } while (blockChanged);
    }

#if 0
        struct Merge {
            BinaryOperator *anchor = nullptr;
            //
            BinaryOperator *div = nullptr;
            SmallVector<BinaryOperator*,16> rems;
        };
        // for global we could
        DominatorTree DT;
        DT.recalculate(F);
#endif
    return changed;
  } // runOnFunction

  bool shouldSimplify(BinaryOperator *b) const {
    if (ConstantInt *divisor = dyn_cast<ConstantInt>(b->getOperand(1))) {
      return !divisor->getValue().isPowerOf2();
    } else {
      return true;
    }
  }

  // replace all successive rems with divs
  bool replaceAllRemsInBlock(BasicBlock::iterator ii, BasicBlock::iterator ie) const {
    bool changed = false;

    SmallVector<Instruction *, 4> deleteMe;

    BinaryOperator *Q = cast<BinaryOperator>(&*ii);
    auto targetOp = Q->getOpcode() == Instruction::SDiv ? Instruction::SRem : Instruction::URem;
    for (; ii != ie; ii++) {
      Instruction *R = &*ii;
      // TODO: can we use match(...) here?
      // (c.f. PatternMatch.hpp)
      if (R->getOpcode() == targetOp && Q->getOperand(0) == R->getOperand(0) && Q->getOperand(1) == R->getOperand(1)) {
        emulateRem(Q, R);
        //
        deleteMe.push_back(R);
        //
        changed = true;
      }
    }

    // delete all the replaced instructions
    for (auto *R : deleteMe) {
      R->eraseFromParent();
    }

    return changed;
  }

  // find a matching divide to place above us
  // (we'll restart the block)
  //
  // E.g. given
  //   %R = srem %N, %D
  //   ...
  //
  // scan for a
  //   %Q = sdiv %N, %D
  // down below and move it above the remainder op
  bool hoistMatchingDivAbove(BasicBlock::iterator ii, BasicBlock::iterator ie) const {
    BinaryOperator *R = cast<BinaryOperator>(&*ii);
    auto targetOp = R->getOpcode() == Instruction::SRem ? Instruction::SDiv : Instruction::UDiv;
    for (; ii != ie; ii++) {
      Instruction *Q = &*ii;
      // TODO: can we use match(...) here?
      // (c.f. PatternMatch.hpp)
      if (Q->getOpcode() == targetOp && Q->getOperand(0) == R->getOperand(0) && Q->getOperand(1) == R->getOperand(1)) {
        Q->removeFromParent();
        Q->insertBefore(R);

        // since we're here, we'll fix this one
        emulateRem(Q, R);
        R->eraseFromParent();

        // return to restart iteration over the block
        return true;
      }
    }

    return false;
  }

  // assumes Q precedes R
  void emulateRem(Instruction *Q, Instruction *R) const {
    IRBuilder<> B(R);
    auto *Q_D = B.CreateMul(Q, Q->getOperand(1));
    auto *R1 = B.CreateSub(Q->getOperand(0), Q_D, R->getName());
    //
    R->replaceAllUsesWith(R1);
    R->dropAllReferences();
  }
}; // class IntDivRemCombine

char IntDivRemCombine::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-divrem-combine"
#define PASS_DESCRIPTION "Integer Division/Remainder Combine"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(IntDivRemCombine, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(IntDivRemCombine, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

IntDivRemCombine::IntDivRemCombine() : FunctionPass(ID), options((int)(IGC_GET_FLAG_VALUE(EnableIntDivRemCombine))) {
  initializeIntDivRemCombinePass(*PassRegistry::getPassRegistry());
}

llvm::FunctionPass *IGC::createIntDivRemCombinePass() { return new IntDivRemCombine(); }
