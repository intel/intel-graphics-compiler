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
class DebugInfoData;

///--------------------------------------------------------------------------------------------------------
class CShader
{
public:
    friend class CShaderProgram;
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
    void         EOTRenderTarget();
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
    virtual bool hasReadWriteImage(llvm::Function& F) { return false; }
    bool CompileSIMDSizeInCommon();
    virtual bool CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F) { return true; }
    CVariable* LazyCreateCCTupleBackingVariable(
        CoalescingEngine::CCTuple* ccTuple,
        VISA_Type baseType = ISA_TYPE_UD);
    CVariable* GetSymbol(llvm::Value* value, bool fromConstantPool = false);
    void        AddSetup(uint index, CVariable* var);
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
        bool uniform,
        const CName &name)
    {
        return GetNewVariable(nbElement, type, align, uniform, 1, name);
    }
    CVariable* GetNewVariable(
        uint16_t nbElement,
        VISA_Type type,
        e_alignment align,
        bool uniform,
        uint16_t numberInstance,
        const CName &name);
    CVariable* GetNewVariable(const CVariable* from);
    CVariable* GetNewAddressVariable(
        uint16_t nbElement,
        VISA_Type type,
        bool uniform,
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
    bool        IsValueUsed(llvm::Value* value);
    CVariable*  GetGlobalCVar(llvm::Value* value);
    uint        GetNbElementAndMask(llvm::Value* value, uint32_t& mask);
    void        CreatePayload(uint regCount, uint idxOffset, CVariable*& payload, llvm::Instruction* inst, uint paramOffset, uint8_t hfFactor);
    uint        GetNbVectorElementAndMask(llvm::Value* value, uint32_t& mask);
    uint32_t    GetExtractMask(llvm::Value* value);
    uint16_t    AdjustExtractIndex(llvm::Value* value, uint16_t elemIndex);
    bool        GetIsUniform(llvm::Value* v) const;
    bool        InsideDivergentCF(llvm::Instruction* inst);
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
    CVariable* GetARGV();
    CVariable* GetRETV();
    CVariable* CreateSP();
    /// init stack-pointer at the beginning of the kernel
    void InitKernelStack(CVariable*& stackBase, CVariable*& stackAllocSize);
    /// save the stack-pointer when entering a stack-call function
    void SaveSP();
    /// restore the stack-pointer when exiting a stack-call function
    void RestoreSP();
    /// Get the max private mem size based on simd width
    uint32_t GetMaxPrivateMem();

    void        AllocateInput(CVariable* var, uint offset, uint instance = 0);
    void        AllocateOutput(CVariable* var, uint offset, uint instance = 0);
    CVariable* ImmToVariable(uint64_t immediate, VISA_Type type);
    CVariable* GetConstant(llvm::Constant* C, CVariable* dstVar = nullptr);
    CVariable* GetScalarConstant(llvm::Value* c);
    CVariable* GetStructVariable(llvm::Value* v, llvm::Constant* initValue = nullptr);
    CVariable* GetUndef(VISA_Type type);
    llvm::Constant* findCommonConstant(llvm::Constant* C, uint elts, uint currentEmitElts, bool& allSame);
    virtual unsigned int GetGlobalMappingValue(llvm::Value* c);
    virtual CVariable* GetGlobalMapping(llvm::Value* c);
    CVariable* BitCast(CVariable* var, VISA_Type newType);
    void        ResolveAlias(CVariable* var);
    void        CacheArgumentsList();
    void        MapPushedInputs();
    void        CreateGatherMap();
    void        CreateConstantBufferOutput(SKernelProgram* pKernelProgram);
    void        CreateFunctionSymbol(llvm::Function* pFunc);
    void        CreateGlobalSymbol(llvm::GlobalVariable* pGlobal);

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
    bool        IsValueCoalesced(llvm::Value* v);

    bool        GetHasBarrier() const { return m_HasBarrier; }
    void        SetHasBarrier() { m_HasBarrier = true; }

    void        GetSimdOffsetBase(CVariable*& pVar);
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
    unsigned m_spillSize = 0;
    float m_spillCost = 0;          // num weighted spill inst / total inst

    std::vector<llvm::Value*> m_argListCache;

    /// The size in byte used by igc (non-spill space). And this
    /// is the value passed to VISA so that VISA's spill, if any,
    /// will go after this space.
    uint m_ScratchSpaceSize;

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
    /// This method is used to create the vISA variable for function F's formal return value
    CVariable* getOrCreateReturnSymbol(llvm::Function* F);
    /// This method is used to create the vISA variable for function F's formal argument
    CVariable* getOrCreateArgumentSymbol(
        llvm::Argument* Arg,
        bool ArgInCallee, // true if Arg isn't in current func
        bool useStackCall = false);
    VISA_Type GetType(llvm::Type* type);

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
                    m_uavLoaded |= (uint64_t)1 << (typeBti);
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

                if (bufType == RESOURCE)
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
                else if (bufType == CONSTANT_BUFFER)
                {
                    m_constantBufferLoaded |= BITMASK_RANGE(0, m_pBtiLayout->GetConstantBufferIndexSize());
                }
                else if (bufType == UAV)
                {
                    //m_uavLoaded |= BITMASK_RANGE(0, m_pBtiLayout->GetUavIndexSize());
                    m_uavLoaded |= ((~((0xffffffffffffffff) << (m_pBtiLayout->GetUavIndexSize() + 1))) & (~((0xffffffffffffffff) << 0)));
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

    // When debug info is enabled, this vector stores mapping of
    // VISA index->Gen ISA offset. Currently, some APIs uses this
    // to dump out elf.
    std::vector<std::pair<unsigned int, unsigned int>> m_VISAIndexToGenISAOff;
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

    DebugInfoData* diData = nullptr;

    void SetHasGlobalStatelessAccess() { m_HasGlobalStatelessMemoryAccess = true; }
    bool GetHasGlobalStatelessAccess() const { return m_HasGlobalStatelessMemoryAccess; }
    void SetHasConstantStatelessAccess() { m_HasConstantStatelessMemoryAccess = true; }
    bool GetHasConstantStatelessAccess() const { return m_HasConstantStatelessMemoryAccess; }
    void SetHasGlobalAtomics() { m_HasGlobalAtomics = true; }
    bool GetHasGlobalAtomics() const { return m_HasGlobalAtomics; }
    calignmentSize as;

    // In bytes
    uint32_t getGRFSize() const { return m_Platform->getGRFSize(); }


    e_alignment getGRFAlignment() const { return CVariable::getAlignment(getGRFSize()); }

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

    llvm::BumpPtrAllocator Allocator;

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

    CEncoder encoder;
    std::vector<CVariable*> setup;
    std::vector<CVariable*> patchConstantSetup;

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
    CVariable* m_SavedSP;
    CVariable* m_ARGV;
    CVariable* m_RETV;

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

    bool m_HasBarrier;
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
    CodeGenContext* GetContext() { return m_context; }
    void FillProgram(SVertexShaderKernelProgram* pKernelProgram);
    void FillProgram(SHullShaderKernelProgram* pKernelProgram);
    void FillProgram(SDomainShaderKernelProgram* pKernelProgram);
    void FillProgram(SGeometryShaderKernelProgram* pKernelProgram);
    void FillProgram(SPixelShaderKernelProgram* pKernelProgram);
    void FillProgram(SComputeShaderKernelProgram* pKernelProgram);
    void FillProgram(SOpenCLProgramInfo* pKernelProgram);
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
