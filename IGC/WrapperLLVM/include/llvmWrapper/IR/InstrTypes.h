/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTRTYPES_H
#define IGCLLVM_IR_INSTRTYPES_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/PatternMatch.h"

namespace IGCLLVM
{
    using TerminatorInst = llvm::Instruction;

    namespace BinaryOperator
    {
        inline bool isNot(const llvm::Value *V)
        {
            return llvm::PatternMatch::match(V, llvm::PatternMatch::m_Not(llvm::PatternMatch::m_Value()));
        }
    }

    inline void removeFnAttr(llvm::CallInst *CI, llvm::Attribute::AttrKind Kind)
    {
#if LLVM_VERSION_MAJOR >= 14
        CI->removeFnAttr(Kind);
#else
        CI->removeAttribute(llvm::AttributeList::FunctionIndex, Kind);
#endif
    }

    inline void addFnAttr(llvm::CallInst *CI, llvm::Attribute::AttrKind Kind)
    {
#if LLVM_VERSION_MAJOR >= 14
        CI->addFnAttr(Kind);
#else
        CI->addAttribute(llvm::AttributeList::FunctionIndex, Kind);
#endif
    }

    inline uint64_t getRetDereferenceableBytes(llvm::CallBase* Call)
    {
#if LLVM_VERSION_MAJOR >= 14
        return Call->getRetDereferenceableBytes();
#else
        return Call->getDereferenceableBytes(llvm::AttributeList::ReturnIndex);
#endif
    }
}

#endif
