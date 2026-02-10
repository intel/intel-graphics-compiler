/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTRUCTIONS_H
#define IGCLLVM_IR_INSTRUCTIONS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/ModRef.h"

#include "Probe/Assertion.h"

namespace IGCLLVM {

inline llvm::Value *getCalledValue(llvm::CallInst &CI) { return CI.getCalledOperand(); }

inline llvm::Value *getCalledValue(llvm::CallInst *CI) { return CI->getCalledOperand(); }

inline const llvm::Value *getCalledValue(const llvm::CallInst *CI) { return CI->getCalledOperand(); }

inline unsigned getNumArgOperands(const llvm::CallInst *CI) { return CI->arg_size(); }

inline unsigned getArgOperandNo(llvm::CallInst &CI, const llvm::Use *U) { return CI.getArgOperandNo(U); }

// We repeat the implementation for llvm::Function here - trying to proxy the
// calls through CB.getCalledFunction() would leave indirect calls unhandled.
inline void setMemoryEffects(llvm::CallBase &CB, IGCLLVM::MemoryEffects ME) {
  CB.removeFnAttrs(ME.getOverridenAttrKinds());
  for (const auto &MemAttr : ME.getAsAttributeSet(CB.getContext()))
    CB.addFnAttr(MemAttr);
}

inline void setDoesNotAccessMemory(llvm::CallBase &CB) { setMemoryEffects(CB, IGCLLVM::MemoryEffects::none()); }

inline void setOnlyReadsMemory(llvm::CallBase &CB) { setMemoryEffects(CB, IGCLLVM::MemoryEffects::readOnly()); }

inline void setOnlyWritesMemory(llvm::CallBase &CB) { setMemoryEffects(CB, IGCLLVM::MemoryEffects::writeOnly()); }

inline void setOnlyAccessesArgMemory(llvm::CallBase &CB) { setMemoryEffects(CB, IGCLLVM::MemoryEffects::argMemOnly()); }

inline void setOnlyAccessesInaccessibleMemory(llvm::CallBase &CB) {
  setMemoryEffects(CB, IGCLLVM::MemoryEffects::inaccessibleMemOnly());
}

inline void setOnlyAccessesInaccessibleMemOrArgMem(llvm::CallBase &CB) {
  setMemoryEffects(CB, IGCLLVM::MemoryEffects::inaccessibleOrArgMemOnly());
}

inline llvm::Constant *getShuffleMaskForBitcode(llvm::ShuffleVectorInst *SVI) {
  return llvm::ShuffleVectorInst::convertShuffleMaskForBitcode(SVI->getShuffleMask(), SVI->getType());
}

inline bool isInsertSubvectorMask(llvm::ShuffleVectorInst *SVI, int &NumSubElts, int &Index) {
  return SVI->isInsertSubvectorMask(NumSubElts, Index);
}

inline bool isFreezeInst(llvm::Instruction *I) { return llvm::isa<llvm::FreezeInst>(I); }

inline bool isDebugOrPseudoInst(const llvm::Instruction &I) { return I.isDebugOrPseudoInst(); }

inline bool comesBefore(llvm::Instruction *A, llvm::Instruction *B) { return A->comesBefore(B); }

inline llvm::Type *getGEPIndexedType(llvm::Type *Ty, llvm::SmallVectorImpl<unsigned> &indices) {
  llvm::SmallVector<llvm::Value *, 8> gepIndices;
  gepIndices.reserve(indices.size() + 1);
  auto *int32Ty = llvm::IntegerType::getInt32Ty(Ty->getContext());
  gepIndices.push_back(llvm::ConstantInt::get(int32Ty, 0));
  for (unsigned idx : indices) {
    gepIndices.push_back(llvm::ConstantInt::get(int32Ty, idx));
  }
  return llvm::GetElementPtrInst::getIndexedType(Ty, gepIndices);
}

inline llvm::Type *getGEPIndexedType(llvm::Type *Ty, llvm::ArrayRef<llvm::Value *> indices) {
  return llvm::GetElementPtrInst::getIndexedType(Ty, indices);
}

#if LLVM_VERSION_MAJOR <= 16 || defined(IGC_LLVM_TRUNK_REVISION)
constexpr int PoisonMaskElem = llvm::UndefMaskElem;
#else
constexpr int PoisonMaskElem = llvm::PoisonMaskElem;
#endif

} // namespace IGCLLVM

#endif
