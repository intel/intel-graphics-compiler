/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "llvmWrapper/IR/DerivedTypes.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

/// The vector widths OpenCL C defines, and BiF implements vector builtins for.
inline bool isOpenCLVectorWidth(unsigned NumElements) {
  return NumElements == 2 || NumElements == 3 || NumElements == 4 || NumElements == 8 || NumElements == 16;
}

/// Emits an operation on N-wide vectors element by element in place of \p I:
/// unrolled up to MaxUnrolledWidth elements, otherwise as a loop.
class PerElementEmitter {
public:
  // BiF unrolls its vector variants up to 16 elements. Unrolling further
  // inlines the scalar BiF body once per element: pow on 1024 floats took over
  // 250 s. Wider vectors get a loop, which LoopUnroll still unrolls after BiF
  // inlining if the body is small.
  static constexpr unsigned MaxUnrolledWidth = 16;

  PerElementEmitter(llvm::Instruction &I, unsigned NumElements) : I(I), NumElements(NumElements), B(&I) {}

  /// Inserts before \p I, or into the loop body while a callback runs.
  llvm::IRBuilder<> &builder() { return B; }

  bool isLoop() const { return NumElements > MaxUnrolledWidth; }

  /// Calls \p Body for each element index and returns the vector of its
  /// results, or null if \p ResultTy is null.
  llvm::Value *emit(IGCLLVM::FixedVectorType *ResultTy, llvm::function_ref<llvm::Value *(llvm::Value *Lane)> Body) {
    if (!isLoop()) {
      llvm::Value *Result = ResultTy ? llvm::PoisonValue::get(ResultTy) : nullptr;
      for (unsigned L = 0; L < NumElements; ++L) {
        llvm::Value *R = Body(B.getInt32(L));
        if (ResultTy)
          Result = B.CreateInsertElement(Result, R, B.getInt32(L));
      }
      return Result;
    }
    llvm::PHINode *Lane = beginLoop();
    llvm::PHINode *Acc = nullptr;
    if (ResultTy) {
      Acc = B.CreatePHI(ResultTy, 2, "acc");
      Acc->addIncoming(llvm::PoisonValue::get(ResultTy), Head);
    }
    llvm::Value *R = Body(Lane);
    llvm::Value *Result = nullptr;
    if (Acc) {
      Result = B.CreateInsertElement(Acc, R, Lane);
      Acc->addIncoming(Result, B.GetInsertBlock());
    }
    endLoop(Lane, NumElements);
    return Result;
  }

  /// Folds \p Count steps into \p Init: Acc = Step(Acc, S) for S = 0 .. Count - 1.
  /// Returns the final accumulator.
  llvm::Value *reduce(llvm::Value *Init, unsigned Count,
                      llvm::function_ref<llvm::Value *(llvm::Value *Acc, llvm::Value *S)> Step) {
    if (!isLoop()) {
      llvm::Value *Acc = Init;
      for (unsigned S = 0; S < Count; ++S)
        Acc = Step(Acc, B.getInt32(S));
      return Acc;
    }
    llvm::PHINode *S = beginLoop();
    llvm::PHINode *Acc = B.CreatePHI(Init->getType(), 2, "acc");
    Acc->addIncoming(Init, Head);
    llvm::Value *Result = Step(Acc, S);
    Acc->addIncoming(Result, B.GetInsertBlock());
    endLoop(S, Count);
    return Result;
  }

private:
  llvm::Instruction &I;
  unsigned NumElements;
  llvm::IRBuilder<> B;
  llvm::BasicBlock *Head = nullptr;
  llvm::BasicBlock *Loop = nullptr;
  llvm::BasicBlock *Tail = nullptr;

  // Splits the block at I and returns the counter of a loop placed before it,
  // with the builder in the loop body.
  llvm::PHINode *beginLoop() {
    Head = I.getParent();
    Tail = Head->splitBasicBlock(I.getIterator(), "long.vector.tail");
    Loop = llvm::BasicBlock::Create(I.getContext(), "long.vector.loop", I.getFunction(), Tail);
    Head->getTerminator()->setSuccessor(0, Loop);
    B.SetInsertPoint(Loop);
    llvm::PHINode *Counter = B.CreatePHI(B.getInt32Ty(), 2, "lane");
    Counter->addIncoming(B.getInt32(0), Head);
    return Counter;
  }

  void endLoop(llvm::PHINode *Counter, unsigned TripCount) {
    llvm::Value *Next = B.CreateAdd(Counter, B.getInt32(1));
    Counter->addIncoming(Next, B.GetInsertBlock());
    B.CreateCondBr(B.CreateICmpULT(Next, B.getInt32(TripCount)), Loop, Tail);
    B.SetInsertPoint(&I);
  }
};

} // namespace IGC
