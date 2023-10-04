/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"

#include <deque>
#include <unordered_map>

using namespace llvm;

namespace pktz
{
    struct PacketBuilder
    {
    public:
        PacketBuilder(Module* pModule, uint32_t width = 16);
        virtual ~PacketBuilder()
        {
            if (mpIRBuilder)
                delete mpIRBuilder;
        }
        PacketBuilder(const PacketBuilder &) = delete;
        PacketBuilder &operator=(const PacketBuilder &) = delete;

        IGCLLVM::IRBuilder<>* IRB() { return mpIRBuilder; };
        LLVMContext &getContext() { return mpModule->getContext(); }

        IGCLLVM::Module*     mpModule;
        IGCLLVM::IRBuilder<>* mpIRBuilder;

        uint32_t mVWidth;   // vector width target simd
        uint32_t mVWidth16; // vector width simd16

        // Built in types: scalar
        Type* mVoidTy;
        Type* mInt1Ty;
        Type* mInt8Ty;
        Type* mInt16Ty;
        Type* mInt32Ty;
        Type* mInt64Ty;
        Type* mIntPtrTy;
        Type* mFP16Ty;
        Type* mFP32Ty;
        Type* mFP32PtrTy;
        Type* mDoubleTy;
        Type* mInt8PtrTy;
        Type* mInt16PtrTy;
        Type* mInt32PtrTy;

        Type* mSimd4FP64Ty;

        // Built in types: target SIMD
        Type* mSimdFP16Ty;
        Type* mSimdFP32Ty;
        Type* mSimdInt1Ty;
        Type* mSimdInt16Ty;
        Type* mSimdInt32Ty;
        Type* mSimdInt64Ty;
        Type* mSimdIntPtrTy;

        // Built in types: simd16

        Type* mSimd16FP16Ty;
        Type* mSimd16FP32Ty;
        Type* mSimd16Int1Ty;
        Type* mSimd16Int16Ty;
        Type* mSimd16Int32Ty;
        Type* mSimd16Int64Ty;
        Type* mSimd16IntPtrTy;

        Type* mSimd32Int8Ty;

        void  SetTargetWidth(uint32_t width);
        void  SetTempAlloca(Value* inst);
        bool  IsTempAlloca(Value* inst);
        bool  SetNamedMetaDataOnCallInstr(Instruction* inst, StringRef mdName);
        bool  HasNamedMetaDataOnCallInstr(Instruction* inst, StringRef mdName);
        Type* GetVectorType(Type* pType);
        void  SetMetadata(StringRef s, uint32_t val)
        {
            llvm::NamedMDNode* metaData = mpModule->getOrInsertNamedMetadata(s);
            Constant*          cval     = IRB()->getInt32(val);
            llvm::MDNode*      mdNode   = llvm::MDNode::get(getContext(),
                                                     llvm::ConstantAsMetadata::get(cval));
            if (metaData->getNumOperands())
            {
                metaData->setOperand(0, mdNode);
            }
            else
            {
                metaData->addOperand(mdNode);
            }
        }
        uint32_t GetMetadata(StringRef s)
        {
            NamedMDNode* metaData = mpModule->getNamedMetadata(s);
            if (metaData)
            {
                MDNode*   mdNode = metaData->getOperand(0);
                Metadata* val    = mdNode->getOperand(0);
                return mdconst::dyn_extract<ConstantInt>(val)->getZExtValue();
            }
            else
            {
                return 0;
            }
        }
#include "gen_builder.hpp"
#include "gen_builder_intrin.hpp"
#include "gen_builder_meta.hpp"

        Value* VLOG2PS(Value* src);
        Value* VPOW24PS(Value* src);
        Value* VEXP2PS(Value* src);

        //#include "PacketBuilder_misc.h"
        Constant* C(bool i);
        Constant* C(char i);
        Constant* C(uint8_t i);
        Constant* C(int i);
        Constant* C(int64_t i);
        Constant* C(uint64_t i);
        Constant* C(uint16_t i);
        Constant* C(uint32_t i);
        Constant* C(float i);

        template <typename Ty>
        Constant* C(const std::initializer_list<Ty>& constList)
        {
          std::vector<Constant*> vConsts;
          for (auto i : constList)
          {
            vConsts.push_back(C((Ty)i));
          }
          return ConstantVector::get(vConsts);
        }

        template <typename Ty>
        Constant* CA(LLVMContext& ctx, ArrayRef<Ty> constList)
        {
          return ConstantDataArray::get(ctx, constList);
        }

        template <typename Ty>
        Constant* CInc(uint32_t base, uint32_t count)
        {
          std::vector<Constant*> vConsts;

          for (uint32_t i = 0; i < count; i++)
          {
            vConsts.push_back(C((Ty)base));
            base++;
          }
          return ConstantVector::get(vConsts);
        }

        Constant* PRED(bool pred);

        Value* VIMMED1(int i);
        Value* VIMMED1_16(int i);

        Value* VIMMED1(uint32_t i);
        Value* VIMMED1_16(uint32_t i);

        Value* VIMMED1(float i);
        Value* VIMMED1_16(float i);

        Value* VIMMED1(bool i);
        Value* VIMMED1_16(bool i);

        Value* VUNDEF(Type* t);

        Value* VUNDEF_F();
        Value* VUNDEF_F_16();

        Value* VUNDEF_I();
        Value* VUNDEF_I_16();

        Value* VUNDEF(Type* ty, uint32_t size);

        Value* VUNDEF_IPTR();

        Value* VBROADCAST(Value* src, const llvm::Twine& name = "");
        Value* VBROADCAST_16(Value* src);

        Value* VRCP(Value* va, const llvm::Twine& name = "");
        Value* VPLANEPS(Value* vA, Value* vB, Value* vC, Value*& vX, Value*& vY);

        uint32_t IMMED(Value* i);
        int32_t  S_IMMED(Value* i);

        CallInst* CALL(Value* Callee, const std::initializer_list<Value*>& args, const llvm::Twine& name = "");
        CallInst* CALL(Value* Callee)
        {
          return CALLA(Callee);
        }
        CallInst* CALL(Value* Callee, Value* arg);
        CallInst* CALL2(Value* Callee, Value* arg1, Value* arg2);
        CallInst* CALL3(Value* Callee, Value* arg1, Value* arg2, Value* arg3);

        Value* MASK(Value* vmask);
        Value* MASK_16(Value* vmask);

        Value* VMASK(Value* mask);
        Value* VMASK_16(Value* mask);

        Value* VMOVMSK(Value* mask);

        //////////////////////////////////////////////////////////////////////////
        /// @brief functions that build IR to call x86 intrinsics directly, or
        /// emulate them with other instructions if not available on the host
        //////////////////////////////////////////////////////////////////////////

        Value* EXTRACT_16(Value* x, uint32_t imm);
        Value* JOIN_16(Value* a, Value* b);

        Value* PSHUFB(Value* a, Value* b);
        Value* PMOVSXBD(Value* a);
        Value* PMOVSXWD(Value* a);
        Value* PMAXSD(Value* a, Value* b);
        Value* PMINSD(Value* a, Value* b);
        Value* PMAXUD(Value* a, Value* b);
        Value* PMINUD(Value* a, Value* b);
        Value* VABSPS(Value* a);
        Value* FMADDPS(Value* a, Value* b, Value* c);

        Value* ICLAMP(Value* src, Value* low, Value* high, const llvm::Twine& name = "");
        Value* FCLAMP(Value* src, Value* low, Value* high);
        Value* FCLAMP(Value* src, float low, float high);

        Value* VPOPCNT(Value* a);

        Value* VEXTRACTI128(Value* a, Constant* imm8);
        Value* VINSERTI128(Value* a, Value* b, Constant* imm8);

        Value* CreateEntryAlloca(Function* pFunc, Type* pType);
        Value* CreateEntryAlloca(Function* pFunc, Type* pType, Value* pArraySize);

        uint32_t GetTypeSize(Type* pType);

      // #include "PacketBuilder_mem.h"
      public:
        typedef enum _JIT_MEM_CLIENT
        {
          MEM_CLIENT_INTERNAL,
          GFX_MEM_CLIENT_FETCH,
          GFX_MEM_CLIENT_SAMPLER,
          GFX_MEM_CLIENT_SHADER,
        } JIT_MEM_CLIENT;

      protected:
        virtual Value* OFFSET_TO_NEXT_COMPONENT(Value* base, Constant* offset);
        void           AssertMemoryUsageParams(Value* ptr, JIT_MEM_CLIENT usage);

      public:
        virtual Value* GEP(Value* Ptr, Value* Idx, Type* Ty = nullptr, const Twine& Name = "");
        virtual Value* GEP(Type* Ty, Value* Ptr, Value* Idx, const Twine& Name = "");
        virtual Value* GEP(Value* ptr, const std::initializer_list<Value*>& indexList, Type* Ty = nullptr);
        virtual Value* GEP(Value* ptr, const std::initializer_list<uint32_t>& indexList, Type* Ty = nullptr);

        Value* GEPA(Value* Ptr, ArrayRef<Value*> IdxList, const Twine& Name = "");
        Value* GEPA(Type* Ty, Value* Ptr, ArrayRef<Value*> IdxList, const Twine& Name = "");

        Value* IN_BOUNDS_GEP(Value* ptr, const std::initializer_list<Value*>& indexList);
        Value* IN_BOUNDS_GEP(Value* ptr, const std::initializer_list<uint32_t>& indexList);

        virtual LoadInst* LOAD(Value* Ptr, const char* Name, Type* Ty = nullptr, JIT_MEM_CLIENT usage = MEM_CLIENT_INTERNAL);

        virtual LoadInst* LOAD(Value*         Ptr,
          const Twine&   Name = "",
          Type*          Ty = nullptr,
          JIT_MEM_CLIENT usage = MEM_CLIENT_INTERNAL);

        virtual LoadInst* LOAD(Type* Ty, Value* Ptr, const Twine& Name = "", JIT_MEM_CLIENT usage = MEM_CLIENT_INTERNAL);

        virtual LoadInst* LOAD(Value*         Ptr,
          bool           isVolatile,
          const Twine&   Name = "",
          Type*          Ty = nullptr,
          JIT_MEM_CLIENT usage = MEM_CLIENT_INTERNAL);

        virtual LoadInst* LOAD(Value*                                 BasePtr,
          const std::initializer_list<uint32_t>& offset,
          const llvm::Twine&                     Name = "",
          Type*                                  Ty = nullptr,
          JIT_MEM_CLIENT                         usage = MEM_CLIENT_INTERNAL);

        virtual CallInst* MASKED_LOAD(Value*         Ptr,
          unsigned       Alignment,
          Value*         Mask,
          Value*         PassThru = nullptr,
          const Twine&   Name = "",
          Type*          Ty = nullptr,
          JIT_MEM_CLIENT usage = MEM_CLIENT_INTERNAL)
        {
          return IRB()->CreateMaskedLoad(
              Ptr, IGCLLVM::getAlignmentValueIfNeeded(IGCLLVM::getAlign(Alignment)),
              Mask, PassThru, Name);
        }

        LoadInst*  LOADV(Value* BasePtr, const std::initializer_list<Value*>& offset, const llvm::Twine& name = "");
        StoreInst* STORE(Value* Val, Value* BasePtr, const std::initializer_list<uint32_t>& offset);
        StoreInst* STOREV(Value* Val, Value* BasePtr, const std::initializer_list<Value*>& offset);

        Value* MEM_ADD(Value*                                 i32Incr,
          Value*                                 basePtr,
          const std::initializer_list<uint32_t>& indices,
          const llvm::Twine&                     name = "");
    };
} // end of namespace pktz
