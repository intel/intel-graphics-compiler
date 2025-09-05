/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTRTYPES_H
#define IGCLLVM_IR_INSTRTYPES_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/PatternMatch.h"

namespace IGCLLVM {
using TerminatorInst = llvm::Instruction;

namespace BinaryOperator {
inline bool isNot(const llvm::Value *V) {
  return llvm::PatternMatch::match(V, llvm::PatternMatch::m_Not(llvm::PatternMatch::m_Value()));
}
} // namespace BinaryOperator

inline void removeFnAttr(llvm::CallInst *CI, llvm::Attribute::AttrKind Kind) {
  CI->removeFnAttr(Kind);
}

inline void addFnAttr(llvm::CallInst *CI, llvm::Attribute::AttrKind Kind) {
  CI->addFnAttr(Kind);
}

inline void addFnAttr(llvm::CallInst *CI, llvm::Attribute Attr) { addFnAttr(CI, Attr.getKindAsEnum()); }

inline uint64_t getRetDereferenceableBytes(llvm::CallBase *Call) {
  return Call->getRetDereferenceableBytes();
}
} // namespace IGCLLVM

#endif
