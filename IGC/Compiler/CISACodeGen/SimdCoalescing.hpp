/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class SimdCoalescing : public llvm::FunctionPass {
public:
  static char ID;

  static constexpr unsigned MaxCoalescingWidth = 8;

  SimdCoalescing();

  llvm::StringRef getPassName() const override { return "SimdCoalescing"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<WIAnalysis>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnFunction(llvm::Function &F) override;

private:
  WIAnalysis *WI = nullptr;
  bool Changed = false;

  bool isCoalescable(llvm::Instruction *I) const {
    if (I->getType()->isVectorTy() || I->getType()->isVoidTy() || I->getType()->isIntegerTy(1) ||
        I->getType()->isIntegerTy(64) || I->getType()->isDoubleTy()) {
      return false;
    }

    if (auto *BO = llvm::dyn_cast<llvm::BinaryOperator>(I)) {
      auto Op = BO->getOpcode();
      switch (Op) {
      case llvm::Instruction::Add:
        // TODO: Plan to add support for more opcodes if this proves to be beneficial,
        // and avoid intriducing regressions on spill/fill codegen.
        // case llvm::Instruction::FAdd:
        // case llvm::Instruction::Sub:
        // case llvm::Instruction::FSub:
        // case llvm::Instruction::Mul:
        // case llvm::Instruction::FMul:
        // case llvm::Instruction::Shl:
        // case llvm::Instruction::LShr:
        // case llvm::Instruction::AShr:
        return true;

      default:
        break;
      }
    }
    return false;
  }

  static bool areCompatible(llvm::Instruction *A, llvm::Instruction *B);
  static bool hasNoIntraBundleDeps(llvm::ArrayRef<llvm::Instruction *> Bundle);
  static bool isProfitable(llvm::ArrayRef<llvm::Instruction *> Bundle);
  static bool canAvoidScatterExtract(llvm::ArrayRef<llvm::Instruction *> Bundle);

  bool vectorizeBundle(llvm::ArrayRef<llvm::Instruction *> Bundle, llvm::LLVMContext &Ctx);
  void coalesceInstructions(llvm::SmallVectorImpl<llvm::Instruction *> &Candidates, llvm::LLVMContext &Ctx,
                            unsigned EffectiveMaxWidth);

  unsigned getEffectiveMaxWidth(CodeGenContext *CGCtx) const {
    unsigned GRFCount = CGCtx->platform.getMaxNumGRF(ShaderType::COMPUTE_SHADER);
    if (GRFCount >= 256)
      return MaxCoalescingWidth;
    return 2;
  }
};

} // namespace IGC
