/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once


#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/DenseSet.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/MDFrameWork.h"
#include "common/Types.hpp"

typedef unsigned int uint;

#define SIZE_WORD   2
#define SIZE_DWORD  4
#define SIZE_OWORD 16

enum ADDRESS_SPACE : unsigned int;

namespace IGC
{

    class CodeGenContext;
    struct SProgramOutput;

#ifdef _DEBUG
    template<typename T, size_t N>
    using smallvector = std::vector<T>;
#else
    template<typename T, size_t N>
    using smallvector = llvm::SmallVector<T, N>;
#endif

    enum e_llvmType
    {
        e_Instruction = 0,
        e_Intrinsic = 1,
        e_GenISAIntrinsic = 1,
    };
#define LLVMTYPEBYTE 24

#define OPCODE(instName,llvmType) \
    instName | llvmType<<LLVMTYPEBYTE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    name = OPCODE(llvm::llvmType::instName,e_##llvmType),
    enum EOPCODE
    {
#include "opCode.h"
    };
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    static_assert((llvm::llvmType::instName < ( 1 << LLVMTYPEBYTE ) ), "Enum bitfield range check");
#include "opCode.h"
#undef DECLARE_OPCODE

    EOPCODE GetOpCode(const llvm::Instruction* inst);
    bool SupportsModifier(llvm::Instruction* inst);
    bool SupportsSaturate(llvm::Instruction* inst);
    bool SupportsPredicate(llvm::Instruction* inst);
    bool SupportsCondModifier(llvm::Instruction* inst);
    bool SupportsRegioning(llvm::Instruction* inst);
    bool IsMathIntrinsic(EOPCODE opcode);
    bool IsAtomicIntrinsic(EOPCODE opcode);
    bool IsGradientIntrinsic(EOPCODE opcode);
    bool IsSubGroupIntrinsicWithSimd32Implementation(EOPCODE opcode);
    bool UsesTypedConstantBuffer(CodeGenContext* pContext);

    bool ComputesGradient(llvm::Instruction* inst);

    BufferType GetBufferType(uint addrSpace);

    void VectorToElement(
        llvm::Value* inst,
        llvm::Value* elem[],
        llvm::Type* int32Ty,
        llvm::Instruction* insert_before,
        int vsize = 4);
    llvm::Value* ElementToVector(
        llvm::Value* elem[],
        llvm::Type* int32Ty,
        llvm::Instruction* insert_before,
        int vsize = 4);

    /// return true if pLLVMInst is load from constant-buffer with immediate constant-buffer index
    bool IsLoadFromDirectCB(llvm::Instruction* pLLVMInst, uint& cbId, llvm::Value*& eltPtrVal);
    bool IsReadOnlyLoadDirectCB(llvm::Instruction* pLLVMInst, uint& cbId, llvm::Value*& eltPtrVal, BufferType& buftype);

    int findSampleInstructionTextureIdx(llvm::Instruction* inst);
    llvm::Value* getTextureIndexArgBasedOnOpcode(llvm::Instruction* inst);
    llvm::Value* GetBufferOperand(llvm::Instruction* inst);

    llvm::LoadInst* cloneLoad(llvm::LoadInst* Orig, llvm::Value* Ptr);
    llvm::StoreInst* cloneStore(llvm::StoreInst* Orig, llvm::Value* Val, llvm::Value* Ptr);

    llvm::Value* CreateLoadRawIntrinsic(llvm::LoadInst* inst, llvm::Instruction* bufPtr, llvm::Value* offsetVal);
    llvm::Value* CreateStoreRawIntrinsic(llvm::StoreInst* inst, llvm::Instruction* bufPtr, llvm::Value* offsetVal);

    void getTextureAndSamplerOperands(llvm::GenIntrinsicInst* pIntr, llvm::Value*& pTextureValue, llvm::Value*& pSamplerValue);
    void ChangePtrTypeInIntrinsic(llvm::GenIntrinsicInst*& pIntr, llvm::Value* oldPtr, llvm::Value* newPtr, bool isExtendedForBindlessPromotion);
    void ChangePtrTypeInIntrinsic(llvm::GenIntrinsicInst*& pIntr, llvm::Value* oldPtr, llvm::Value* newPtr);

    llvm::Value* TracePointerSource(llvm::Value* resourcePtr);
    llvm::Value* TracePointerSource(llvm::Value* resourcePtr, bool hasBranching, bool fillList, std::vector<llvm::Value*>& instList);
    llvm::Value* TracePointerSource(llvm::Value* resourcePtr, bool hasBranching, bool fillList, std::vector<llvm::Value*>& instList, llvm::SmallSet<llvm::PHINode*, 8> & visitedPHIs);
    bool GetResourcePointerInfo(llvm::Value* srcPtr, unsigned& resID,
        IGC::BufferType& resTy, IGC::BufferAccessType& accessTy);

    bool isSampleLoadGather4InfoInstruction(llvm::Instruction* inst);
    bool isSampleInstruction(llvm::Instruction* inst);
    bool isInfoInstruction(llvm::Instruction* inst);
    bool isLdInstruction(llvm::Instruction* inst);
    bool isGather4Instruction(llvm::Instruction* inst);
    bool isVectorInputInstruction(llvm::Instruction* inst);

    bool IsMediaIOIntrinsic(llvm::Instruction* inst);
    bool IsSIMDBlockIntrinsic(llvm::Instruction* inst);
    bool isSubGroupIntrinsic(const llvm::Instruction* I);

    bool isURBWriteIntrinsic(const llvm::Instruction* inst);

    unsigned EncodeAS4GFXResource(
        const llvm::Value& bufIdx,
        BufferType bufType,
        unsigned uniqueIndAS);

    unsigned SetBufferAsBindless(unsigned addressSpaceOfPtr, BufferType bufferType);

    BufferType DecodeAS4GFXResource(unsigned addrSpace, bool& directIdx, unsigned& bufId);
    int getConstantBufferLoadOffset(llvm::LoadInst* ld);

    bool IsDirectIdx(unsigned addrSpace);
    bool isNaNCheck(llvm::FCmpInst& FC);

    inline bool IsBindless(BufferType t)
    {
        return t == BINDLESS || t == BINDLESS_READONLY;
    }

    bool IsUnsignedCmp(const llvm::CmpInst::Predicate Pred);
    bool IsSignedCmp(const llvm::CmpInst::Predicate Pred);

    bool IsBitCastForLifetimeMark(const llvm::Value* V);

    // isA64Ptr - Queries whether given pointer type requires 64-bit representation in vISA
    bool isA64Ptr(llvm::PointerType* PT, CodeGenContext* pContext);

    /// Return true if F is an entry function of a kernel or a shader.
    ///    A entry function must have an entry in FunctionInfoMetaData
    ///       with type KernelFunction;
    ///    A non-entry function may have an entry, if so, that entry in
    ///       FunctionInfoMetaData must have type UserFunction.
    inline bool isEntryFunc(const IGCMD::MetaDataUtils* pM, const llvm::Function* CF)
    {
        llvm::Function* F = const_cast<llvm::Function*>(CF);
        if (F == nullptr || F->empty() ||
            pM->findFunctionsInfoItem(F) == pM->end_FunctionsInfo())
            return false;

        IGCMD::FunctionInfoMetaDataHandle Info = pM->getFunctionsInfoItem(F);
        assert(Info->isTypeHasValue() && "FunctionInfoMetaData missing type!");
        return Info->getType() == FunctionTypeMD::KernelFunction;
    }

    // Return a unique entry function.
    // Assert if more than one entry function exists.
    llvm::Function* getUniqueEntryFunc(const IGCMD::MetaDataUtils* pM);

    // \brief Get next instruction, returning null if it's the last of the BB.
    // This is the replacement of Instruction::getNextNode(), since getNextNode()
    // on last inst of BB will return sentinel node as instruction, which will
    // cause memory corruption.  A better solution is to switch to iterator and
    // avoid using getNextNode().
    inline llvm::Instruction* GetNextInstruction(llvm::Instruction* inst)
    {
        llvm::BasicBlock::iterator I = llvm::BasicBlock::iterator(inst);
        if (++I == inst->getParent()->end())
        {
            return nullptr;
        }
        return &(*I);
    }

    inline bool RTWriteHasSource0Alpha(
        const llvm::RTWritIntrinsic* rtWrite,
        ModuleMetaData* md)
    {
        return !llvm::isa<llvm::UndefValue>(rtWrite->getSource0Alpha());
    }
    inline bool DoesRTWriteSrc0AlphaBelongToHomogeneousPart(
        const llvm::RTWritIntrinsic* rtWrite,
        ModuleMetaData* md)
    {
        return !rtWrite->hasMask() && RTWriteHasSource0Alpha(rtWrite, md);
    }

    inline bool VectorUsedByConstExtractOnly(
        llvm::Value* val,
        llvm::SmallVector< llvm::SmallVector<llvm::ExtractElementInst*, 1>, 4> & extracts)
    {
        for (auto UI = val->user_begin(), UE = val->user_end(); UI != UE; ++UI)
        {
            llvm::ExtractElementInst* ei =
                llvm::dyn_cast<llvm::ExtractElementInst>(*UI);
            if (!ei)
            {
                return false;
            }
            else
            {
                llvm::ConstantInt* idxv =
                    llvm::dyn_cast<llvm::ConstantInt>(ei->getIndexOperand());
                if (!idxv)
                {
                    return false;
                }
                uint idx = (uint)idxv->getZExtValue();
                extracts[idx].push_back(ei);
            }
        }
        return true;
    }

    inline bool LoadUsedByConstExtractOnly(
        llvm::LoadInst* ld,
        llvm::SmallVector< llvm::SmallVector<llvm::ExtractElementInst*, 1>, 4> & extracts)
    {
        return VectorUsedByConstExtractOnly(ld, extracts);
    }


    llvm::Value* mutatePtrType(llvm::Value* ptrv, llvm::PointerType* newType,
        llvm::IRBuilder<>& builder, const llvm::Twine& name = "");

    unsigned int AppendConservativeRastWAHeader(IGC::SProgramOutput* program, SIMDMode simdmode);

    bool DSDualPatchEnabled(class CodeGenContext* ctx);

    /// \brief Check whether inst precedes given position in one basic block
    inline bool isInstPrecede(
        const llvm::Instruction* inst,
        const llvm::Instruction* pos)
    {
        // must within same basic block
        assert(inst->getParent() == pos->getParent());
        if (inst == pos)
        {
            return true;
        }

        auto II = inst->getParent()->begin();
        for (; &*II != inst && &*II != pos; ++II)
            ;
        return &*II == inst;
    }

    // If true, the codegen will not emit any code for this instruction
    // (So dst and src are aliased to each other.)
    bool isNoOpInst(llvm::Instruction* I, CodeGenContext* Ctx);

    // CxtI is the instruction at which V is checked whether
    // it is positive or not. 
    bool valueIsPositive(
        llvm::Value* V,
        const llvm::DataLayout* DL,
        llvm::AssumptionCache* AC = nullptr,
        llvm::Instruction* CxtI = nullptr);

    inline float GetThreadOccupancyPerSubslice(SIMDMode simdMode, unsigned threadGroupSize, unsigned hwThreadPerSubslice, unsigned slmSize, unsigned slmSizePerSubSlice)
    {
        unsigned simdWidth = 8;

        switch (simdMode)
        {
        case SIMDMode::SIMD8:   simdWidth = 8;  break;
        case SIMDMode::SIMD16:  simdWidth = 16; break;
        case SIMDMode::SIMD32:  simdWidth = 32; break;
        default:
            assert(false && "Invalid SIMD mode");
        }
        unsigned nThreadsPerTG = (threadGroupSize + simdWidth - 1) / simdWidth;

        unsigned TGPerSubsliceNoSLM = hwThreadPerSubslice / nThreadsPerTG;
        unsigned nTGDispatch = (slmSize == 0) ? TGPerSubsliceNoSLM : std::min(TGPerSubsliceNoSLM, slmSizePerSubSlice / slmSize);

        float occupancy =
            float(nTGDispatch * nThreadsPerTG) / float(hwThreadPerSubslice);
        return occupancy;
    }

    // Duplicate of the LLVM function in llvm/Transforms/Utils/ModuleUtils.h
    // Global can now be any pointer type that uses addrspace
    void appendToUsed(llvm::Module& M, llvm::ArrayRef<llvm::GlobalValue*> Values);

    bool safeScheduleUp(llvm::BasicBlock* BB, llvm::Value* V, llvm::Instruction*& InsertPos, llvm::DenseSet<llvm::Instruction*> Scheduled);

    inline unsigned GetHwThreadsPerWG(const IGC::CPlatform& platform)
    {
        unsigned hwThreadPerWorkgroup = platform.getMaxNumberThreadPerSubslice();

        if (platform.supportPooledEU())
        {
            hwThreadPerWorkgroup = platform.getMaxNumberThreadPerWorkgroupPooledMax();
        }
        return hwThreadPerWorkgroup;
    }

    inline SIMDMode getLeastSIMDAllowed(unsigned int threadGroupSize, unsigned int hwThreadPerWorkgroup)
    {
        if (hwThreadPerWorkgroup == 0)
        {
            hwThreadPerWorkgroup = 42; //On GT1 HW, there are 7 threads/EU and 6 EU/subslice, 42 is the minimum threads/workgroup any HW can support 
        }
        if ((threadGroupSize <= hwThreadPerWorkgroup * 8) &&
            threadGroupSize <= 512)
        {
            return SIMDMode::SIMD8;
        }
        else if (threadGroupSize <= hwThreadPerWorkgroup * 16)
        {
            return SIMDMode::SIMD16;
        }
        else
        {
            return SIMDMode::SIMD32;
        }
    }

} // namespace IGC
