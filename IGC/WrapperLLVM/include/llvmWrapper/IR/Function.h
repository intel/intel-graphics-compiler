/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_FUNCTION_H
#define IGCLLVM_IR_FUNCTION_H

#include <iterator>
#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/ModRef.h"
#include "Probe/Assertion.h"

namespace IGCLLVM {

inline llvm::Argument *getArg(const llvm::Function &F, unsigned ArgNo) {
  IGC_ASSERT(F.arg_size() > ArgNo);
  llvm::Argument *Arg = nullptr;

  Arg = F.getArg(ArgNo);

  return Arg;
}

inline bool onlyWritesMemory(llvm::Function *F) { return F->onlyWritesMemory(); }

inline void pushBackBasicBlock(llvm::Function *F, llvm::BasicBlock *BB) {
#if LLVM_VERSION_MAJOR < 16
  F->getBasicBlockList().push_back(BB);
#else
  F->insert(F->end(), BB);
#endif
}

inline void splice(llvm::Function *pNewFunc, llvm::Function::iterator it, llvm::Function *pOldFunc) {
#if LLVM_VERSION_MAJOR < 16
  pNewFunc->getBasicBlockList().splice(it, pOldFunc->getBasicBlockList());
#else
  pNewFunc->splice(it, pOldFunc);
#endif
}

inline void splice(llvm::Function *pNewFunc, llvm::Function::iterator it, llvm::Function *pOldFunc,
                   llvm::BasicBlock *BB) {
#if LLVM_VERSION_MAJOR < 16
  pNewFunc->getBasicBlockList().splice(it, pOldFunc->getBasicBlockList(), BB);
#else
  pNewFunc->splice(it, pOldFunc, BB->getIterator());
#endif
}

inline void splice(llvm::Function *pNewFunc, llvm::Function::iterator it, llvm::Function *pOldFunc,
                   llvm::Function::iterator itBegin, llvm::Function::iterator itEnd) {
#if LLVM_VERSION_MAJOR < 16
  pNewFunc->getBasicBlockList().splice(it, pOldFunc->getBasicBlockList(), itBegin, itEnd);
#else
  pNewFunc->splice(it, pOldFunc, itBegin, itEnd);
#endif
}

inline auto rbegin(llvm::Function *pFunc) {
#if LLVM_VERSION_MAJOR < 16
  return pFunc->getBasicBlockList().rbegin();
#else
  return std::make_reverse_iterator(pFunc->end());
#endif
}

inline auto rend(llvm::Function *pFunc) {
#if LLVM_VERSION_MAJOR < 16
  return pFunc->getBasicBlockList().rend();
#else
  return std::make_reverse_iterator(pFunc->begin());
#endif
}

inline void insertBasicBlock(llvm::Function *pFunc, llvm::Function::iterator it, llvm::BasicBlock *BB) {
#if LLVM_VERSION_MAJOR < 16
  pFunc->getBasicBlockList().insert(it, BB);
#else
  pFunc->insert(it, BB);
#endif
}

inline void setMemoryEffects(llvm::Function &F, IGCLLVM::MemoryEffects ME) {
  F.removeFnAttrs(ME.getOverridenAttrKinds());
  F.addFnAttrs(ME.getAsAttrBuilder(F.getContext()));
}

inline void setDoesNotAccessMemory(llvm::Function &F) { setMemoryEffects(F, IGCLLVM::MemoryEffects::none()); }

inline void setOnlyReadsMemory(llvm::Function &F) { setMemoryEffects(F, IGCLLVM::MemoryEffects::readOnly()); }

inline void setOnlyWritesMemory(llvm::Function &F) { setMemoryEffects(F, IGCLLVM::MemoryEffects::writeOnly()); }

inline void setOnlyAccessesArgMemory(llvm::Function &F) { setMemoryEffects(F, IGCLLVM::MemoryEffects::argMemOnly()); }

inline void setOnlyAccessesInaccessibleMemory(llvm::Function &F) {
  setMemoryEffects(F, IGCLLVM::MemoryEffects::inaccessibleMemOnly());
}

inline void setOnlyAccessesInaccessibleMemOrArgMem(llvm::Function &F) {
  setMemoryEffects(F, IGCLLVM::MemoryEffects::inaccessibleOrArgMemOnly());
}

} // namespace IGCLLVM

#endif
