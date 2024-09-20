/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_FUNCTION_H
#define IGCLLVM_IR_FUNCTION_H

#include <iterator>
#include "llvm/IR/Function.h"
#include "Probe/Assertion.h"

namespace IGCLLVM {

inline llvm::Argument *getArg(const llvm::Function &F, unsigned ArgNo) {
  IGC_ASSERT(F.arg_size() > ArgNo);
  llvm::Argument *Arg = nullptr;

#if LLVM_VERSION_MAJOR < 10
  // similar to lvm::Function::getArg implementation
  auto ArgIt = F.arg_begin();
  std::advance(ArgIt, ArgNo);
  Arg = const_cast<llvm::Argument *>(&*ArgIt);
#else
  Arg = F.getArg(ArgNo);
#endif

  return Arg;
}

inline void addRetAttr(llvm::Function *F, llvm::Attribute::AttrKind Kind) {
#if LLVM_VERSION_MAJOR < 14
  F->addAttribute(llvm::AttributeList::ReturnIndex, Kind);
#else
  F->addRetAttr(Kind);
#endif
}

inline void addRetAttrs(llvm::Function* F, llvm::AttrBuilder &B) {
#if LLVM_VERSION_MAJOR < 14
  F->addAttributes(llvm::AttributeList::ReturnIndex, B);
#else
  F->addRetAttrs(B);
#endif
}

inline void addFnAttrs(llvm::Function* F, llvm::AttrBuilder &B) {
#if LLVM_VERSION_MAJOR < 14
  F->addAttributes(llvm::AttributeList::FunctionIndex, B);
#else
  F->addFnAttrs(B);
#endif
}

inline bool onlyWritesMemory(llvm::Function *F) {
#if LLVM_VERSION_MAJOR < 14
  return F->doesNotReadMemory();
#else
  return F->onlyWritesMemory();
#endif
}

inline void pushBackBasicBlock(llvm::Function* F, llvm::BasicBlock* BB) {
#if LLVM_VERSION_MAJOR < 16
    F->getBasicBlockList().push_back(BB);
#else
    F->insert(F->end(), BB);
#endif
}

inline void splice(llvm::Function* pNewFunc, llvm::Function::iterator it, llvm::Function* pOldFunc) {
#if LLVM_VERSION_MAJOR < 16
    pNewFunc->getBasicBlockList().splice(it, pOldFunc->getBasicBlockList());
#else
    pNewFunc->splice(it, pOldFunc);
#endif
}

inline void splice(llvm::Function* pNewFunc, llvm::Function::iterator it, llvm::Function* pOldFunc, llvm::BasicBlock* BB) {
#if LLVM_VERSION_MAJOR < 16
    pNewFunc->getBasicBlockList().splice(it, pOldFunc->getBasicBlockList(), BB);
#else
    pNewFunc->splice(it, pOldFunc);
#endif
}

inline void splice(
    llvm::Function* pNewFunc,
    llvm::Function::iterator it,
    llvm::Function* pOldFunc,
    llvm::Function::iterator itBegin,
    llvm::Function::iterator itEnd) {
#if LLVM_VERSION_MAJOR < 16
    pNewFunc->getBasicBlockList().splice(it, pOldFunc->getBasicBlockList(), itBegin, itEnd);
#else
    pNewFunc->splice(it, pOldFunc, itBegin, itEnd);
#endif
}

inline auto rbegin(llvm::Function* pFunc) {
#if LLVM_VERSION_MAJOR < 16
    return pFunc->getBasicBlockList().rbegin();
#else
    return std::make_reverse_iterator(pFunc->end());
#endif
}

inline auto rend(llvm::Function* pFunc) {
#if LLVM_VERSION_MAJOR < 16
    return pFunc->getBasicBlockList().rend();
#else
    return std::make_reverse_iterator(pFunc->begin());
#endif
}

inline void insertBasicBlock(
    llvm::Function* pFunc,
    llvm::Function::iterator it,
    llvm::BasicBlock* BB) {
#if LLVM_VERSION_MAJOR < 16
    pFunc->getBasicBlockList().insert(it, BB);
#else
    pFunc->insert(it, BB);
#endif
}

} // namespace IGCLLVM

#endif
