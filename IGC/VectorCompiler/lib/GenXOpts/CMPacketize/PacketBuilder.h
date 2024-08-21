/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/Support/Alignment.h"

#include <deque>
#include <unordered_map>

using namespace llvm;

namespace pktz {
struct PacketBuilder {
public:
  PacketBuilder(Module *MIn, uint32_t Width = 16);
  virtual ~PacketBuilder() {
    if (IRB)
      delete IRB;
  }
  PacketBuilder(const PacketBuilder &) = delete;
  PacketBuilder &operator=(const PacketBuilder &) = delete;

  LLVMContext &getContext() { return M->getContext(); }

  IGCLLVM::Module *M;
  IGCLLVM::IRBuilder<> *IRB;

  uint32_t VWidth;   // vector width target simd

  // Built in types: scalar
  Type *Int1Ty;
  Type *Int8Ty;
  Type *Int16Ty;
  Type *Int32Ty;
  Type *Int64Ty;
  Type *FP32Ty;

  // Built in types: target SIMD
  Type *SimdFP32Ty;
  Type *SimdInt32Ty;

  void setTargetWidth(uint32_t Width);
  Type *getVectorType(Type *Ty);

  Value *VLOG2PS(Value *A);
  Value *VEXP2PS(Value *A);

  Value *ADD(Value *LHS, Value *RHS, const Twine &Name = "",
             bool HasNUW = false, bool HasNSW = false);
  Value *AND(Value *LHS, Value *RHS, const Twine &Name = "");
  Value *AND(Value *LHS, uint64_t RHS, const Twine &Name = "");
  Value *ASHR(Value *LHS, uint64_t RHS, const Twine &Name = "",
              bool IsExact = false);
  Value *EXP2(Value *A, const Twine &Name = "");
  Value *FABS(Value *A, const Twine &Name = "");
  Value *FADD(Value *LHS, Value *RHS, const Twine &Name = "",
              MDNode *FPMathTag = nullptr);
  Value *FCMP_OEQ(Value *LHS, Value *RHS, const Twine &Name = "",
                  MDNode *FPMathTag = nullptr);
  Value *FCMP_OLT(Value *LHS, Value *RHS, const Twine &Name = "",
                  MDNode *FPMathTag = nullptr);
  Value *FCMP_UNO(Value *LHS, Value *RHS, const Twine &Name = "",
                  MDNode *FPMathTag = nullptr);
  Value *FMUL(Value *LHS, Value *RHS, const Twine &Name = "",
              MDNode *FPMathTag = nullptr);
  Value *FP_TO_SI(Value *V, Type *DestTy, const Twine &Name = "");
  Value *FSUB(Value *LHS, Value *RHS, const Twine &Name = "",
              MDNode *FPMathTag = nullptr);
  Value *MUL(Value *LHS, Value *RHS, const Twine &Name = "",
             bool HasNUW = false, bool HasNSW = false);
  Value *NOT(Value *V, const Twine &Name = "");
  Value *OR(Value *LHS, Value *RHS, const Twine &Name = "");
  Value *SHL(Value *LHS, Value *RHS, const Twine &Name = "",
             bool HasNUW = false, bool HasNSW = false);
  Value *SHL(Value *LHS, uint64_t RHS, const Twine &Name = "",
             bool HasNUW = false, bool HasNSW = false);
  Value *SI_TO_FP(Value *V, Type *DestTy, const Twine &Name = "");
  Value *SUB(Value *LHS, Value *RHS, const Twine &Name = "",
             bool HasNUW = false, bool HasNSW = false);
  Value *S_EXT(Value *V, Type *DestTy, const Twine &Name = "");
  Value *TRUNC(Value *V, Type *DestTy, const Twine &Name = "");
  Value *VMAXPS(Value *A, Value *B, const Twine &Name = "");
  Value *VMINPS(Value *A, Value *B, const Twine &Name = "");
  Value *VSQRTPS(Value *A, const Twine &Name = "");
  Value *UI_TO_FP(Value *V, Type *DestTy, const Twine &Name = "");

  //#include "PacketBuilder_misc.h"
  Constant *C(int Val);
  Constant *C(uint32_t Val);
  Constant *C(float Val);

  template <typename T> Constant *C(const std::initializer_list<T> &ConstList) {
    std::vector<Constant *> Consts;
    for (T Val : ConstList) {
      Consts.push_back(C(Val));
    }
    return ConstantVector::get(Consts);
  }

  template <typename T> Constant *CInc(uint32_t Base, uint32_t Count) {
    std::vector<Constant *> Consts;
    for (uint32_t Idx = 0; Idx < Count; Idx++) {
      Consts.push_back(C(static_cast<T>(Base + Idx)));
    }
    return ConstantVector::get(Consts);
  }

  Value *VIMMED1(int Val);
  Value *VIMMED1(uint32_t Val);
  Value *VIMMED1(float Val);

  Value *VBROADCAST(Value *Src, const llvm::Twine &Name = "");

  CallInst *CALL(Value *Callee, const std::initializer_list<Value *> &ArgsList,
                 const llvm::Twine &Name = "");

  //////////////////////////////////////////////////////////////////////////
  /// @brief functions that build IR to call x86 intrinsics directly, or
  /// emulate them with other instructions if not available on the host
  //////////////////////////////////////////////////////////////////////////

  uint32_t getTypeSize(Type *Ty);

  Value *BITCAST(Value *V, Type *DestTy, const Twine &Name = "");
  CallInst *CALLA(Value *Callee, ArrayRef<Value *> Args = None,
                  const Twine &Name = "", MDNode *FPMathTag = nullptr);
  Value *CAST(Instruction::CastOps Op, Value *V, Type *DestTy,
              const Twine &Name = "");
  ReturnInst *RET(Value *V) { return IRB->CreateRet(V); }
  Value *SELECT(Value *C, Value *True, Value *False, const Twine &Name = "",
                Instruction *MDFrom = nullptr);
  Value *VECTOR_SPLAT(unsigned NumElts, Value *V, const Twine &Name = "");
  Value *VEXTRACT(Value *Vec, uint64_t Idx, const Twine &Name = "");

  // #include "PacketBuilder_mem.h"
protected:
  void assertMemoryUsageParams(Value *Ptr);

public:
  GetElementPtrInst *GEP(Type *Ty, Value *Ptr,
                         const std::initializer_list<uint32_t> &IndexList,
                         const Twine &Name = "");
  GetElementPtrInst *GEPA(Type *Ty, Value *Ptr, ArrayRef<Value *> IdxList,
                          const Twine &Name = "");
  LoadInst *LOAD(Type *Ty, Value *Ptr, const Twine &Name = "");
  LoadInst *LOAD(Type *Ty, Value *BasePtr,
                 const std::initializer_list<uint32_t> &IndexList,
                 const llvm::Twine &Name = "");
  LoadInst *ALIGNED_LOAD(Type *Ty, Value *Ptr, IGCLLVM::Align Align,
                         const Twine &Name = "");
  AllocaInst *ALLOCA(Type *Ty, Value *ArraySize = nullptr,
                     const Twine &Name = "");
  Value *INT_TO_PTR(Value *V, Type *DestTy, const Twine &Name = "");
  CallInst *MASKED_GATHER(Type *Ty, Value *Ptrs, unsigned Align, Value *Mask = nullptr,
                          Value *PassThru = nullptr, const Twine &Name = "");
  CallInst *MASKED_SCATTER(Value *Val, Value *Ptrs, unsigned Align,
                           Value *Mask = nullptr);
  CallInst *MASKED_STORE(Value *Val, Value *Ptr, unsigned Align, Value *Mask);
  StoreInst *STORE(Value *Val, Value *Ptr, bool IsVolatile = false);
};
} // end of namespace pktz

#endif // PACKET_BUILDER_H
