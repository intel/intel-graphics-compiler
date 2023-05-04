/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/MemorySSAUpdater.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/CFG.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/MDFrameWork.h"
#include "common/Types.hpp"
#include "Probe/Assertion.h"
#include <functional>

typedef unsigned int uint;

#define SIZE_WORD   2
#define SIZE_DWORD  4
#define SIZE_OWORD 16
#define SIZE_YWORD 32

enum ADDRESS_SPACE : unsigned int;

namespace IGC
{

    class CodeGenContext;
    struct SProgramOutput;

    static const char * const INTEL_SYMBOL_TABLE_VOID_PROGRAM = "Intel_Symbol_Table_Void_Program";

#ifdef _DEBUG
    template<typename T, size_t N>
    using smallvector = std::vector<T>;
#else
    template<typename T, size_t N>
    using smallvector = llvm::SmallVector<T, N>;
#endif

    // This is used to return true/false/dunno results.
    enum class Tristate
    {
        Unknown = -1, False = 0, True = 1
    };

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
    bool IsExtendedMathInstruction(llvm::Instruction* Inst);
    bool IsSubGroupIntrinsicWithSimd32Implementation(EOPCODE opcode);
    bool UsesTypedConstantBuffer(
        const CodeGenContext* pContext,
        const BufferType bufType);

    bool ComputesGradient(llvm::Instruction* inst);

    BufferType GetBufferType(uint addrSpace);

    uint getImmValueU32(const llvm::Value* value);
    bool getImmValueBool(const llvm::Value* value);

    template <typename EnumT>
    static inline EnumT getImmValueEnum(const llvm::Value* val)
    {
        return static_cast<EnumT>(getImmValueU32(val));
    }

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

    llvm::Value* ConvertToFloat(llvm::IRBuilder<>& builder, llvm::Value* val);
    void ConvertToFloat(llvm::IRBuilder<>& builder, llvm::SmallVectorImpl<llvm::Value*>& instList);
    //scalarize aggregate into flattened members
    void ScalarizeAggregateMembers(llvm::IRBuilder<>& builder, llvm::Value* val, llvm::SmallVectorImpl<llvm::Value*>& instList);
    //scalarize aggregate into flattened member addresses
    void ScalarizeAggregateMemberAddresses(IGCLLVM::IRBuilder<>& builder, llvm::Type* type, llvm::Value* val, llvm::SmallVectorImpl<llvm::Value*>& instList, llvm::SmallVector<llvm::Value*, 16> indices);

    /// return true if pLLVMInst is load from constant-buffer with immediate constant-buffer index
    bool IsLoadFromDirectCB(llvm::Instruction* pLLVMInst, uint& cbId, llvm::Value*& eltPtrVal);
    bool IsReadOnlyLoadDirectCB(llvm::Instruction* pLLVMInst, uint& cbId, llvm::Value*& eltPtrVal, BufferType& buftype);

    int findSampleInstructionTextureIdx(llvm::Instruction* inst);
    llvm::Value* getTextureIndexArgBasedOnOpcode(llvm::Instruction* inst);
    llvm::Value* GetBufferOperand(llvm::Instruction* inst);

    llvm::LoadInst* cloneLoad(llvm::LoadInst* Orig, llvm::Value* Ptr);
    llvm::StoreInst* cloneStore(llvm::StoreInst* Orig, llvm::Value* Val, llvm::Value* Ptr);

    llvm::LdRawIntrinsic* CreateLoadRawIntrinsic(llvm::LoadInst* inst, llvm::Value* bufPtr, llvm::Value* offsetVal);
    llvm::StoreRawIntrinsic* CreateStoreRawIntrinsic(llvm::StoreInst* inst, llvm::Value* bufPtr, llvm::Value* offsetVal);

    void getTextureAndSamplerOperands(llvm::GenIntrinsicInst* pIntr, llvm::Value*& pTextureValue, llvm::Value*& pSamplerValue);
    void ChangePtrTypeInIntrinsic(llvm::GenIntrinsicInst*& pIntr, llvm::Value* oldPtr, llvm::Value* newPtr);

    llvm::Value* TracePointerSource(llvm::Value* resourcePtr);
    llvm::Value* TracePointerSource(llvm::Value* resourcePtr, bool hasBranching, bool enablePhiLoops, bool fillList, std::vector<llvm::Value*>& instList);
    llvm::Value* TracePointerSource(llvm::Value* resourcePtr, bool hasBranching, bool enablePhiLoops, bool fillList, std::vector<llvm::Value*>& instList, llvm::SmallSet<llvm::PHINode*, 8> & visitedPHIs);
    bool GetResourcePointerInfo(llvm::Value* srcPtr, unsigned& resID, IGC::BufferType& resTy, IGC::BufferAccessType& accessTy, bool& needBufferOffset);
    BufferAccessType getDefaultAccessType(BufferType bufTy);
    bool GetGRFOffsetFromRTV(llvm::Value* pointerSrc, unsigned& GRFOffset);
    bool GetGRFOffsetFromGlobalRootSignatureValue(llvm::Value* pointerSrc, unsigned& GRFOffset);
    bool GetStatelessBufferInfo(llvm::Value* pointer, unsigned& bufIdOrGRFOffset, IGC::BufferType& bufferTy, llvm::Value*& bufferSrcPtr, bool& isDirectBuf);
    // try to evaluate the address if it is constant.
    bool EvalConstantAddress(llvm::Value* address, unsigned int& offset, const llvm::DataLayout* pDL, llvm::Value* ptrSrc = nullptr);
    bool getConstantAddress(llvm::Instruction& I, ConstantAddress& cl, CodeGenContext* pContext, bool& directBuf, bool& statelessBuf, bool& bindlessBuf,bool& rootconstantBuf, unsigned int& TableOffset);


    bool isSampleLoadGather4InfoInstruction(const llvm::Instruction* inst);
    bool isSampleInstruction(const llvm::Instruction* inst);
    bool isInfoInstruction(const llvm::Instruction* inst);
    bool isLdInstruction(const llvm::Instruction* inst);
    bool isGather4Instruction(const llvm::Instruction* inst);

    bool IsMediaIOIntrinsic(const llvm::Instruction* inst);
    bool IsSIMDBlockIntrinsic(const llvm::Instruction* inst);
    bool isSubGroupIntrinsic(const llvm::Instruction* I);
    bool isSubGroupIntrinsicPVC(const llvm::Instruction* I);
    bool hasSubGroupIntrinsicPVC(llvm::Function& F);

    bool isBarrierIntrinsic(const llvm::Instruction* I);

    bool isUserFunctionCall(const llvm::Instruction* I);

    bool IsStatelessMemLoadIntrinsic(llvm::GenISAIntrinsic::ID id);
    bool IsStatelessMemStoreIntrinsic(llvm::GenISAIntrinsic::ID id);
    bool IsStatelessMemAtomicIntrinsic(llvm::GenIntrinsicInst& inst, llvm::GenISAIntrinsic::ID id);

    bool isURBWriteIntrinsic(const llvm::Instruction* inst);

    llvm::Instruction* AdjustSystemValueCall(llvm::GenIntrinsicInst* inst);

    unsigned EncodeAS4GFXResource(
        const llvm::Value& bufIdx,
        BufferType bufType,
        unsigned uniqueIndAS = IGC::DefaultIndirectIdx);

    unsigned SetBufferAsBindless(unsigned addressSpaceOfPtr, BufferType bufferType);
    bool isStatefulAddrSpace(unsigned AS);

    BufferType DecodeAS4GFXResource(unsigned addrSpace, bool& directIdx, unsigned& bufId);
    BufferType DecodeBufferType(unsigned addrSpace);
    int getConstantBufferLoadOffset(llvm::LoadInst* ld);

    unsigned getNumberOfExitBlocks(llvm::Function& function);

    bool isDummyBasicBlock(llvm::BasicBlock* BB);

    bool IsDirectIdx(unsigned addrSpace);
    bool isNaNCheck(llvm::FCmpInst& FC);

    inline bool IsBindless(BufferType t)
    {
        return t == BINDLESS || t == BINDLESS_CONSTANT_BUFFER || t == BINDLESS_TEXTURE;
    }
    inline bool IsSSHbindless(BufferType t)
    {
        return t == SSH_BINDLESS || t == SSH_BINDLESS_CONSTANT_BUFFER || t == SSH_BINDLESS_TEXTURE;
    }

    bool IsUnsignedCmp(const llvm::CmpInst::Predicate Pred);
    bool IsSignedCmp(const llvm::CmpInst::Predicate Pred);

    bool IsBitCastForLifetimeMark(const llvm::Value* V);

    // isA64Ptr - Queries whether given pointer type requires 64-bit representation in vISA
    bool isA64Ptr(llvm::PointerType* PT, CodeGenContext* pContext);

    inline llvm::Function* getIntelSymbolTableVoidProgram(llvm::Module* pM)
    {
        return pM->getFunction(INTEL_SYMBOL_TABLE_VOID_PROGRAM);
    }
    inline bool isIntelSymbolTableVoidProgram(llvm::Function* pF)
    {
        return (pF == getIntelSymbolTableVoidProgram(pF->getParent()));
    }

    int getFunctionControl(const CodeGenContext* pContext);

    inline bool ForceAlwaysInline(const CodeGenContext* pContext)
    {
        // return true if FunctionControl is set to INLINE, and SelectiveFunctionControl does not force fcalls.
        return getFunctionControl(pContext) == FLAG_FCALL_FORCE_INLINE &&
            IGC_GET_FLAG_VALUE(SelectiveFunctionControl) == 0;
    }

    // Strips the clone postfix added by GenXCodeGenModule in the function name
    inline std::string StripCloneName(std::string name)
    {
        auto found = name.rfind("_GenXClone");
        if (found != std::string::npos)
        {
            return name.substr(0, found);
        }
        return name;
    }

    inline bool isOptDisabledForModule(ModuleMetaData* modMD, llvm::StringRef optStr)
    {
        if (modMD->m_OptsToDisable.count(optStr.str()) != 0)
        {
            return true;
        }
        return false;
    }

    inline bool isOptDisabledForFunction(ModuleMetaData* modMD, llvm::StringRef optStr, llvm::Function* F)
    {
        // Search function metadata to check if pass needs to be disabled
        auto funcIt = modMD->FuncMD.find(F);
        if (funcIt != modMD->FuncMD.end())
        {
            if (funcIt->second.m_OptsToDisablePerFunc.count(optStr.str()) != 0)
            {
                return true;
            }
        }
        return false;
    }

    void InsertOptsMetadata(CodeGenContext* pCtx, llvm::Function* F = nullptr);

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
        IGC_ASSERT_MESSAGE(Info->isTypeHasValue(), "FunctionInfoMetaData missing type!");
        return Info->getType() == FunctionTypeMD::KernelFunction;
    }

    inline bool isPixelShaderPhaseFunction(const llvm::Function *CF) {
        const llvm::Module* M = CF->getParent();
        static const char* const phases[] = { NAMED_METADATA_COARSE_PHASE,
                                              NAMED_METADATA_PIXEL_PHASE };
        for (auto phase : phases) {
            if (auto MD = M->getNamedMetadata(phase)) {
                if (MD->getOperand(0) && MD->getOperand(0)->getOperand(0)) {
                    auto Func = llvm::mdconst::dyn_extract<llvm::Function>(
                        MD->getOperand(0)->getOperand(0));
                    if (Func == CF)
                        return true;
                }
            }
        }
        return false;
    }

    inline bool isCoarsePhaseFunction(const llvm::Function* CF) {
        const llvm::Module * M = CF->getParent();
        if (auto MD = M->getNamedMetadata(NAMED_METADATA_COARSE_PHASE)) {
            if (MD->getOperand(0) && MD->getOperand(0)->getOperand(0)) {
                auto Func = llvm::mdconst::dyn_extract<llvm::Function>(
                    MD->getOperand(0)->getOperand(0));
                return Func == CF;
            }
        }
        return false;
    }

    inline bool isPixelPhaseFunction(const llvm::Function* CF) {
        const llvm::Module* M = CF->getParent();
        if (auto MD = M->getNamedMetadata(NAMED_METADATA_PIXEL_PHASE)) {
            if (MD->getOperand(0) && MD->getOperand(0)->getOperand(0)) {
                auto Func = llvm::mdconst::dyn_extract<llvm::Function>(
                    MD->getOperand(0)->getOperand(0));
                return Func == CF;
            }
        }
        return false;
    }

    inline bool isNonEntryMultirateShader(const llvm::Function* CF) {
        if (isPixelPhaseFunction(CF))
        {
            const llvm::Module* CM = CF->getParent();
            if (auto MD = CM->getNamedMetadata(NAMED_METADATA_COARSE_PHASE)) {
                if (MD->getOperand(0) && MD->getOperand(0)->getOperand(0)) {
                    auto Func = llvm::mdconst::dyn_extract<llvm::Function>(
                        MD->getOperand(0)->getOperand(0));
                    return Func != nullptr;
                }
            }
        }
        return  false;
    }

    // Return a unique entry function.
    // If more than one entry exists, return the first and and set it as unique.
    // All subsequent calls to this function will get the entry set by the first call.
    llvm::Function* getUniqueEntryFunc(const IGCMD::MetaDataUtils* pM, IGC::ModuleMetaData* pModMD);

    template <typename T>
    inline bool RTWriteHasSource0Alpha(
        const T* rtWrite,
        ModuleMetaData* md)
    {
        return (nullptr != rtWrite->getSource0Alpha()) && !llvm::isa<llvm::UndefValue>(rtWrite->getSource0Alpha());
    }

    template <typename T>
    inline bool DoesRTWriteSrc0AlphaBelongToHomogeneousPart(
        const T* rtWrite,
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
    unsigned int AppendConservativeRastWAHeader(void*& pBinary, unsigned int& binarySize, SIMDMode simdmode);

    bool DSDualPatchEnabled(class CodeGenContext* ctx);


    /// \brief Check whether inst precedes given position in one basic block
    inline bool isInstPrecede(
        const llvm::Instruction* inst,
        const llvm::Instruction* pos)
    {
        // must within same basic block
        IGC_ASSERT(inst->getParent() == pos->getParent());
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
            IGC_ASSERT_MESSAGE(0, "Invalid SIMD mode");
            break;
        }

        IGC_ASSERT(simdWidth);
        const unsigned nThreadsPerTG = (threadGroupSize + simdWidth - 1) / simdWidth;
        IGC_ASSERT(nThreadsPerTG);
        const unsigned TGPerSubsliceNoSLM = hwThreadPerSubslice / nThreadsPerTG;
        const unsigned nTGDispatch = (slmSize == 0) ? TGPerSubsliceNoSLM : std::min(TGPerSubsliceNoSLM, slmSizePerSubSlice / slmSize);
        IGC_ASSERT(float(hwThreadPerSubslice));
        const float occupancy = float(nTGDispatch * nThreadsPerTG) / float(hwThreadPerSubslice);
        return occupancy;
    }

    // Duplicate of the LLVM function in llvm/Transforms/Utils/ModuleUtils.h
    // Global can now be any pointer type that uses addrspace
    void appendToUsed(llvm::Module& M, llvm::ArrayRef<llvm::GlobalValue*> Values);

    bool safeScheduleUp(llvm::BasicBlock* BB, llvm::Value* V, llvm::Instruction*& InsertPos, llvm::DenseSet<llvm::Instruction*> Scheduled);

    inline unsigned GetHwThreadsPerWG(const IGC::CPlatform& platform)
    {
        unsigned hwThreadPerWorkgroup = platform.getMaxNumberHWThreadForEachWG();
        if (platform.supportPooledEU())
        {
            hwThreadPerWorkgroup = std::min(platform.getMaxNumberThreadPerWorkgroupPooledMax(), (unsigned)64);
        }
        return hwThreadPerWorkgroup ? hwThreadPerWorkgroup : 1;
    }

    inline SIMDMode getLeastSIMDAllowed(unsigned int threadGroupSize, unsigned int hwThreadPerWorkgroup)
    {
        if (hwThreadPerWorkgroup == 0)
        {
            hwThreadPerWorkgroup = 42; //On GT1 HW, there are 7 threads/EU and 6 EU/subslice, 42 is the minimum threads/workgroup any HW can support
        }
        if (threadGroupSize <= hwThreadPerWorkgroup * 8)
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

    // Debug line info helper function
    inline void updateDebugLoc(llvm::Instruction* pOrigin, llvm::Instruction* pNew)
    {
        IGC_ASSERT_MESSAGE(nullptr != pOrigin, "Expect valid instructions");
        IGC_ASSERT_MESSAGE(nullptr != pNew, "Expect valid instructions");
        pNew->setDebugLoc(pOrigin->getDebugLoc());
    }

    inline bool isDbgIntrinsic(const llvm::Instruction* I) {
        if (auto *GXI = llvm::dyn_cast<llvm::GenIntrinsicInst>(I))
            return GXI->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_CatchAllDebugLine;
        return llvm::isa<llvm::DbgInfoIntrinsic>(I);
    }

    llvm::ConstantInt* getConstantSInt(llvm::IRBuilder<>& Builder, const int bitSize, int64_t val);
    llvm::ConstantInt* getConstantUInt(llvm::IRBuilder<>& Builder, const int bitSize, uint64_t val);
    llvm::Value* CreateMulhS64(llvm::IRBuilder<>& B, llvm::Value* const u, llvm::Value* const v);
    llvm::Value* CreateMulhU64(llvm::IRBuilder<>& B, llvm::Value* const u, llvm::Value* const v);
    llvm::Value* CreateMulh(llvm::Function& F, llvm::IRBuilder<>& B, const bool isSigned, llvm::Value* const u, llvm::Value* const v);

    // Ported from PostDominators.cpp of llvm10 or later
    // replace this with PDT.dominates(I1, I2) once we upgrade
    bool PDT_dominates(llvm::PostDominatorTree& PTD,
        const llvm::Instruction* I1,
        const llvm::Instruction* I2);

    // Returns true if a function has an inline asm call instruction
    bool hasInlineAsmInFunc(llvm::Function& F);

    std::tuple<std::string, std::string, unsigned> ParseVectorVariantFunctionString(llvm::StringRef varStr);

    // Return base type of complex type or nullptr if it cannot be processed
    llvm::Type* GetBaseType(llvm::Type* ProcessedType);

    // Function modifies address space in selected uses of given input value
    void FixAddressSpaceInAllUses(llvm::Value* ptr, uint newAS, uint oldAS);

    llvm::Value* CombineSampleOrGather4Params(
        llvm::IRBuilder<>& builder,
        llvm::Value* param1,
        llvm::Value* param2,
        uint numBits,
        const std::string& param1Name,
        const std::string& param2Name);

    // Returns the dynamic URB base offset and an immediate const offset
    // from the dynamic base. The function calculates the result by walking
    // the use-def chain of pUrbOffset.
    // If pUrbOffset is an immediate constant (==offset) then
    // <nullptr, offset> is returned.
    // In all other cases <pUrbOffset, 0> is returned.
    std::pair<llvm::Value*, unsigned int> GetURBBaseAndOffset(llvm::Value* pUrbOffset);

    std::vector<std::pair<unsigned int, std::string>> GetPrintfStrings(llvm::Module &M);

    template<typename Fn>
    struct Defer
    {
        Defer(Fn F) : F(F) {}
        ~Defer() {
            if (Do) F();
        }
        void operator()() {
            if (Do) F();
            Do = false;
        }
    private:
        bool Do = true;
        Fn F;
    };

    // Mimic LLVM functions:
    //   RecursivelyDeleteTriviallyDeadInstructions()
    // The difference is that the input here are dead instructions and
    // are not necessarily trivially dead. For example, store instruction.
    void RecursivelyDeleteDeadInstructions(
        llvm::Instruction* I,
        const llvm::TargetLibraryInfo* TLI = nullptr,
        llvm::MemorySSAUpdater* MSSAU = nullptr,
        std::function<void(llvm::Value*)> AboutToDeleteCallback =
            std::function<void(llvm::Value*)>());

    void RecursivelyDeleteDeadInstructions(
        const llvm::SmallVectorImpl<llvm::Instruction*>& DeadInsts,
        const llvm::TargetLibraryInfo* TLI = nullptr,
        llvm::MemorySSAUpdater* MSSAU = nullptr,
        std::function<void(llvm::Value*)> AboutToDeleteCallback =
            std::function<void(llvm::Value*)>());
} // namespace IGC
