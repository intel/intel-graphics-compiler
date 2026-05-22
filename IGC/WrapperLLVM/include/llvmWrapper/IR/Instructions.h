/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2026 Intel Corporation

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

inline llvm::CallInst *createCallInst(llvm::Function *F, llvm::ArrayRef<llvm::Value *> Args, const llvm::Twine &Name,
                                      llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::CallInst::Create(F, Args, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::CallInst::Create(F, Args, Name, InsertBefore->getIterator());
#endif
}

inline llvm::CallInst *createCallInst(llvm::FunctionType *FTy, llvm::Value *Callee, llvm::ArrayRef<llvm::Value *> Args,
                                      const llvm::Twine &Name, llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::CallInst::Create(FTy, Callee, Args, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::CallInst::Create(FTy, Callee, Args, Name, InsertBefore->getIterator());
#endif
}

inline llvm::CallInst *createCallInst(llvm::FunctionCallee Callee, llvm::ArrayRef<llvm::Value *> Args,
                                      const llvm::Twine &Name, llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::CallInst::Create(Callee, Args, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::CallInst::Create(Callee, Args, Name, InsertBefore->getIterator());
#endif
}

inline llvm::ExtractValueInst *createExtractValueInst(llvm::Value *Agg, llvm::ArrayRef<unsigned> Idxs,
                                                      const llvm::Twine &Name, llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::ExtractValueInst::Create(Agg, Idxs, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::ExtractValueInst::Create(Agg, Idxs, Name, InsertBefore->getIterator());
#endif
}

inline llvm::InsertValueInst *createInsertValueInst(llvm::Value *Agg, llvm::Value *Val, llvm::ArrayRef<unsigned> Idxs,
                                                    const llvm::Twine &Name, llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::InsertValueInst::Create(Agg, Val, Idxs, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::InsertValueInst::Create(Agg, Val, Idxs, Name, InsertBefore->getIterator());
#endif
}

inline llvm::AllocaInst *createAllocaInst(llvm::Type *Ty, unsigned AddrSpace, llvm::Value *ArraySize,
                                          const llvm::Twine &Name, llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return new llvm::AllocaInst(Ty, AddrSpace, ArraySize, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return new llvm::AllocaInst(Ty, AddrSpace, ArraySize, Name, InsertBefore->getIterator());
#endif
}

inline llvm::StoreInst *createStoreInst(llvm::Value *Val, llvm::Value *Ptr, bool isVolatile,
                                        llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return new llvm::StoreInst(Val, Ptr, isVolatile, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return new llvm::StoreInst(Val, Ptr, isVolatile, InsertBefore->getIterator());
#endif
}

inline llvm::LoadInst *createLoadInst(llvm::Type *Ty, llvm::Value *Ptr, const llvm::Twine &Name,
                                      llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return new llvm::LoadInst(Ty, Ptr, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return new llvm::LoadInst(Ty, Ptr, Name, InsertBefore->getIterator());
#endif
}

inline llvm::GetElementPtrInst *createGEPInBounds(llvm::Type *PointeeType, llvm::Value *Ptr,
                                                  llvm::ArrayRef<llvm::Value *> IdxList, const llvm::Twine &Name,
                                                  llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::GetElementPtrInst::CreateInBounds(PointeeType, Ptr, IdxList, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::GetElementPtrInst::CreateInBounds(PointeeType, Ptr, IdxList, Name, InsertBefore->getIterator());
#endif
}

inline llvm::GetElementPtrInst *createGEPInst(llvm::Type *PointeeType, llvm::Value *Ptr,
                                              llvm::ArrayRef<llvm::Value *> IdxList, const llvm::Twine &Name,
                                              llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::GetElementPtrInst::Create(PointeeType, Ptr, IdxList, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::GetElementPtrInst::Create(PointeeType, Ptr, IdxList, Name, InsertBefore->getIterator());
#endif
}

inline void insertBefore(llvm::Instruction *What, llvm::Instruction *Pos) {
#if LLVM_VERSION_MAJOR < 18
  What->insertBefore(Pos);
#else
  IGC_ASSERT(Pos);
  What->insertBefore(Pos->getIterator());
#endif
}

inline llvm::AddrSpaceCastInst *createAddrSpaceCastInst(llvm::Value *S, llvm::Type *DstTy, const llvm::Twine &Name,
                                                        llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return new llvm::AddrSpaceCastInst(S, DstTy, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return new llvm::AddrSpaceCastInst(S, DstTy, Name, InsertBefore->getIterator());
#endif
}

inline llvm::BitCastInst *createBitCastInst(llvm::Value *S, llvm::Type *Ty, const llvm::Twine &Name,
                                            llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return new llvm::BitCastInst(S, Ty, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return new llvm::BitCastInst(S, Ty, Name, InsertBefore->getIterator());
#endif
}

inline llvm::ICmpInst *createICmpInst(llvm::Instruction *InsertBefore, llvm::CmpInst::Predicate Pred, llvm::Value *LHS,
                                      llvm::Value *RHS, const llvm::Twine &Name) {
#if LLVM_VERSION_MAJOR < 18
  return new llvm::ICmpInst(InsertBefore, Pred, LHS, RHS, Name);
#else
  IGC_ASSERT(InsertBefore);
  return new llvm::ICmpInst(InsertBefore->getIterator(), Pred, LHS, RHS, Name);
#endif
}

inline llvm::PHINode *createPHINode(llvm::Type *Ty, unsigned int NumReservedValue, const llvm::Twine &Name,
                                    llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return llvm::PHINode::Create(Ty, NumReservedValue, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return llvm::PHINode::Create(Ty, NumReservedValue, Name, InsertBefore->getIterator());
#endif
}

inline llvm::IntToPtrInst *createIntToPtrInst(llvm::Value *S, llvm::Type *Ty, const llvm::Twine &Name,
                                              llvm::Instruction *InsertBefore) {
#if LLVM_VERSION_MAJOR < 18
  return new llvm::IntToPtrInst(S, Ty, Name, InsertBefore);
#else
  IGC_ASSERT(InsertBefore);
  return new llvm::IntToPtrInst(S, Ty, Name, InsertBefore->getIterator());
#endif
}

inline llvm::Instruction *getFirstNonPHI(llvm::BasicBlock *BB) {
#if LLVM_VERSION_MAJOR < 20
  return BB->getFirstNonPHI();
#else
  auto It = BB->getFirstNonPHIIt();
  return It == BB->end() ? nullptr : &*It;
#endif
}

inline llvm::Instruction *getFirstNonPHIOrDbg(llvm::BasicBlock *BB, bool SkipPseudoOp = true) {
#if LLVM_VERSION_MAJOR < 20
  return BB->getFirstNonPHIOrDbg(SkipPseudoOp);
#else
  auto It = BB->getFirstNonPHIOrDbg(SkipPseudoOp);
  return It == BB->end() ? nullptr : &*It;
#endif
}

inline llvm::Instruction *getFirstNonPHIOrDbgOrLifetime(llvm::BasicBlock *BB, bool SkipPseudoOp = true) {
#if LLVM_VERSION_MAJOR < 20
  return BB->getFirstNonPHIOrDbgOrLifetime(SkipPseudoOp);
#else
  auto It = BB->getFirstNonPHIOrDbgOrLifetime(SkipPseudoOp);
  return It == BB->end() ? nullptr : &*It;
#endif
}

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

// On LLVM < 22, debug intrinsics  appear in the
// instruction stream and getNextNonDebugInstruction() skips over them.
// On LLVM >= 22, debug info moved to DbgVariableRecord so
// getNextNode() / getPrevNode() are already non-debug.
inline llvm::Instruction *getNextNonDebugInstruction(llvm::Instruction *I, bool SkipPseudoOp = false) {
#if LLVM_VERSION_MAJOR >= 22
  return I->getNextNode();
#else
  return I->getNextNonDebugInstruction(SkipPseudoOp);
#endif
}

inline const llvm::Instruction *getNextNonDebugInstruction(const llvm::Instruction *I, bool SkipPseudoOp = false) {
#if LLVM_VERSION_MAJOR >= 22
  return I->getNextNode();
#else
  return I->getNextNonDebugInstruction(SkipPseudoOp);
#endif
}

inline llvm::Instruction *getPrevNonDebugInstruction(llvm::Instruction *I, bool SkipPseudoOp = false) {
#if LLVM_VERSION_MAJOR >= 22
  return I->getPrevNode();
#else
  return I->getPrevNonDebugInstruction(SkipPseudoOp);
#endif
}

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

#if LLVM_VERSION_MAJOR <= 16
constexpr int PoisonMaskElem = llvm::UndefMaskElem;
#else
constexpr int PoisonMaskElem = llvm::PoisonMaskElem;
#endif

} // namespace IGCLLVM

#endif
