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
  uint32_t VWidth16; // vector width simd16

  // Built in types: scalar
  Type *VoidTy;
  Type *Int1Ty;
  Type *Int8Ty;
  Type *Int16Ty;
  Type *Int32Ty;
  Type *Int64Ty;
  Type *IntPtrTy;
  Type *FP16Ty;
  Type *FP32Ty;
  Type *FP32PtrTy;
  Type *DoubleTy;
  Type *Int8PtrTy;
  Type *Int16PtrTy;
  Type *Int32PtrTy;

  Type *Simd4FP64Ty;

  // Built in types: target SIMD
  Type *SimdFP16Ty;
  Type *SimdFP32Ty;
  Type *SimdInt1Ty;
  Type *SimdInt16Ty;
  Type *SimdInt32Ty;
  Type *SimdInt64Ty;
  Type *SimdIntPtrTy;

  // Built in types: simd16

  Type *Simd16FP16Ty;
  Type *Simd16FP32Ty;
  Type *Simd16Int1Ty;
  Type *Simd16Int16Ty;
  Type *Simd16Int32Ty;
  Type *Simd16Int64Ty;
  Type *Simd16IntPtrTy;

  Type *Simd32Int8Ty;

  void setTargetWidth(uint32_t Width);
  void setTempAlloca(Value *Inst);
  bool isTempAlloca(Value *Inst);
  bool setNamedMetaDataOnCallInstr(Instruction *Inst, StringRef MDName);
  bool hasNamedMetaDataOnCallInstr(Instruction *Inst, StringRef MDName);
  Type *getVectorType(Type *Ty);

  void setMetadata(StringRef MDName, uint32_t Val) {
    auto *MD = M->getOrInsertNamedMetadata(MDName);
    auto *N = llvm::MDNode::get(
        getContext(), llvm::ConstantAsMetadata::get(IRB->getInt32(Val)));
    if (MD->getNumOperands())
      MD->setOperand(0, N);
    else
      MD->addOperand(N);
  }

  uint32_t getMetadata(StringRef MDName) {
    auto *MD = M->getNamedMetadata(MDName);
    if (MD) {
      auto *N = MD->getOperand(0);
      return mdconst::dyn_extract<ConstantInt>(N->getOperand(0))
          ->getZExtValue();
    }
    return 0;
  }

#include "gen_builder.hpp"
#include "gen_builder_intrin.hpp"
#include "gen_builder_meta.hpp"

  Value *VLOG2PS(Value *A);
  Value *VPOW24PS(Value *A);
  Value *VEXP2PS(Value *A);

  //#include "PacketBuilder_misc.h"
  Constant *C(bool Val);
  Constant *C(char Val);
  Constant *C(uint8_t Val);
  Constant *C(int Val);
  Constant *C(int64_t Val);
  Constant *C(uint64_t Val);
  Constant *C(uint16_t Val);
  Constant *C(uint32_t Val);
  Constant *C(float Val);

  template <typename T> Constant *C(const std::initializer_list<T> &ConstList) {
    std::vector<Constant *> Consts;
    for (T Val : ConstList) {
      Consts.push_back(C(Val));
    }
    return ConstantVector::get(Consts);
  }

  template <typename T> Constant *CA(LLVMContext &Ctx, ArrayRef<T> ConstList) {
    return ConstantDataArray::get(Ctx, ConstList);
  }

  template <typename T> Constant *CInc(uint32_t Base, uint32_t Count) {
    std::vector<Constant *> Consts;
    for (uint32_t Idx = 0; Idx < Count; Idx++) {
      Consts.push_back(C(static_cast<T>(Base + Idx)));
    }
    return ConstantVector::get(Consts);
  }

  Constant *PRED(bool Pred);

  Value *VIMMED1(int Val);
  Value *VIMMED1_16(int Val);

  Value *VIMMED1(uint32_t Val);
  Value *VIMMED1_16(uint32_t Val);

  Value *VIMMED1(float Val);
  Value *VIMMED1_16(float Val);

  Value *VIMMED1(bool Val);
  Value *VIMMED1_16(bool Val);

  Value *VUNDEF(Type *Ty);

  Value *VUNDEF_F();
  Value *VUNDEF_F_16();

  Value *VUNDEF_I();
  Value *VUNDEF_I_16();

  Value *VUNDEF(Type *Ty, uint32_t Size);

  Value *VUNDEF_IPTR();

  Value *VBROADCAST(Value *Src, const llvm::Twine &Name = "");
  Value *VBROADCAST_16(Value *src);

  Value *VRCP(Value *A, const llvm::Twine &Name = "");
  Value *VPLANEPS(Value *vA, Value *vB, Value *vC, Value *&vX, Value *&vY);

  uint32_t IMMED(Value *V);
  int32_t S_IMMED(Value *V);

  CallInst *CALL(Value *Callee, const std::initializer_list<Value *> &ArgsList,
                 const llvm::Twine &Name = "");
  CallInst *CALL(Value *Callee) { return CALLA(Callee); }
  CallInst *CALL(Value *Callee, Value *Arg);
  CallInst *CALL2(Value *Callee, Value *Arg1, Value *Arg2);
  CallInst *CALL3(Value *Callee, Value *Arg1, Value *Arg2, Value *Arg3);

  Value *MASK(Value *VMask);
  Value *MASK_16(Value *VMask);

  Value *VMASK(Value *Mask);
  Value *VMASK_16(Value *Mask);

  Value *VMOVMSK(Value *Mask);

  //////////////////////////////////////////////////////////////////////////
  /// @brief functions that build IR to call x86 intrinsics directly, or
  /// emulate them with other instructions if not available on the host
  //////////////////////////////////////////////////////////////////////////

  Value *EXTRACT_16(Value *A, uint32_t Imm);
  Value *JOIN_16(Value *A, Value *B);

  Value *PSHUFB(Value *A, Value *B);
  Value *PMOVSXBD(Value *A);
  Value *PMOVSXWD(Value *A);
  Value *PMAXSD(Value *A, Value *B);
  Value *PMINSD(Value *A, Value *B);
  Value *PMAXUD(Value *A, Value *B);
  Value *PMINUD(Value *A, Value *B);
  Value *VABSPS(Value *A);
  Value *FMADDPS(Value *A, Value *B, Value *C);

  Value *ICLAMP(Value *Src, Value *Low, Value *High,
                const llvm::Twine &Name = "");
  Value *FCLAMP(Value *Src, Value *Low, Value *High);
  Value *FCLAMP(Value *Src, float Low, float High);

  Value *VPOPCNT(Value *A);

  Value *VEXTRACTI128(Value *A, Constant *Imm8);
  Value *VINSERTI128(Value *A, Value *B, Constant *Imm8);

  Value *createEntryAlloca(Function *F, Type *Ty);
  Value *createEntryAlloca(Function *F, Type *Ty, Value *ArrSize);

  uint32_t getTypeSize(Type *Ty);

  // #include "PacketBuilder_mem.h"
protected:
  virtual Value *OFFSET_TO_NEXT_COMPONENT(Value *Base, Constant *Offset);
  void assertMemoryUsageParams(Value *Ptr);

public:
  virtual Value *GEP(Value *Ptr, Value *Idx, Type *Ty = nullptr,
                     const Twine &Name = "");
  virtual Value *GEP(Type *Ty, Value *Ptr, Value *Idx, const Twine &Name = "");
  virtual Value *GEP(Value *Ptr,
                     const std::initializer_list<Value *> &IndexList,
                     Type *Ty = nullptr);
  virtual Value *GEP(Value *Ptr,
                     const std::initializer_list<uint32_t> &IndexList,
                     Type *Ty = nullptr);

  Value *GEPA(Value *Ptr, ArrayRef<Value *> IdxList, const Twine &Name = "");
  Value *GEPA(Type *Ty, Value *Ptr, ArrayRef<Value *> IdxList,
              const Twine &Name = "");

  Value *IN_BOUNDS_GEP(Value *Ptr,
                       const std::initializer_list<Value *> &IndexList);
  Value *IN_BOUNDS_GEP(Value *Ptr,
                       const std::initializer_list<uint32_t> &IndexList);

  virtual LoadInst *LOAD(Value *Ptr, const char *Name, Type *Ty = nullptr);

  virtual LoadInst *LOAD(Value *Ptr, const Twine &Name = "", Type *Ty = nullptr);

  virtual LoadInst *LOAD(Type *Ty, Value *Ptr, const Twine &Name = "");

  virtual LoadInst *LOAD(Value *Ptr, bool IsVolatile, const Twine &Name = "",
                         Type *Ty = nullptr);

  virtual LoadInst *LOAD(Value *BasePtr,
                         const std::initializer_list<uint32_t> &IndexList,
                         const llvm::Twine &Name = "", Type *Ty = nullptr);

  virtual CallInst *MASKED_LOAD(Value *Ptr, unsigned Alignment, Value *Mask,
                                Value *PassThru = nullptr,
                                const Twine &Name = "", Type *Ty = nullptr) {
    return IRB->CreateMaskedLoad(
        Ptr, IGCLLVM::getAlignmentValueIfNeeded(IGCLLVM::getAlign(Alignment)),
        Mask, PassThru, Name);
  }

  LoadInst *LOADV(Value *BasePtr,
                  const std::initializer_list<Value *> &IndexList,
                  const llvm::Twine &Name = "");
  StoreInst *STORE(Value *Val, Value *BasePtr,
                   const std::initializer_list<uint32_t> &IndexList);
  StoreInst *STOREV(Value *Val, Value *BasePtr,
                    const std::initializer_list<Value *> &IndexList);

  Value *MEM_ADD(Value *Increment, Value *BasePtr,
                 const std::initializer_list<uint32_t> &IndexList,
                 const llvm::Twine &Name = "");
};
} // end of namespace pktz

#endif // PACKET_BUILDER_H
