/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/DebugInfoData.hpp"
#include "Compiler/CISACodeGen/CVariable.hpp"
#include "Compiler/CISACodeGen/PushAnalysis.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/CISABuilder.hpp"
#include "Compiler/CISACodeGen/LiveVars.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/CoalescingEngine.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
// Needed for SConstantGatherEntry
#include "usc_gen7.h"
#include "common/Types.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/debug/Dump.hpp"
#include <map>
#include <string>
#include <vector>
#include "Probe/Assertion.h"

namespace llvm
{
    class Value;
    class PHINode;
    class Function;
    class BasicBlock;
}

namespace IGC
{
class DeSSA;
class CoalescingEngine;
class GenXFunctionGroupAnalysis;
class VariableReuseAnalysis;

struct PushInfo;

// Helper Function
VISA_Type GetType(llvm::Type* pType, CodeGenContext* pDataLayout);
uint64_t GetImmediateVal(llvm::Value* Const);
e_alignment GetPreferredAlignment(llvm::Value* Val, WIAnalysis* WIA, CodeGenContext* pContext);

class CShaderProgram;
class CBindlessShader;

///--------------------------------------------------------------------------------------------------------
class CShader
{
public:
    friend class CShaderProgram;

    class ExtractMaskWrapper
    {
        // To enable ExtractMask of any vector size. Currently, only vector
        // whose size is no larger than 32 has its extractMask calculated.
    private:
        uint32_t m_EM;     // 32 bit extractMask;
        bool     m_hasEM;  // If true, m_EM is valid; otherwise, not valid.
    public:
        ExtractMaskWrapper(CShader* pS, llvm::Value* VecVal);

        ExtractMaskWrapper() = delete;
        ExtractMaskWrapper(const ExtractMaskWrapper&) = delete;
        ExtractMaskWrapper& operator=(const ExtractMaskWrapper&) = delete;

        // b: bit position, from 0 to 31.
        bool isSet(uint32_t b) const
        {
            if (m_hasEM) {
                IGC_ASSERT(b < 32);
                return (1 << (b)) & m_EM;
            }
            return true;
        }

        uint32_t getEM() const { return m_EM; }
        uint16_t hasEM() const { return m_hasEM; }
    };

    CShader(llvm::Function*, CShaderProgram* pProgram);
    virtual ~CShader();
    void        Destroy();
    virtual void InitEncoder(SIMDMode simdMode, bool canAbortOnSpill, ShaderDispatchMode shaderMode = ShaderDispatchMode::NOT_APPLICABLE);
    virtual void PreCompile() {}
    virtual void PreCompileFunction(llvm::Function& F) {}
    virtual void ParseShaderSpecificOpcode(llvm::Instruction* inst) {}
    virtual void AllocatePayload() {}
    virtual void AddPrologue() {}
    virtual void PreAnalysisPass();
    virtual void ExtractGlobalVariables() {}
    void         EOTURBWrite();
    void         EOTRenderTarget(CVariable* r1, bool isPerCoarse);
    CVariable* URBFence();
    void         EOTGateway(CVariable* payload = nullptr);
    virtual void AddEpilogue(llvm::ReturnInst* ret);

    virtual CVariable* GetURBOutputHandle()
    {
        IGC_ASSERT_MESSAGE(0, "Should be overridden in a derived class!");
        return nullptr;
    }
    virtual CVariable* GetURBInputHandle(CVariable* pVertexIndex)
    {
        IGC_ASSERT_MESSAGE(0, "Should be overridden in a derived class!");
        return nullptr;
    }

    virtual CVariable* GetGlobalBufferPtr() { IGC_ASSERT(0); return nullptr; }
    virtual CVariable* GetLocalBufferPtr() { IGC_ASSERT(0); return nullptr; }
    virtual CVariable* GetStackID() { IGC_ASSERT(0); return nullptr; }
    virtual CVariable* GetInlinedDataPtr() { IGC_ASSERT(0); return nullptr; }
    // if true, HW will pass one GRF NOS of inlinedata to payload, (compute only right now)

    virtual bool passNOSInlineData() { return false; }
    virtual bool loadThreadPayload() { return false; }
    virtual unsigned getAnnotatedNumThreads() { return 0; }
    virtual bool IsRegularGRFRequested() { return false; }
    virtual bool IsLargeGRFRequested() { return false; }
    virtual bool hasReadWriteImage(llvm::Function& F) { return false; }
    virtual bool CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F)
    {
        return CompileSIMDSizeInCommon(simdMode);
    }
    CVariable* LazyCreateCCTupleBackingVariable(
        CoalescingEngine::CCTuple* ccTuple,
        VISA_Type baseType = ISA_TYPE_UD);
    CVariable* GetSymbol(llvm::Value* value, bool fromConstantPool = false);
    void        AddSetup(uint index, CVariable* var);
    bool        AppendPayloadSetup(CVariable* var);
    void        AddPatchTempSetup(CVariable* var);
    void        AddPatchConstantSetup(uint index, CVariable* var);

    // TODO: simplify calls to GetNewVariable to these shorter and more
    // expressive cases where possible.
    //
    // CVariable* GetNewVector(VISA_Type type, const CName &name) {
    //     return GetNewVariable(numLanes(m_SIMDSize), type, EALIGN_GRF, false, name);
    // }
    // CVariable* GetNewUniform(VISA_Type type, const CName &name) {
    //    grep a GetNewVariable(1, .. true) and see what B and W use
    //     return GetNewVariable(1, type, alignOf_TODO(type), true, name);
    // }

    CVariable* GetNewVariable(
        uint16_t nbElement,
        VISA_Type type,
        e_alignment align,
        const CName &name)
    {
        return GetNewVariable(nbElement, type, align, false, 1, name);
    }
    CVariable* GetNewVariable(
        uint16_t nbElement,
        VISA_Type type,
        e_alignment align,
        UniformArgWrap uniform,
        const CName &name)
    {
        return GetNewVariable(nbElement, type, align, uniform, 1, name);
    }
    CVariable* GetNewVariable(
        uint16_t nbElement,
        VISA_Type type,
        e_alignment align,
        UniformArgWrap uniform,
        uint16_t numberInstance,
        const CName &name);
    CVariable* GetNewVariable(const CVariable* from);
    CVariable* GetNewAddressVariable(
        uint16_t nbElement,
        VISA_Type type,
        UniformArgWrap uniform,
        bool vectorUniform,
        const CName &name);
    CVariable* GetNewVector(llvm::Value* val, e_alignment preferredAlign = EALIGN_AUTO);
    CVariable* GetNewAlias(CVariable* var, VISA_Type type, uint16_t offset, uint16_t numElements);
    CVariable* GetNewAlias(CVariable* var, VISA_Type type, uint16_t offset, uint16_t numElements, bool uniform);

    // If BaseVar's type matches V's, return BaseVar; otherwise, create an new
    // alias CVariable to BaseVar. The newly-created alias CVariable's size
    // should be the same as BaseVar's size (used for creating alias for values
    // in the same DeSSA's congruent class).
    CVariable* createAliasIfNeeded(llvm::Value* V, CVariable* BaseVar);
    // Allow to create an alias of a variable handpicking a slice to be able to do cross lane in SIMD32
    CVariable* GetVarHalf(CVariable* var, unsigned int half);

    void        CopyVariable(CVariable* dst, CVariable* src, uint dstSubVar = 0, uint srcSubVar = 0);
    void        PackAndCopyVariable(CVariable* dst, CVariable* src, uint subVar = 0);
    void        CopyVariableRaw(CVariable* dst, CVariable* src);
    CVariable*  CopyVariableRaw(CVariable* src, bool singleInstance = true);
    bool        IsValueUsed(llvm::Value* value);
    CVariable*  GetGlobalCVar(llvm::Value* value);
    uint        GetNbElementAndMask(llvm::Value* value, uint32_t& mask);
    void        CreatePayload(uint regCount, uint idxOffset, CVariable*& payload, llvm::Instruction* inst, uint paramOffset, uint8_t hfFactor);
    uint        GetNbVectorElementAndMask(llvm::Value* value, uint32_t& mask);
    uint16_t    AdjustExtractIndex(llvm::Value* value, uint16_t elemIndex);
    WIBaseClass::WIDependancy GetDependency(llvm::Value* v) const;
    void        SetDependency(llvm::Value* v, WIBaseClass::WIDependancy dep);
    bool        GetIsUniform(llvm::Value* v) const;
    bool        InsideDivergentCF(const llvm::Instruction* inst) const;
    bool        InsideWorkgroupDivergentCF(const llvm::Instruction* inst) const;
    CEncoder& GetEncoder();
    CVariable* GetR0();
    CVariable* GetNULL();
    CVariable* GetTSC();
    CVariable* GetSR0();
    CVariable* GetCR0();
    CVariable* GetCE0();
    CVariable* GetDBG();
    CVariable* GetHWTID();
    CVariable* GetSP();
    CVariable* GetFP();
    CVariable* GetPrevFP();
    CVariable* GetARGV();
    CVariable* GetRETV();
    CVariable* GetPrivateBase();
    CVariable* GetImplArgBufPtr();
    CVariable* GetLocalIdBufPtr();

    void SaveSRet(CVariable* sretPtr);
    CVariable* GetAndResetSRet();

    bool hasSP() const { return m_SP != nullptr; }
    bool hasFP() const { return m_FP != nullptr; }

    void InitializeStackVariables();
    void SaveStackState();
    void RestoreStackState();

    void InitializeScratchSurfaceStateAddress();

    void        AllocateInput(CVariable* var, uint offset, uint instance = 0, bool forceLiveOut = false);
    void        AllocateOutput(CVariable* var, uint offset, uint instance = 0);
    CVariable* ImmToVariable(uint64_t immediate, VISA_Type type, bool isCodePatchCandidate = false);
    CVariable* GetConstant(llvm::Constant* C, CVariable* dstVar = nullptr);
    CVariable* GetScalarConstant(llvm::Value* c);
    CVariable* GetUndef(VISA_Type type);
    llvm::Constant* findCommonConstant(llvm::Constant* C, uint elts, uint currentEmitElts, bool& allSame);
    virtual unsigned int GetGlobalMappingValue(llvm::Value* c);
    virtual CVariable* GetGlobalMapping(llvm::Value* c);
    CVariable* BitCast(CVariable* var, VISA_Type newType);
    void        ResolveAlias(CVariable* var);
    void        CacheArgumentsList();
    virtual void MapPushedInputs();
    void        CreateGatherMap();
    void        CreateConstantBufferOutput(SKernelProgram* pKernelProgram);
    void        CreateFunctionSymbol(llvm::Function* pFunc);
    void        CreateGlobalSymbol(llvm::GlobalVariable* pGlobal);

    CVariable*  GetStructVariable(llvm::Value* v, bool forceVectorInit = false);

    void        CreateImplicitArgs();
    void        CreateAliasVars();
    uint        GetBlockId(llvm::BasicBlock* block);
    uint        GetNumSBlocks() { return m_numBlocks; }

    void        SetUniformHelper(WIAnalysis* WI) { m_WI = WI; }
    void        SetDeSSAHelper(DeSSA* deSSA) { m_deSSA = deSSA; }
    void        SetCoalescingEngineHelper(CoalescingEngine* ce) { m_coalescingEngine = ce; }
    void        SetCodeGenHelper(CodeGenPatternMatch* CG) { m_CG = CG; }
    void        SetPushInfoHelper(PushInfo* PI) { pushInfo = *PI; }
    void        SetDominatorTreeHelper(llvm::DominatorTree* DT) { m_DT = DT; }
    void        SetDataLayout(const llvm::DataLayout* DL) { m_DL = DL; }
    void        SetFunctionGroupAnalysis(GenXFunctionGroupAnalysis* FGA) { m_FGA = FGA; }
    void        SetVariableReuseAnalysis(VariableReuseAnalysis* VRA) { m_VRA = VRA; }
    void        SetMetaDataUtils(IGC::IGCMD::MetaDataUtils* pMdUtils) { m_pMdUtils = pMdUtils; }
    void        SetScratchSpaceSize(uint size) { m_ScratchSpaceSize = size; }
    IGCMD::MetaDataUtils* GetMetaDataUtils() { return m_pMdUtils; }

    virtual  void SetShaderSpecificHelper(EmitPass* emitPass) {}

    void        AllocateConstants(uint& offset);
    void        AllocateStatelessConstants(uint& offset);
    void        AllocateSimplePushConstants(uint& offset);
    void        AllocateNOSConstants(uint& offset);
    void        AllocateConstants3DShader(uint& offset);
    ShaderType  GetShaderType() const { return GetContext()->type; }
    bool        IsPatchablePS();
    bool        IsValueCoalesced(llvm::Value* v);

    bool        GetHasBarrier() const { return m_BarrierNumber > 0; }
    void        SetHasBarrier() { if (m_BarrierNumber == 0) m_BarrierNumber = 1; }
    void        SetBarrierNumber(int BarrierNumber) { m_BarrierNumber = BarrierNumber; }
    int         GetBarrierNumber() const { return m_BarrierNumber; }

    void        GetSimdOffsetBase(CVariable*& pVar, bool dup = false);
    /// Returns a simd8 register filled with values [24, 20, 16, 12, 8, 4, 0]
    /// that are used to index subregisters of a GRF when counting offsets in bytes.
    /// Used e.g. for indirect addressing via a0 register.
    CVariable* GetPerLaneOffsetsReg(uint typeSizeInBytes);

    void        GetPayloadElementSymbols(llvm::Value* inst, CVariable* payload[], int vecWidth);

    CodeGenContext* GetContext() const { return m_ctx; }

    SProgramOutput* ProgramOutput();

    bool CanTreatAsAlias(llvm::ExtractElementInst* inst);
    bool CanTreatScalarSourceAsAlias(llvm::InsertElementInst*);

    bool HasBecomeNoop(llvm::Instruction* inst);

    // If V is not in any congruent class, not aliased to any other
    // variables, not payload-coalesced, then this function returns
    // true.
    bool IsCoalesced(llvm::Value* V);

    bool VMECoalescePattern(llvm::GenIntrinsicInst*);

    bool isUnpacked(llvm::Value* value);

    /// Return true if we are sure that all lanes are active at the begging of the thread
    virtual bool HasFullDispatchMask() { return false; }
    bool needsEntryFence() const;

    llvm::Function* entry;
    const CBTILayout* m_pBtiLayout;
    const CPlatform* m_Platform;
    const CDriverInfo* m_DriverInfo;

    ModuleMetaData* m_ModuleMetadata;

    /// Dispatch size is the number of logical threads running in one hardware thread
    SIMDMode m_dispatchSize;
    /// SIMD Size is the default size of instructions
    ShaderDispatchMode m_ShaderDispatchMode;
    /// the default emit size for this shader. This is the default size for variables as well
    /// as the default execution size for each instruction. encoder may override it explicitly
    /// via CEncoder::SetSIMDSize
    SIMDMode m_SIMDSize;
    uint8_t m_numberInstance;
    PushInfo pushInfo;
    bool isInputsPulled; //true if any input is pulled, false otherwise
    bool isMessageTargetDataCacheDataPort;
    uint m_sendStallCycle;
    uint m_staticCycle;
    uint m_loopNestedStallCycle;
    uint m_loopNestedCycle;
    unsigned m_spillSize = 0;
    float m_spillCost = 0;          // num weighted spill inst / total inst

    std::vector<llvm::Value*> m_argListCache;

    /// The size in byte used by igc (non-spill space). And this
    /// is the value passed to VISA so that VISA's spill, if any,
    /// will go after this space.
    uint m_ScratchSpaceSize;

    CVariable* m_ScratchSurfaceAddress = nullptr;

    ShaderStats* m_shaderStats;

    // Number of binding table entries per cache line.
    static constexpr DWORD cBTEntriesPerCacheLine = 32;
    // Max BTI value that can increase binding table count.
    // SampleEngine:    Binding Table Index is set to 252 specifies the bindless surface offset.
    // DataPort:        The special entry 255 is used to reference Stateless A32 or A64 address model,
    //                  and the special entry 254 is used to reference the SLM address model.
    //                  The special entry 252 is used to reference bindless resource operation.
    static constexpr DWORD MAX_BINDING_TABLE_INDEX = 251;
    static constexpr uint cMessageExtendedDescriptorEOTBit = BIT(5);

    CVariable* GetCCTupleToVariableMapping(CoalescingEngine::CCTuple* ccTuple)
    {
        return ccTupleMapping[ccTuple];
    }

    void addConstantInPool(llvm::Constant* C, CVariable* Var) {
        ConstantPool[C] = Var;
    }

    CVariable* lookupConstantInPool(llvm::Constant* C) {
        return ConstantPool.lookup(C);
    }

    unsigned int EvaluateSIMDConstExpr(llvm::Value* C);

    /// Initialize per function status.
    void BeginFunction(llvm::Function* F);
    // This method split payload interpolations from the shader into another compilation unit
    void SplitPayloadFromShader(llvm::Function* F);
    /// This method is used to create the vISA variable for function F's formal return value
    CVariable* getOrCreateReturnSymbol(llvm::Function* F);
    /// This method is used to create the vISA variable for function F's formal argument
    CVariable* getOrCreateArgumentSymbol(
        llvm::Argument* Arg,
        bool ArgInCallee, // true if Arg isn't in current func
        bool useStackCall = false);
    void UpdateSymbolMap(llvm::Value* v, CVariable* CVar);
    VISA_Type GetType(llvm::Type* type);
    uint32_t GetNumElts(llvm::Type* type, bool isUniform = false);

    /// Evaluate constant expression and return the result immediate value.
    uint64_t GetConstantExpr(llvm::ConstantExpr* C);


    uint32_t GetMaxUsedBindingTableEntryCount(void) const
    {
        if (m_BindingTableUsedEntriesBitmap != 0)
        {
            // m_BindingTableEntryCount is index; '+ 1' due to calculate total used count.
            return (m_BindingTableEntryCount + 1);
        }
        return 0;
    }

    uint32_t GetBindingTableEntryBitmap(void) const
    {
        return m_BindingTableUsedEntriesBitmap;
    }

    void SetBindingTableEntryCountAndBitmap(bool directIdx, BufferType bufType, uint32_t typeBti, uint32_t bti)
    {
        if (bti <= MAX_BINDING_TABLE_INDEX)
        {
            if (directIdx)
            {
                m_BindingTableEntryCount = (bti <= m_pBtiLayout->GetBindingTableEntryCount()) ? (std::max(bti, m_BindingTableEntryCount)) : m_BindingTableEntryCount;
                m_BindingTableUsedEntriesBitmap |= BIT(bti / cBTEntriesPerCacheLine);

                if (bufType == RESOURCE)
                {
                    m_shaderResourceLoaded[typeBti / 32] |= BIT(typeBti % 32);
                }
                else if (bufType == CONSTANT_BUFFER)
                {
                    m_constantBufferLoaded |= BIT(typeBti);
                }
                else if (bufType == UAV)
                {
                    m_uavLoaded |= QWBIT(typeBti);
                }
                else if (bufType == RENDER_TARGET)
                {
                    m_renderTargetLoaded |= BIT(typeBti);
                }
            }
            else
            {
                // Indirect addressing, set the maximum BTI.
                m_BindingTableEntryCount = m_pBtiLayout->GetBindingTableEntryCount();
                m_BindingTableUsedEntriesBitmap |= BITMASK_RANGE(0, (m_BindingTableEntryCount / cBTEntriesPerCacheLine));

                if (bufType == RESOURCE || bufType == BINDLESS_TEXTURE)
                {
                    unsigned int MaxArray = m_pBtiLayout->GetTextureIndexSize() / 32;
                    for (unsigned int i = 0; i < MaxArray; i++)
                    {
                        m_shaderResourceLoaded[i] = 0xffffffff;
                    }

                    for (unsigned int i = MaxArray * 32; i < m_pBtiLayout->GetTextureIndexSize(); i++)
                    {
                        m_shaderResourceLoaded[MaxArray] = BIT(i % 32);
                    }
                }
                else if (bufType == CONSTANT_BUFFER || bufType == BINDLESS_CONSTANT_BUFFER)
                {
                    m_constantBufferLoaded |= BITMASK_RANGE(0, m_pBtiLayout->GetConstantBufferIndexSize());
                }
                else if (bufType == UAV || bufType == BINDLESS)
                {
                    m_uavLoaded |= QWBITMASK_RANGE(0, m_pBtiLayout->GetUavIndexSize());
                }
                else if (bufType == RENDER_TARGET)
                {
                    m_renderTargetLoaded |= BITMASK_RANGE(0, m_pBtiLayout->GetRenderTargetIndexSize());
                }
            }
        }
    }

    /// Evaluate the Sampler Count field value.
    unsigned int GetSamplerCount(unsigned int samplerCount);

    static unsigned GetIMEReturnPayloadSize(llvm::GenIntrinsicInst* I);

    void addCVarsForVectorBC(llvm::BitCastInst* BCI, llvm::SmallVector<CVariable*, 8> CVars)
    {
        IGC_ASSERT_MESSAGE(m_VectorBCItoCVars.find(BCI) == std::end(m_VectorBCItoCVars), "a variable already exists for this vector bitcast");
        m_VectorBCItoCVars.try_emplace(BCI, CVars);
    }

    CVariable* getCVarForVectorBCI(llvm::BitCastInst* BCI, int index)
    {
        auto iter = m_VectorBCItoCVars.find(BCI);
        if (iter == m_VectorBCItoCVars.end())
        {
            return nullptr;
        }
        return (*iter).second[index];
    }

    void SetHasGlobalStatelessAccess() { m_HasGlobalStatelessMemoryAccess = true; }
    bool GetHasGlobalStatelessAccess() const { return m_HasGlobalStatelessMemoryAccess; }
    void SetHasConstantStatelessAccess() { m_HasConstantStatelessMemoryAccess = true; }
    bool GetHasConstantStatelessAccess() const { return m_HasConstantStatelessMemoryAccess; }
    void SetHasGlobalAtomics() { m_HasGlobalAtomics = true; }
    bool GetHasGlobalAtomics() const { return m_HasGlobalAtomics; }
    bool GetHasDPAS() const { return m_HasDPAS; }
    void SetHasDPAS() { m_HasDPAS = true; }
    bool GetHasEval() const { return m_HasEval; }
    void SetHasEval() { m_HasEval = true; }
    void IncStatelessWritesCount() { ++m_StatelessWritesCount; }
    void IncIndirectStatelessCount() { ++m_IndirectStatelessCount; }
    uint32_t GetStatelessWritesCount() const { return m_StatelessWritesCount; }
    uint32_t GetIndirectStatelessCount() const { return m_IndirectStatelessCount; }

    // In bytes
    uint32_t getGRFSize() const { return m_Platform->getGRFSize(); }
    // in DWORDs
    uint32_t getMinPushConstantBufferAlignmentInBytes() const { return m_Platform->getMinPushConstantBufferAlignment() * sizeof(DWORD); }

    // Note that for PVC A0 simd16, PVCLSCEnabled() returns true
    // but no LSC is generated!
    bool PVCLSCEnabled() const {
        return m_Platform->isCoreChildOf(IGFX_XE_HPC_CORE) && m_Platform->hasLSC();
    }

    e_alignment getGRFAlignment() const { return CVariable::getAlignment(getGRFSize()); }

    llvm::DenseMap<llvm::Value*, CVariable*>& GetSymbolMapping()
    {
        return symbolMapping;
    }

    llvm::DenseMap<llvm::Value*, CVariable*>& GetGlobalMapping()
    {
        return globalSymbolMapping;
    }

    llvm::DenseMap<llvm::Constant*, CVariable*>& GetConstantMapping()
    {
        return ConstantPool;
    }

    int64_t GetKernelArgOffset(CVariable* argV)
    {
        auto it = kernelArgToPayloadOffsetMap.find(argV);
        return it != kernelArgToPayloadOffsetMap.end() ? (int64_t) it->second : -1;
    }

    DebugInfoData& GetDebugInfoData();

    unsigned int GetPrimitiveTypeSizeInRegisterInBits(const llvm::Type* Ty) const;
    unsigned int GetPrimitiveTypeSizeInRegister(const llvm::Type* Ty) const;
    unsigned int GetScalarTypeSizeInRegisterInBits(const llvm::Type* Ty) const;
    unsigned int GetScalarTypeSizeInRegister(const llvm::Type* Ty) const;

    bool HasStackCalls() const { return m_HasStackCalls; }
    void SetHasStackCalls() { m_HasStackCalls = true; }
    bool IsIntelSymbolTableVoidProgram() const { return m_isIntelSymbolTableVoidProgram; }
    void SetIsIntelSymbolTableVoidProgram() { m_isIntelSymbolTableVoidProgram = true; }

    ////////////////////////////////////////////////////////////////////
    // NOTE: for vector load/stores instructions pass the
    // optional instruction argument checks additional constraints
    static Tristate shouldGenerateLSCQuery(
        const CodeGenContext& Ctx,
        llvm::Instruction* vectorLdStInst = nullptr,
        SIMDMode Mode = SIMDMode::UNKNOWN);
    bool shouldGenerateLSC(llvm::Instruction* vectorLdStInst = nullptr);
    bool forceCacheCtrl(llvm::Instruction* vectorLdStInst = nullptr);
    uint32_t totalBytesToStoreOrLoad(llvm::Instruction* vectorLdStInst);

protected:
    bool CompileSIMDSizeInCommon(SIMDMode simdMode);
    uint32_t GetShaderThreadUsageRate();
private:
    // Return DefInst's CVariable if it could be reused for UseInst, and return
    // nullptr otherwise.
    CVariable* reuseSourceVar(llvm::Instruction* UseInst,
        llvm::Instruction* DefInst,
        e_alignment preferredAlign);

    // Return nullptr if no source variable is reused. Otherwise return a
    // CVariable from its source operand.
    CVariable* GetSymbolFromSource(llvm::Instruction* UseInst,
        e_alignment preferredAlign);

protected:
    CShaderProgram* m_parent;
    CodeGenContext* m_ctx;
    WIAnalysis* m_WI;
    DeSSA* m_deSSA;
    CoalescingEngine* m_coalescingEngine;
    CodeGenPatternMatch* m_CG;
    llvm::DominatorTree* m_DT;
    const llvm::DataLayout* m_DL;
    GenXFunctionGroupAnalysis* m_FGA;
    VariableReuseAnalysis* m_VRA;

    uint m_numBlocks;
    IGC::IGCMD::MetaDataUtils* m_pMdUtils;

#if defined(_DEBUG) || defined(_INTERNAL)
    llvm::SpecificBumpPtrAllocator<CVariable> Allocator;
#else
    llvm::BumpPtrAllocator Allocator;
#endif

    // Mapping from formal argument to its variable or from function to its
    // return variable. Per kernel mapping. Used when llvm functions are
    // compiled into vISA subroutine
    llvm::DenseMap<llvm::Value*, CVariable*> globalSymbolMapping;

    llvm::DenseMap<llvm::Value*, CVariable*> symbolMapping;
    // Yet another map: a mapping from ccTuple to its corresponding root variable.
    // Variables that participate in congruence class tuples will be defined as
    // aliases (with respective offset) to the root variable.
    llvm::DenseMap<CoalescingEngine::CCTuple*, CVariable*> ccTupleMapping;
    // Constant pool.
    llvm::DenseMap<llvm::Constant*, CVariable*> ConstantPool;

    // keep a map when we generate accurate mask for vector value
    // in order to reduce register usage
    llvm::DenseMap<llvm::Value*, uint32_t> extractMasks;

    // keep a map for each kernel argument to its allocated payload offset
    llvm::DenseMap<CVariable*, uint32_t> kernelArgToPayloadOffsetMap;

    CEncoder encoder;
    std::vector<CVariable*> setup;
    std::vector<CVariable*> payloadLiveOutSetup;
    std::vector<CVariable*> payloadTempSetup;
    std::vector<CVariable*> patchConstantSetup;
    std::vector<CVariable*> perPrimitiveSetup;

    uint m_maxBlockId;

    CVariable* m_R0;
    CVariable* m_NULL;
    CVariable* m_TSC;
    CVariable* m_SR0;
    CVariable* m_CR0;
    CVariable* m_CE0;
    CVariable* m_DBG;
    CVariable* m_HW_TID;
    CVariable* m_SP;
    CVariable* m_FP;
    CVariable* m_SavedFP;
    CVariable* m_ARGV;
    CVariable* m_RETV;
    CVariable* m_SavedSRetPtr;
    CVariable* m_ImplArgBufPtr;
    CVariable* m_LocalIdBufPtr;

    std::vector<USC::SConstantGatherEntry> gatherMap;
    uint     m_ConstantBufferLength;
    uint     m_constantBufferMask;
    uint     m_constantBufferLoaded;
    uint64_t m_uavLoaded;
    uint     m_shaderResourceLoaded[4];
    uint     m_renderTargetLoaded;

    int  m_cbSlot;
    uint m_statelessCBPushedSize;
    uint m_NOSBufferSize = 0;

    /// holds max number of inputs that can be pushed for this shader unit
    static const uint32_t m_pMaxNumOfPushedInputs;

    int m_BarrierNumber;
    SProgramOutput m_simdProgram;

    // Holds max used binding table entry index.
    uint32_t m_BindingTableEntryCount;

    // Holds binding table entries bitmap.
    uint32_t m_BindingTableUsedEntriesBitmap;

    // for each vector BCI whose uses are all extractElt with imm offset,
    // we store the CVariables for each index
    llvm::DenseMap<llvm::Instruction*, llvm::SmallVector<CVariable*, 8>> m_VectorBCItoCVars;

    // Those two are for stateful token setup. It is a quick
    // special case checking. Once a generic approach is added,
    // this two fields shall be retired.
    bool m_HasGlobalStatelessMemoryAccess;
    bool m_HasConstantStatelessMemoryAccess;

    bool m_HasGlobalAtomics = false;

    bool m_HasDPAS = false;
    bool m_HasEval = false;
    bool m_passNOSInlinedata = false;

    uint32_t m_StatelessWritesCount = 0;
    uint32_t m_IndirectStatelessCount = 0;

    DebugInfoData diData;

    bool m_HasStackCalls = false;
    bool m_isIntelSymbolTableVoidProgram = false;
};

/// This class contains the information for the different SIMD version
/// of a kernel. Each kernel in the module is associated to one CShaderProgram
class CShaderProgram
{
public:
    typedef llvm::MapVector<llvm::Function*, CShaderProgram*> KernelShaderMap;
    CShaderProgram(CodeGenContext* ctx, llvm::Function* kernel);
    ~CShaderProgram();
    CShader* GetOrCreateShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
    CShader* GetShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
    void DeleteShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
    CodeGenContext* GetContext() { return m_context; }
    void FillProgram(SVertexShaderKernelProgram* pKernelProgram);
    void FillProgram(SHullShaderKernelProgram* pKernelProgram);
    void FillProgram(SDomainShaderKernelProgram* pKernelProgram);
    void FillProgram(SGeometryShaderKernelProgram* pKernelProgram);
    void FillProgram(SPixelShaderKernelProgram* pKernelProgram);
    void FillProgram(SComputeShaderKernelProgram* pKernelProgram);
    void FillProgram(SOpenCLProgramInfo* pKernelProgram);
    CBindlessShader* FillProgram(SBindlessProgram* pKernelProgram);
    ShaderStats* m_shaderStats;

protected:
    CShader*& GetShaderPtr(SIMDMode simd, ShaderDispatchMode mode);
    CShader* CreateNewShader(SIMDMode simd);
    void ClearShaderPtr(SIMDMode simd);

    inline bool hasShaderOutput(CShader* shader)
    {
        return (shader && shader->ProgramOutput()->m_programSize > 0);
    }

    inline void freeShaderOutput(CShader* shader)
    {
        if (hasShaderOutput(shader))
        {
            IGC::aligned_free(shader->ProgramOutput()->m_programBin);
            shader->ProgramOutput()->m_programSize = 0;
        }
    }

    CodeGenContext* m_context;
    llvm::Function* m_kernel;
    std::array<CShader*, 8> m_SIMDshaders;
};

struct SInstContext
{
    CVariable* flag;
    e_modifier dst_mod;
    bool invertFlag;
    void init()
    {
        flag = NULL;
        dst_mod = EMOD_NONE;
        invertFlag = false;
    }
};

static const SInstContext g_InitContext =
{
    NULL,
    EMOD_NONE,
    false,
};

void unify_opt_PreProcess(CodeGenContext* pContext);
// Forward declaration
struct PSSignature;
void CodeGen(PixelShaderContext* ctx, CShaderProgram::KernelShaderMap& shaders, PSSignature* pSignature = nullptr);
void CodeGen(OpenCLProgramContext* ctx, CShaderProgram::KernelShaderMap& shaders);
}
