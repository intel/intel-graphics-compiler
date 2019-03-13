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
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/helper.h"

#include "visa_wa.h"


namespace IGC
{
class CShader;
class CVariable;

struct SFlag
{
    CVariable* var;
    e_predMode mode;
    bool invertFlag;
    void init()
    {
        var = NULL;
        mode = EPRED_NORMAL;
        invertFlag = false;
    }
};

struct SModifier
{
    uint16_t subReg;
    uint8_t subVar;
    uint8_t region[3];
    e_modifier mod;
    bool specialRegion;
    void init()
    {
        mod = EMOD_NONE;
        subVar = 0;
        subReg = 0;
        specialRegion = false;
    }
};

struct SAlias
{
    CVariable* m_rootVar;
    VISA_Type  m_type;
    SAlias(CVariable* var, VISA_Type type) :
        m_rootVar(var), m_type(type)
    { }
};

struct SAliasMapInfo {
    static inline SAlias getEmptyKey() { return SAlias(nullptr, ISA_TYPE_UD); }
    static inline SAlias getTombstoneKey() { return SAlias(nullptr, ISA_TYPE_D); }
    static unsigned getHashValue(const SAlias &Val) {
        return llvm::DenseMapInfo<CVariable*>::getHashValue(Val.m_rootVar) ^ Val.m_type;
    }
    static bool isEqual(const SAlias &LHS, const SAlias &RHS) {
        return LHS.m_rootVar == RHS.m_rootVar && LHS.m_type == RHS.m_type;
    }
};

/// Helps representing URB write channel masks in a way that provides type safety and adapts to
/// the channel mask format required by V-ISA interface.
class URBChannelMask
{
public:
    explicit URBChannelMask(unsigned int bitmask) : m_bitmask(bitmask) {}

    /// Returns the size of bitmask, 
    /// defined as the position of the most significant bit with value 1.
    /// E.g. size(10001) == 5, size(1) == 1 , size(1111) = 4
    size_t size() const;

    /// Returns channel mask in the format expected by V-ISA.
    /// If the mask is full (i.e. consists of all 1) the return value must be 0xFF
    /// that means 'no channel mask'. In other cases it is the actual stored mask
    /// E.g. 1010 asVISAMask --> 1010, 111 asVISAMask --> 11111111 (full mask case)
    unsigned int asVISAMask() const;

    // returns true if all channels are set (i.e., we can skip the channel mask)
    bool isAllSet() const
    {
        return ((m_bitmask + 1) & m_bitmask) == 0;
    }
private:
    unsigned int m_bitmask;
};

struct SEncoderState
{
    SModifier m_srcOperand[4];
    SModifier m_dstOperand;
    SFlag     m_flag;
    SIMDMode  m_simdSize;
    SIMDMode  m_uniformSIMDSize;
    e_mask    m_mask;
    bool      m_noMask;
    bool      m_SubSpanDestination;
    bool      m_secondHalf;
};

class CEncoder
{
public:
    void InitEncoder( bool canAbortOnSpill, bool hasStackCall);
    SEncoderState CopyEncoderState();
    void SetEncoderState(SEncoderState &newState);

    void SetKernelStackPointer64();  
    void SetStackFunctionArgSize(uint size);  // size in GRFs
    void SetStackFunctionRetSize(uint size);  // size in GRFs

    void GetVISAPredefinedVar(CVariable* pVar, PreDefined_Vars var);
    void CreateVISAVar(CVariable* var);
    void DeclareInput(CVariable* var, uint offset, uint instance);
    void MarkAsOutput(CVariable* var);
    void Compile();
    CEncoder();
    ~CEncoder();
    void SetProgram(CShader* program);
    void Jump(CVariable* flag, uint label);
    void Label(uint label);
    uint GetNewLabelID();
    void DwordAtomicRaw(AtomicOp atomic_op,
                        const ResourceDescriptor &bindingTableIndex,
                        CVariable *dst, CVariable *elem_offset, CVariable *src0,
                        CVariable *src1, bool is16Bit = false);
    void AtomicRawA64(AtomicOp atomic_op, CVariable *dst,
                      CVariable *elem_offset, CVariable *src0, CVariable *src1,
                      unsigned short bitwidth);
    void Cmp(e_predicate p, CVariable* dst, CVariable* src0, CVariable* src1);
    void Select(CVariable* flag, CVariable* dst, CVariable* src0, CVariable* src1);
    void GenericAlu(e_opcode opcode, CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2 = nullptr);
    void URBWrite(CVariable* src, const int payloadElementOffset, CVariable* offset, CVariable* urbHandle, CVariable* dynamicMask);
    void Send(CVariable* dst, CVariable* src, uint exDesc, CVariable* messDescriptor, bool isSendc = false);
    void Send(CVariable* dst, CVariable* src, uint ffid, CVariable* exDesc, CVariable* messDescriptor, bool isSendc = false);
    void Sends(CVariable* dst, CVariable* src0, CVariable* src1, uint ffid, CVariable* exDesc, CVariable* messDescriptor, bool isSendc = false);
    void RenderTargetWrite(CVariable* var[], 
                                 bool isUndefined[],
                                 bool lastRenderTarget,  
                                 bool perSample,
                                 bool coarseMode,
                                 bool headerMaskFromCe0,
                                 CVariable* bindingTableIndex,
                                 CVariable* RTIndex,
                                 CVariable* source0Alpha,
                                 CVariable* oMask,
                                 CVariable* depth,
                                 CVariable *stencil,
                                 CVariable *CPSCounter,
                                 CVariable *sampleIndex,
                                 CVariable *r1Reg);
    void Sample(EOPCODE subOpcode, uint writeMask, CVariable* offset, const ResourceDescriptor& bindingTableIndex, const SamplerDescriptor& SamplerIdx, uint numSources, CVariable* dst, llvm::SmallVector<CVariable*, 4>& payload, bool zeroLOD, bool cpsEnable, bool feedbackEnable, bool nonUniformState = false);
    void Load(EOPCODE subOpcode, uint writeMask, CVariable* offset, const ResourceDescriptor& resource, uint numSources, CVariable* dst, llvm::SmallVector<CVariable*, 4>& payload, bool zeroLOD, bool feedbackEnable);
    void Info(EOPCODE subOpcode, uint writeMask, const ResourceDescriptor& resource, CVariable* lod, CVariable* dst);
    void Gather4Inst(EOPCODE subOpcode, CVariable* offset, const ResourceDescriptor& resource, const SamplerDescriptor& sampler, uint numSources, CVariable* dst, llvm::SmallVector<CVariable*, 4>& payload, uint channel,  bool feedbackEnable);

    void OWLoad(CVariable* dst, const ResourceDescriptor& resource, CVariable* offset, bool owordAligned, uint dstSize, uint dstOffset = 0);
    void OWStore(CVariable* dst, e_predefSurface surfaceType, CVariable* bufidx, CVariable* offset, uint dstSize, uint srcOffset);

    void AddrAdd(CVariable* dst, CVariable* src0, CVariable* src1);
    void Barrier(e_barrierKind BarrierKind);
    void Fence(bool CommitEnable, 
                bool L3_Flush_RW_Data, 
                bool L3_Flush_Constant_Data, 
                bool L3_Flush_Texture_Data, 
                bool L3_Flush_Instructions,
                bool Global_Mem_Fence,
                bool L1_Flush,
                bool SWFence);
    void FlushSamplerCache();
    void EndOfThread();
    void OWLoadA64(CVariable* dst, CVariable* offset, uint dstSize, uint dstOffset = 0);
    void OWStoreA64(CVariable* dst, CVariable* offset, uint dstSize, uint srcOffset);
    void MediaBlockMessage(ISA_Opcode subOpcode,
        CVariable* dst,
        e_predefSurface surfaceType,
        CVariable* bufId,
        CVariable* xOffset,
        CVariable* yOffset,
        uint modifier,
        unsigned char blockWidth,
        unsigned char blockHeight,
        uint plane);
    void GatherA64(CVariable* dst, CVariable* offset, unsigned elementSize, unsigned numElems);
    void ScatterA64(CVariable* val, CVariable* offset, unsigned elementSize, unsigned numElems);
    void ByteGather(CVariable *dst, const ResourceDescriptor& resource, CVariable *offset, unsigned elementSize, unsigned numElems);
    void ByteScatter(CVariable *src, const ResourceDescriptor& resource, CVariable *offset, unsigned elementSize, unsigned numElems);
    void Gather4Scaled(CVariable *dst, const ResourceDescriptor& resource, CVariable *offset);
    void Gather4ScaledNd(CVariable *dst, const ResourceDescriptor& resource, CVariable *offset, unsigned nd);
    void Scatter4Scaled(CVariable *src, const ResourceDescriptor& resource, CVariable *offset);
    void Gather4A64(CVariable *dst, CVariable *offset);
    void Scatter4A64(CVariable *src, CVariable *offset);
    void BoolToInt(CVariable* dst, CVariable* src) ;
    void Copy(CVariable* dst, CVariable* src);
    void SubroutineCall(CVariable *flag, llvm::Function *F);
    void SubroutineRet(CVariable *flag);
    void StackCall(CVariable *flag, llvm::Function *F, unsigned char argSize, unsigned char retSize);
    void IndirectStackCall(CVariable* flag, CVariable* funcPtr, unsigned char argSize, unsigned char retSize);
    void StackRet(CVariable *flag);
    void Loc(unsigned int line);
    void File(std::string& s);

    inline void Jump(uint label);
    inline void Cast(CVariable* dst, CVariable* src);
    inline void Add(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Bfi(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2, CVariable* src3);
    inline void Bfe(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
    inline void Bfrev(CVariable* dst, CVariable* src0);
    inline void CBit(CVariable* dst, CVariable* src0);
    inline void Fbh(CVariable* dst, CVariable* src0);
    inline void Fbl(CVariable* dst, CVariable* src0);
    inline void Mul(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Pow(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Div(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Shl(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Shr(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void MulH(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Cos(CVariable* dst, CVariable* src0);
    inline void Sin(CVariable* dst, CVariable* src0);
    inline void Log(CVariable* dst, CVariable* src0);
    inline void Exp(CVariable* dst, CVariable* src0);
    inline void Frc(CVariable* dst, CVariable* src0);
    inline void Sqrt(CVariable* dst, CVariable* src0);
    inline void Floor(CVariable* dst, CVariable* src0);
    inline void Ceil(CVariable* dst, CVariable* src0);
    inline void Ctlz(CVariable* dst, CVariable* src0);
    inline void Truncate(CVariable* dst, CVariable* src0);
    inline void RoundNE(CVariable* dst, CVariable* src0);
    inline void Mod(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Rsqrt(CVariable* dst, CVariable* src0);
    inline void Inv(CVariable* dst, CVariable* src0);
    inline void Not(CVariable* dst, CVariable* src0);
    inline void Mad(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
    inline void Lrp(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
    inline void Xor(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Or(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void And(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Pln(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void SendC(CVariable* dst, CVariable* src, uint exDesc, CVariable* messDescriptor);
    inline void SendC(CVariable* dst, CVariable* src, uint ffid, CVariable* exDesc,CVariable* messDescriptor);
    inline void LoadMS(EOPCODE subOpcode, uint writeMask, CVariable* offset, const ResourceDescriptor& resource, uint numSources, CVariable* dst, llvm::SmallVector<CVariable*, 4>& payload, bool feedbackEnable);
    inline void SetP(CVariable*  dst, CVariable* src);
    inline void Gather(CVariable* dst, CVariable* bufidx, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize);
    inline void Gather4(CVariable* dst, CVariable* bufidx, CVariable* offset, e_predefSurface surface);
    inline void TypedRead4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV, CVariable* pR, CVariable* pLOD, CVariable* pDst, uint writeMask);
    inline void TypedWrite4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV, CVariable* pR, CVariable* pLOD, CVariable* pSrc);
    inline void Scatter(CVariable* val, CVariable* bufidx, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize);
    inline void Scatter4(CVariable* val, CVariable* bufIdx, CVariable* offset, CVariable* gOffset, e_predefSurface surface);
    inline void IShr( CVariable* dst, CVariable* src0, CVariable* src1 );
    inline void Min(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void Max(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void UAddC(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void USubB(CVariable* dst, CVariable* src0, CVariable* src1);
    inline void IEEESqrt(CVariable* dst, CVariable* src0);
    inline void IEEEDivide(CVariable* dst, CVariable* src0, CVariable* src1);
    void AddPair(CVariable *Lo, CVariable *Hi, CVariable *L0, CVariable *H0, CVariable *L1, CVariable *H1);
    void SubPair(CVariable *Lo, CVariable *Hi, CVariable *L0, CVariable *H0, CVariable *L1, CVariable *H1);
    inline void dp4a(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
    void Lifetime(VISAVarLifetime StartOrEnd, CVariable* dst);
    // VME
    void SendVmeIme(
        CVariable* bindingTableIndex,
        unsigned char streamMode,
        unsigned char searchControlMode,
        CVariable* uniInputVar,
        CVariable* imeInputVar,
        CVariable* ref0Var,
        CVariable* ref1Var,
        CVariable* costCenterVar,
        CVariable* outputVar );

    void SendVmeFbr(
        CVariable* bindingTableIndex,
        CVariable* uniInputVar,
        CVariable* fbrInputVar,
        CVariable* FBRMbModeVar,
        CVariable* FBRSubMbShapeVar,
        CVariable* FBRSubPredModeVar,
        CVariable* outputVar );

    void SendVmeSic(
        CVariable* bindingTableIndex,
        CVariable* uniInputVar,
        CVariable* sicInputVar,
        CVariable* outputVar );

    // VA
    void SendVideoAnalytic(
        llvm::GenIntrinsicInst* inst,
        CVariable* vaResult,
        CVariable* coords,
        CVariable* size,
        CVariable* srcImg,
        CVariable* sampler );

    void SetDstSubVar(uint subVar);
    void SetDstSubReg(uint subReg);
    void SetSrcSubVar(uint srcNum, uint subVar);
    void SetSrcSubReg(uint srcNum, uint subReg);
    void SetDstModifier(e_modifier mod);
    void SetDstModifier(const DstModifier& modifier);
    void SetSrcModifier(uint srcNum, e_modifier mod);
    void SetPredicate(CVariable* flag);
    void SetInversePredicate(bool inv);
    void SetPredicateMode(e_predMode mode);
    void SetSrcRegion(uint srcNum, uint vStride, uint width, uint hStride);
    void SetDstRegion(uint hStride);
    inline void SetNoMask();
    inline void SetMask(e_mask mask);
    inline void SetSimdSize(SIMDMode size);
    inline void SetUniformSIMDSize(SIMDMode size);
    inline void SetSubSpanDestination(bool subspan);
    inline bool IsSubSpanDestination();
    inline void SetSecondHalf(bool secondHalf);
    inline bool IsSecondHalf();

    void Wait();

    VISAKernel* GetVISAKernel() { return vKernel; }
    void Init();
    void Push();

    void SetFloatDenormMode(VISAKernel* vKernel, Float_DenormMode mode16, 
                            Float_DenormMode mode32, Float_DenormMode mode64);
    void SetVectorMask(bool vMask);
    // RM bits in CR0.0.
    //    float RM bits: [5:4];
    //    int RM (float -> int): Bit 12: 0 -> rtz; 1 -> using Float RM
    enum RoundingMode {
        // float rounding mode
        RoundToNearestEven = 0x00,
        RoundToPositive = 0x10,
        RoundToNegative = 0x20,
        RoundToZero = 0x30,
        // int rounding mode, use FP RM for all rounding modes but rtz.
        RoundToNearestEven_int = 0x1000,
        RoundToPositive_int = 0x1010,
        RoundToNegative_int = 0x1020,
        RoundToZero_int = 0x1030,

        IntAndFPRoundingModeMask = 0x1030
    };

    // Switches from actualMode to newMode
    void SetFloatRoundingMode(RoundingMode actualMode, RoundingMode newMode);
    // Switches from actualMode to default rounding mode
    void SetFloatRoundingModeDefault(RoundingMode actualMode);
    // Get rounding mode of encoder
    RoundingMode getEncoderRoundingMode(Float_RoundingMode FP_RM);

    static uint GetCISADataTypeSize(VISA_Type type);
    static e_alignment GetCISADataTypeAlignment(VISA_Type type);
    static VISASampler3DSubOpCode ConvertSubOpcode(EOPCODE subOpcode, bool zeroLOD);
    
    // Wrappers for (potentially) common queries on types
    static bool IsIntegerType(VISA_Type type);
    static bool IsFloatType(VISA_Type type);

    void SetVISAWaTable(WA_TABLE const& waTable);

    /// \brief Initialize per function states and starts vISA emission 
    /// as a vISA subroutine
    void BeginSubroutine(llvm::Function *F);
    /// \brief Initialize per function states and starts vISA emission 
    /// as a vISA stack-call function
    void BeginStackFunction(llvm::Function *F);

    void DestroyVISABuilder();

    void AddFunctionSymbol(llvm::Function* F, CVariable* fvar);

private:
    // helper functions
    VISA_VectorOpnd* GetSourceOperand(CVariable* var, const SModifier& mod);
    VISA_VectorOpnd* GetSourceOperandNoModifier(CVariable* var);
    VISA_VectorOpnd* GetDestinationOperand(CVariable* var, const SModifier& mod);
    VISA_RawOpnd* GetRawSource(CVariable* var, uint offset = 0);
    VISA_RawOpnd* GetRawDestination(CVariable* var, unsigned offset = 0);
    VISA_PredOpnd* GetFlagOperand(const SFlag& flag);
    VISA_StateOpndHandle* GetVISASurfaceOpnd(e_predefSurface surfaceType, CVariable* bti);
    VISA_StateOpndHandle* GetVISASurfaceOpnd(const ResourceDescriptor& resource);
    VISA_LabelOpnd* GetLabel(uint label);
    VISA_LabelOpnd* GetFuncLabel(llvm::Function *F);

    VISAFunction* GetStackFunction(llvm::Function *F);

    VISA_VectorOpnd* GetUniformSource(CVariable* var);
    VISA_StateOpndHandle* GetBTIOperand(uint bindingTableIndex);
    VISA_StateOpndHandle* GetSamplerOperand(CVariable* sampleIdx);
    VISA_StateOpndHandle* GetSamplerOperand(const SamplerDescriptor& sampler, bool& isIdxLT16);
    void GetRowAndColOffset(CVariable* var, unsigned int subVar, unsigned int subreg, unsigned char& rowOff, unsigned char& colOff);

    VISA_GenVar* GetVISAVariable(CVariable* var);
    Common_VISA_EMask_Ctrl ConvertMaskToVisaType(e_mask mask, bool noMask);

    // Generic encoding functions
    void MinMax(CISA_MIN_MAX_SUB_OPCODE subopcode, CVariable* dst, CVariable* src0, CVariable* src1);
    void DataMov(ISA_Opcode opcode, CVariable* dst, CVariable* src);
    void LogicOp(
        ISA_Opcode opcode, 
        CVariable* dst, 
        CVariable* src0, 
        CVariable* src1 = nullptr, 
        CVariable* src2 = nullptr,
        CVariable* src3 = nullptr);
    void Arithmetic(
        ISA_Opcode opcode, 
        CVariable* dst, 
        CVariable* src0 = nullptr, 
        CVariable* src1 = nullptr, 
        CVariable* src2 = nullptr);
    void CarryBorrowArith(ISA_Opcode opcode, CVariable* dst, CVariable* src0, CVariable*src1);
    void ScatterGather(
        ISA_Opcode opcode, 
        CVariable* srcdst, 
        CVariable* bufId, 
        CVariable* offset, 
        CVariable* gOffset,
        e_predefSurface surface,
        int elementSize);
    void ScatterGather4(
        ISA_Opcode opcode,
        CVariable* srcdst,
        CVariable* bufId,
        CVariable* offset,
        CVariable* gOffset,
        e_predefSurface surface);
    void TypedReadWrite(
        ISA_Opcode opcode,
        const ResourceDescriptor& resource,
        CVariable* pU,
        CVariable* pV,
        CVariable* pR,
        CVariable* pLOD,
        CVariable* pSrcDst,
        uint writeMask);

    Common_ISA_Exec_Size  GetAluExecSize(CVariable* dst) const;
    Common_VISA_EMask_Ctrl GetAluEMask(CVariable* dst);
    bool IsSat();
    bool NeedSplitting(CVariable *var, const SModifier &mod,
                       unsigned &numParts,
                       bool isSource = false) const;
    SModifier SplitVariable(Common_ISA_Exec_Size fromExecSize,
                            Common_ISA_Exec_Size toExecSize,
                            unsigned thePart,
                            CVariable *var, const SModifier &mod,
                            bool isSource = false) const;
    Common_ISA_Exec_Size SplitExecSize(Common_ISA_Exec_Size fromExecSize,
                                       unsigned numParts) const;
    Common_VISA_EMask_Ctrl SplitEMask(Common_ISA_Exec_Size fromExecSize,
                                      Common_ISA_Exec_Size toExecSize,
                                      unsigned thePart,
                                      Common_VISA_EMask_Ctrl execMask) const;

    // Split SIMD16 message data payload(MDP) for scattered/untyped write
    // messages into two SIMD8 MDPs : V0 and V1.
    void SplitMDP16To8(CVariable* MDP, uint32_t MDPOfst, uint32_t NumBlks, CVariable* V0, CVariable* V1);
    // Merge two SIMD8 MDPs (V0 & V1) for scattered/untyped read messages into one SIMD16 message : MDP
    void MergeMDP8To16(CVariable* V0, CVariable *V1, uint32_t NumBlks, CVariable* MDP, uint32_t MDPOfst);

    // save compile time by avoiding retry if the amount of spill is (very) small
    bool AvoidRetryOnSmallSpill() const;

    void CreateFunctionSymbolTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries);
    void CreateFunctionRelocationTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries);

protected:
    // encoder states
    SEncoderState m_encoderState;

    llvm::DenseMap<SAlias, CVariable*, SAliasMapInfo> m_aliasesMap;

    VISA_WA_TABLE m_WaTable;

    // Typically IGC just use ones vKernel for every vISA::compile call,
    // in those cases, vKernel and vMainKernel should be the same.
    // Only when using stack-call, vKernel pointer changes every time
    // IGC addes a vISA kernel or function object, but the vMainKernel
    // always pointing to the first kernel added during InitEncoder.
    VISAKernel*   vKernel;
    VISAKernel*   vMainKernel;
    VISABuilder* vbuilder;
    
    bool m_enableVISAdump;
    std::vector<VISA_LabelOpnd*> labelMap;

    /// Per kernel label counter
    unsigned labelCounter;

    /// Keep a map between a function and its label, per kernel state.
    llvm::SmallDenseMap<llvm::Function *, VISA_LabelOpnd *> funcLabelMap;
    /// Keep a map between a stack-called function and the corresponding vISA function
    llvm::SmallDenseMap<llvm::Function *, VISAFunction*> stackFuncMap;

    // dummy variables
    VISA_SurfaceVar* dummySurface;
    VISA_SamplerVar* samplervar;

    CShader* m_program;
};

inline void CEncoder::Jump(uint label)
{
    Jump(NULL,label);
}

inline void CEncoder::Bfi(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2, CVariable* src3)
{
    LogicOp(ISA_BFI, dst, src0, src1, src2, src3);
}

inline void CEncoder::Bfe(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
{
    LogicOp(ISA_BFE, dst, src0, src1, src2);
}

inline void CEncoder::Bfrev(CVariable* dst, CVariable* src0)
{
    LogicOp(ISA_BFREV, dst, src0);
}

inline void CEncoder::CBit(CVariable* dst, CVariable* src)
{
    LogicOp(ISA_CBIT, dst, src);
}

inline void CEncoder::Fbh(CVariable* dst, CVariable* src)
{
    LogicOp(ISA_FBH, dst, src);
}

inline void CEncoder::Fbl(CVariable* dst, CVariable* src)
{
    LogicOp(ISA_FBL, dst, src);
}

inline void CEncoder::Mul(CVariable* dst, CVariable* src0, CVariable* src1)
{
    Arithmetic(ISA_MUL, dst, src0, src1);
}

inline void CEncoder::Pow(CVariable* dst, CVariable* src0, CVariable* src1)
{
    Arithmetic(ISA_POW, dst, src0, src1);
}

inline void CEncoder::Div(CVariable* dst, CVariable* src0, CVariable* src1)
{
    Arithmetic(ISA_DIV, dst, src0, src1);
}

inline void CEncoder::Add(CVariable* dst, CVariable* src0, CVariable* src1)
{
    Arithmetic(ISA_ADD, dst, src0, src1);
}
    
inline void CEncoder::Shl(CVariable* dst, CVariable* src0, CVariable* src1)
{
    LogicOp(ISA_SHL, dst, src0, src1);
}

inline void CEncoder::IShr(CVariable* dst, CVariable* src0, CVariable* src1)
{
    LogicOp(ISA_ASR, dst, src0, src1);
}

inline void CEncoder::Shr(CVariable* dst, CVariable* src0, CVariable* src1)
{
    LogicOp(ISA_SHR, dst, src0, src1);
}

inline void CEncoder::MulH( CVariable* dst, CVariable* src0, CVariable* src1 )
{
    Arithmetic(ISA_MULH, dst, src0, src1);
}

inline void CEncoder::Cos(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_COS, dst, src0);
}

inline void CEncoder::Sin(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_SIN, dst, src0);
}

inline void CEncoder::Log(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_LOG, dst, src0);
}

inline void CEncoder::Exp(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_EXP, dst, src0);
}

inline void CEncoder::Sqrt(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_SQRT, dst, src0);
}

inline void CEncoder::Floor(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_RNDD, dst, src0);
}

inline void CEncoder::Ceil(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_RNDU, dst, src0);
}

inline void CEncoder::Ctlz(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_LZD, dst, src0);
}

inline void CEncoder::Truncate(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_RNDZ, dst, src0);
}

inline void CEncoder::RoundNE(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_RNDE, dst, src0);
}

inline void CEncoder::Mod( CVariable* dst, CVariable* src0, CVariable* src1 )
{
    Arithmetic(ISA_MOD, dst, src0, src1);
}

inline void CEncoder::Rsqrt(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_RSQRT, dst , src0);
}

inline void CEncoder::Inv(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_INV, dst, src0);
}

inline void CEncoder::Not(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_NOT, dst, src0);
}

inline void CEncoder::Frc(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_FRC, dst, src0);
}

inline void CEncoder::Pln(CVariable* dst, CVariable* src0, CVariable* src1)
{
    Arithmetic(ISA_PLANE, dst, src0, src1);
}

inline void CEncoder::Cast(CVariable* dst, CVariable* src)
{
    DataMov(ISA_MOV, dst, src);
}

inline void CEncoder::Mad(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
{
    Arithmetic(ISA_MAD, dst, src0, src1, src2);
}

inline void CEncoder::Lrp(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
{
    Arithmetic(ISA_LRP, dst, src0, src1, src2);
}

inline void CEncoder::Xor(CVariable* dst, CVariable* src0, CVariable* src1)
{
    LogicOp(ISA_XOR, dst, src0, src1);
}

inline void CEncoder::Or(CVariable* dst, CVariable* src0, CVariable* src1)
{
    LogicOp(ISA_OR, dst, src0, src1);
}

inline void CEncoder::And(CVariable* dst, CVariable* src0, CVariable* src1)
{
    LogicOp(ISA_AND, dst, src0, src1);
}

inline void CEncoder::SetP(CVariable* dst, CVariable* src0)
{
    // We always need no mask when doing a set predicate
    m_encoderState.m_noMask = true;
    DataMov(ISA_SETP, dst, src0);
}

inline void CEncoder::Min(CVariable* dst, CVariable* src0, CVariable* src1)
{
    MinMax(CISA_DM_FMIN, dst, src0, src1);
}

inline void CEncoder::Max(CVariable* dst, CVariable* src0, CVariable* src1)
{
    MinMax(CISA_DM_FMAX, dst, src0, src1);
}

inline void CEncoder::UAddC(CVariable* dst, CVariable* src0, CVariable* src1)
{
    CarryBorrowArith(ISA_ADDC, dst, src0, src1);
}

inline void CEncoder::USubB(CVariable* dst, CVariable* src0, CVariable* src1)
{
    CarryBorrowArith(ISA_SUBB, dst, src0, src1);
} 

inline void CEncoder::LoadMS(EOPCODE subOpcode, uint writeMask, CVariable* offset,
    const ResourceDescriptor& resource, uint numSources, CVariable* dst,
    llvm::SmallVector<CVariable*, 4>& payload, bool feedbackEnable)
{
    Load(subOpcode, writeMask, offset, resource, numSources, dst, payload, false, feedbackEnable);
}

inline void CEncoder::Gather(CVariable* dst, CVariable* bufId, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize)
{
    ScatterGather(ISA_GATHER, dst, bufId, offset, gOffset, surface, elementSize);
}

inline void CEncoder::Gather4(CVariable* dst, CVariable* bufId, CVariable* offset, e_predefSurface surface)
{
    ScatterGather4(ISA_GATHER4, dst, bufId, offset, nullptr, surface);
}

inline void CEncoder::TypedRead4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV,
    CVariable* pR, CVariable* pLOD, CVariable* pDst, uint writeMask)
{
    TypedReadWrite(ISA_GATHER4_TYPED, resource, pU, pV, pR, pLOD, pDst, writeMask);
}

inline void CEncoder::TypedWrite4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV,
    CVariable* pR, CVariable* pLOD, CVariable* pSrc)
{
    TypedReadWrite(ISA_SCATTER4_TYPED, resource, pU, pV, pR, pLOD, pSrc, 0);
}

inline void CEncoder::Scatter(CVariable* val, CVariable* bufidx, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize)
{
    ScatterGather(ISA_SCATTER, val, bufidx, offset, gOffset, surface, elementSize);
}

inline void CEncoder::Scatter4(CVariable* val, CVariable* bufIdx, CVariable* offset, CVariable* gOffset, e_predefSurface surface)
{
    ScatterGather4(ISA_SCATTER4, val, bufIdx, offset, gOffset, surface);
}

inline void CEncoder::SendC(CVariable* dst, CVariable* src, uint exDesc, CVariable* messDescriptor)
{
    Send(dst, src, exDesc, messDescriptor, true);
}

inline void CEncoder::SendC(CVariable* dst, CVariable* src, uint ffid, CVariable* exDesc, CVariable* messDescriptor)
{
    Send(dst, src, ffid, exDesc, messDescriptor, true);
}

inline void CEncoder::IEEESqrt(CVariable* dst, CVariable* src0)
{
    Arithmetic(ISA_SQRTM, dst, src0);
}

inline void CEncoder::IEEEDivide(CVariable* dst, CVariable* src0, CVariable* src1)
{
    Arithmetic(ISA_DIVM, dst, src0, src1);
}

inline void CEncoder::dp4a(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2) {
    Arithmetic(ISA_DP4A, dst, src0, src1, src2);
}

inline void CEncoder::SetNoMask()
{
    m_encoderState.m_noMask = true;
}

inline void CEncoder::SetMask(e_mask mask)
{
    m_encoderState.m_mask = mask;
}

inline void CEncoder::SetSimdSize(SIMDMode size)
{
    m_encoderState.m_simdSize = size;
}

inline void CEncoder::SetUniformSIMDSize(SIMDMode size)
{
    m_encoderState.m_uniformSIMDSize = size;
}

inline void CEncoder::SetSubSpanDestination(bool subspan)
{
    m_encoderState.m_SubSpanDestination = subspan;
}

inline void CEncoder::SetSecondHalf(bool secondHalf)
{
    m_encoderState.m_secondHalf = secondHalf;
}

inline bool CEncoder::IsSecondHalf()
{
    return m_encoderState.m_secondHalf;
}

inline bool CEncoder::IsSubSpanDestination()
{
    return m_encoderState.m_SubSpanDestination;
}

VISA_Modifier ConvertModifierToVisaType(e_modifier modifier);
Common_ISA_Cond_Mod ConvertCondModToVisaType(e_predicate condMod);
Common_ISA_Oword_Num  ConvertSizeToVisaType(uint size);
VISAChannelMask ConvertChannelMaskToVisaType(uint mask);
VISASourceSingleChannel ConvertSingleSourceChannel(uint srcChannel);

}
