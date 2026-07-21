/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTVISITOR_H
#define IGCLLVM_IR_INSTVISITOR_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstVisitor.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"

namespace IGCLLVM {

template <typename Derived, typename RetTy = void> class InstVisitor : public llvm::InstVisitor<Derived, RetTy> {
#if LLVM_VERSION_MAJOR < 23
public:
  RetTy visitCondBrInst(IGCLLVM::CondBrInst &I) {
    return static_cast<Derived *>(this)->visitTerminator(static_cast<llvm::Instruction &>(I));
  }
  RetTy visitUncondBrInst(IGCLLVM::UncondBrInst &I) {
    return static_cast<Derived *>(this)->visitTerminator(static_cast<llvm::Instruction &>(I));
  }

  RetTy visitBranchInst(llvm::BranchInst &I) {
    if (I.isConditional())
      return static_cast<Derived *>(this)->visitCondBrInst(*llvm::cast<IGCLLVM::CondBrInst>(&I));
    return static_cast<Derived *>(this)->visitUncondBrInst(*llvm::cast<IGCLLVM::UncondBrInst>(&I));
  }
#endif
};

} // namespace IGCLLVM

#endif
