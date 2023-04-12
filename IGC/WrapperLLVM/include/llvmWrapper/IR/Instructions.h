/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTRUCTIONS_H
#define IGCLLVM_IR_INSTRUCTIONS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"

#include "Probe/Assertion.h"

namespace IGCLLVM
{

    inline llvm::Value* getCalledValue(llvm::CallInst& CI)
    {
#if LLVM_VERSION_MAJOR <= 10
        return CI.getCalledValue();
#else
        return CI.getCalledOperand();
#endif
    }

    inline llvm::Value* getCalledValue(llvm::CallInst* CI)
    {
#if LLVM_VERSION_MAJOR <= 10
        return CI->getCalledValue();
#else
        return CI->getCalledOperand();
#endif
    }

    inline const llvm::Value* getCalledValue(const llvm::CallInst* CI)
    {
#if LLVM_VERSION_MAJOR <= 10
        return CI->getCalledValue();
#else
        return CI->getCalledOperand();
#endif
    }

    inline unsigned getNumArgOperands(const llvm::CallInst* CI)
    {
#if LLVM_VERSION_MAJOR < 14
       return CI->getNumArgOperands();
#else
       return CI->arg_size();
#endif
    }

    inline unsigned getArgOperandNo(llvm::CallInst &CI, const llvm::Use *U) {
#if LLVM_VERSION_MAJOR < 10
      IGC_ASSERT_MESSAGE(CI.isArgOperand(U), "Arg operand # out of range!");
      return (unsigned)(U - CI.arg_begin());
#else
      return CI.getArgOperandNo(U);
#endif
    }

    inline llvm::Constant* getShuffleMaskForBitcode(llvm::ShuffleVectorInst* SVI)
    {
#if LLVM_VERSION_MAJOR < 11
        return SVI->getMask();
#else
        return SVI->getShuffleMaskForBitcode();
#endif
    }

    inline bool isFreezeInst(llvm::Instruction* I)
    {
#if LLVM_VERSION_MAJOR < 10
        (void)I;
        return false;
#else
        return llvm::isa<llvm::FreezeInst>(I);
#endif
    }
}

#endif
