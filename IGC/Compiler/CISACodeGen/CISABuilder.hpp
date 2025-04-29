/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/CVariable.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "visa_wa.h"
#include "inc/common/sku_wa.h"

namespace IGC
{
    class CShader;

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
        e_instance instance;
        bool specialRegion;
        void init()
        {
            mod = EMOD_NONE;
            subVar = 0;
            subReg = 0;
            instance = EINSTANCE_UNSPECIFIED;
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
        static unsigned getHashValue(const SAlias& Val) {
            return llvm::DenseMapInfo<CVariable*>::getHashValue(Val.m_rootVar) ^ Val.m_type;
        }
        static bool isEqual(const SAlias& LHS, const SAlias& RHS) {
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
        bool      m_secondNibble = false;
    };

    class CEncoder
    {
    public:
        void InitEncoder(bool canAbortOnSpill, bool hasStackCall, bool hasInlineAsmCall, bool hasAdditionalVisaAsmToLink, int numThreadsPerEU,
            uint lowerBoundGRF, uint upperBoundGRF, VISAKernel* prevKernel);
        void InitBuildParams(llvm::SmallVector<std::unique_ptr<const char, std::function<void(const char*)>>, 10> & params);
        void InitVISABuilderOptions(TARGET_PLATFORM VISAPlatform, bool canAbortOnSpill, bool hasStackCall, bool enableVISA_IR);
        SEncoderState CopyEncoderState();
        void SetEncoderState(SEncoderState& newState);
        VISA_Align GetVISAAlign(CVariable* var);

        void SetAbortOnSpillThreshold(bool canAbortOnSpill, bool AllowSpill);
        void SetMaxRegForThreadDispatch();
        void SetDispatchSimdSize();
        void SetSpillMemOffset();
        void SetStackFunctionArgSize(uint size);  // size in GRFs
        void SetStackFunctionRetSize(uint size);  // size in GRFs
        void SetExternFunctionFlag();

        void GetVISAPredefinedVar(CVariable* pVar, PreDefined_Vars var);
        void CreateVISAVar(CVariable* var);
        void DeclareInput(CVariable* var, uint offset, uint instance);
        void DeclarePred(CVariable* var, uint offset);
        void MarkAsOutput(CVariable* var);
        void MarkAsPayloadLiveOut(CVariable* var);
        void MarkAsExclusiveLoad(CVariable* var);
        void Compile(bool hasSymbolTable, GenXFunctionGroupAnalysis*& pFGA);
        std::string GetShaderName();
        const vISA::KernelCostInfo* kci;

        CEncoder();
        void SetProgram(CShader* program);
        void Jump(CVariable* flag, uint label);
        void AddLabel(uint label);
        void AddDivergentResourceLoopLabel(uint label);
        uint GetNewLabelID(const CName &name);
        void DwordAtomicRaw(AtomicOp atomic_op,
            const ResourceDescriptor& bindingTableIndex,
            CVariable* dst, CVariable* elem_offset, CVariable* src0,
            CVariable* src1, bool is16Bit = false);
        void AtomicRawA64(AtomicOp atomic_op, const ResourceDescriptor& resource, CVariable* dst,
            CVariable* elem_offset, CVariable* src0, CVariable* src1,
            unsigned short bitwidth);
        void TypedAtomic(
            AtomicOp atomic_op,
            CVariable* dst,
            const ResourceDescriptor& resource,
            CVariable* pU,
            CVariable* pV,
            CVariable* pR,
            CVariable* src0,
            CVariable* src1,
            CVariable* lod,
            bool is16Bit = false);
        void Cmp(e_predicate p, CVariable* dst, CVariable* src0, CVariable* src1);
        void Select(CVariable* flag, CVariable* dst, CVariable* src0, CVariable* src1);
        void GenericAlu(e_opcode opcode, CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2 = nullptr);
        void Send(CVariable* dst, CVariable* src, uint exDesc, CVariable* messDescriptor, bool isSendc = false);
        void Send(CVariable* dst, CVariable* src, uint ffid, CVariable* exDesc, CVariable* messDescriptor, bool isSendc = false);
        void Sends(CVariable* dst, CVariable* src0, CVariable* src1, uint ffid, CVariable* exDesc, CVariable* messDescriptor, bool isSendc = false, bool hasEOT = false);

        void RenderTargetWrite(CVariable* var[],
            bool isUndefined[],
            bool lastRenderTarget,
            bool isNullRT,
            bool perSample,
            bool coarseMode,
            bool headerMaskFromCe0,
            CVariable* bindingTableIndex,
            CVariable* RTIndex,
            CVariable* source0Alpha,
            CVariable* oMask,
            CVariable* depth,
            CVariable* stencil,
            CVariable* CPSCounter,
            CVariable* sampleIndex,
            CVariable* r1Reg);
        void Sample(
            EOPCODE subOpcode,
            uint writeMask,
            CVariable* offset,
            const ResourceDescriptor& bindingTableIndex,
            const ResourceDescriptor& pairedesource,
            const SamplerDescriptor& SamplerIdx,
            uint numSources,
            CVariable* dst,
            llvm::SmallVector<CVariable*, 4> & payload,
            bool zeroLOD,
            bool cpsEnable,
            bool feedbackEnable,
            bool nonUniformState = false);
        void Load(
            EOPCODE subOpcode,
            uint writeMask,
            CVariable* offset,
            const ResourceDescriptor& resource,
            const ResourceDescriptor& pairedesource,
            uint numSources,
            CVariable* dst,
            llvm::SmallVector<CVariable*, 4> & payload,
            bool zeroLOD,
            bool feedbackEnable);

        void Info(EOPCODE subOpcode, uint writeMask, const ResourceDescriptor& resource, CVariable* lod, CVariable* dst);

        void Gather4Inst(
            EOPCODE subOpcode,
            CVariable* offset,
            const ResourceDescriptor& resource,
            const ResourceDescriptor& pairedesource,
            const SamplerDescriptor& sampler,
            uint numSources,
            CVariable* dst,
            llvm::SmallVector<CVariable*, 4> & payload,
            uint channel,
            bool feedbackEnable);

        void OWLoad(CVariable* dst, const ResourceDescriptor& resource, CVariable* offset, bool owordAligned, uint bytesToBeRead, uint dstOffset = 0);
        void OWStore(CVariable* data, e_predefSurface surfaceType, CVariable* bufidx, CVariable* offset, uint bytesToBeRead, uint srcOffset);

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
        void EOT();
        void OWLoadA64(CVariable* dst, CVariable* offset, uint dstSize, uint dstOffset = 0);
        void OWStoreA64(CVariable* dst, CVariable* offset, uint dstSize, uint srcOffset);
        void MediaBlockMessage(ISA_Opcode subOpcode,
            CVariable* dst,
            e_predefSurface surfaceType,
            CVariable* buf,
            CVariable* xOffset,
            CVariable* yOffset,
            uint modifier,
            unsigned char blockWidth,
            unsigned char blockHeight,
            uint plane);
        void GatherA64(CVariable* dst, CVariable* offset, unsigned elementSize, unsigned numElems);
        VISA_VectorOpnd* GetVISALSCSurfaceOpnd(e_predefSurface surfaceType, CVariable* bti);
        static LSC_DATA_SIZE LSC_GetElementSize(unsigned eSize, bool is2DBlockMsg = false);
        static LSC_DATA_ELEMS LSC_GetElementNum(unsigned eNum);
        static LSC_ADDR_TYPE getLSCAddrType(const ResourceDescriptor * resource);
        static LSC_ADDR_TYPE getLSCAddrType(e_predefSurface surfaceType);
        void LSC_LoadGather(LSC_OP subOp, CVariable *dst,
                            CVariable *offset, LSC_DATA_SIZE elemSize,
                            LSC_DATA_ELEMS numElems, unsigned blockOffset,
                            ResourceDescriptor *resource,
                            LSC_ADDR_SIZE addr_size, LSC_DATA_ORDER data_order,
                            int immOffset, int immScale, LSC_CACHE_OPTS cacheOpts,
                            LSC_DOC_ADDR_SPACE addrSpace);
        void LSC_StoreScatter(LSC_OP subOp,
                              CVariable *src, CVariable *offset,
                              LSC_DATA_SIZE elemSize, LSC_DATA_ELEMS numElems,
                              unsigned blockOffset,
                              ResourceDescriptor *resource,
                              LSC_ADDR_SIZE addr_size,
                              LSC_DATA_ORDER data_order,
                              int immOffset, int immScale,
                              LSC_CACHE_OPTS cacheOpts,
                              LSC_DOC_ADDR_SPACE addrSpace);
        void LSC_LoadBlock1D(
            CVariable* dst, CVariable* offset,
            LSC_DATA_SIZE elemSize, LSC_DATA_ELEMS numElems,
            ResourceDescriptor* resource,
            LSC_ADDR_SIZE addrSize, int addrImmOffset,
            LSC_CACHE_OPTS cacheOpts);
        void LSC_StoreBlock1D(
            CVariable * src, CVariable * offset,
            LSC_DATA_SIZE elemSize, LSC_DATA_ELEMS numElems,
            ResourceDescriptor * resource,
            LSC_ADDR_SIZE addrSize, int addrImmOffset,
            LSC_CACHE_OPTS cacheOpts);
        void LSC_AtomicRaw(AtomicOp atomic_op, CVariable *dst,
                           CVariable *offset, CVariable *src0, CVariable *src1,
                           unsigned short bitwidth,
                           ResourceDescriptor *resource,
                           LSC_ADDR_SIZE addr_size, int immOff, int immScale,
                           LSC_CACHE_OPTS cacheOpts);
        void LSC_Fence(LSC_SFID sfid, LSC_SCOPE scope, LSC_FENCE_OP op);
        void LSC_2DBlockMessage(
            LSC_OP subOp, ResourceDescriptor* resource,
            CVariable* dst, CVariable* bufId,
            CVariable* xOffset, CVariable* yOffset,
            unsigned blockWidth,
            unsigned blockHeight,
            unsigned elemSize, unsigned numBlocks,
            bool isTranspose, bool isVnni,
            CVariable* flatImageBaseoffset, CVariable* flatImageWidth,
            CVariable* flatImageHeight, CVariable* flatImagePitch,
            LSC_CACHE_OPTS cacheOpts = { LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT });
        void LSC_2DBlockMessage(
            LSC_OP subOp,
            CVariable* Dst,
            CVariable* AddrPayload,
            CVariable* Src,
            uint32_t ImmX, uint32_t ImmY,
            uint32_t elemSize,
            uint32_t blockWidth, uint32_t blockHeight, uint32_t numBlocks,
            bool isTranspose, bool isVnni,
            LSC_CACHE_OPTS cacheOpts = { LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT });
        void NamedBarrier(e_barrierKind BarrierKind, CVariable* src0, CVariable* src1, CVariable* src2, CVariable* src3);
        void LSC_TypedReadWrite(
            LSC_OP subOp, ResourceDescriptor* resource,
            CVariable* pU, CVariable* pV, CVariable* pR,
            CVariable* pLODorSampleIdx,
            CVariable* pSrcDst,
            unsigned elemSize, unsigned numElems,
            LSC_ADDR_SIZE addr_size, int chMask,
            LSC_CACHE_OPTS cacheOpts = { LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT });


        void LSC_TypedAtomic(
            AtomicOp subOp, ResourceDescriptor* resource,
            CVariable* pU, CVariable* pV, CVariable* pR,
            CVariable* pSrc0, CVariable* pSrc1, CVariable* pSrcDst,
            unsigned elemSize, LSC_ADDR_SIZE addr_size, LSC_CACHE_OPTS cacheOpts);


        void LSC_Typed2dBlock(
            LSC_OP subOpcode,
            CVariable* dst,
            e_predefSurface surfaceType,
            CVariable* buf,
            CVariable* xOffset,
            CVariable* yOffset,
            int blockWidth,
            int blockHeight);
        void LSC_UntypedAppendCounterAtomic(
            LSC_OP lscOp,
            ResourceDescriptor* resource,
            CVariable* dst,
            CVariable* src0);
        void AppendBreakpoint();
        void ScatterA64(CVariable* val, CVariable* offset, unsigned elementSize, unsigned numElems);
        void ByteGather(CVariable* dst, const ResourceDescriptor& resource, CVariable* offset, unsigned elementSize, unsigned numElems);
        void ByteScatter(CVariable* src, const ResourceDescriptor& resource, CVariable* offset, unsigned elementSize, unsigned numElems);
        void Gather4Scaled(CVariable* dst, const ResourceDescriptor& resource, CVariable* offset, unsigned Mask = 0);
        void Gather4ScaledNd(CVariable* dst, const ResourceDescriptor& resource, CVariable* offset, unsigned nd, unsigned Mask = 0);
        void Scatter4Scaled(CVariable* src, const ResourceDescriptor& resource, CVariable* offset);
        void Gather4A64(CVariable* dst, CVariable* offset);
        void Scatter4A64(CVariable* src, CVariable* offset);
        void BoolToInt(CVariable* dst, CVariable* src);
        void Copy(CVariable* dst, CVariable* src);
        void SubroutineCall(CVariable* flag, llvm::Function* F);
        void SubroutineRet(CVariable* flag, llvm::Function* F);
        void StackCall(CVariable* flag, llvm::Function* F, unsigned char argSize, unsigned char retSize);
        void IndirectStackCall(CVariable* flag, CVariable* funcPtr, unsigned char argSize, unsigned char retSize);
        void StackRet(CVariable* flag);
        void Loc(unsigned int line);
        void File(std::string& s);
        void PredAdd(CVariable* flag, CVariable* dst, CVariable* src0, CVariable* src1);
        void DebugLinePlaceholder();

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
        // src0 * src1 + src2
        inline void Madw(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
        inline void Mad(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
        inline void Lrp(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
        inline void Xor(CVariable* dst, CVariable* src0, CVariable* src1);
        inline void Or(CVariable* dst, CVariable* src0, CVariable* src1);
        inline void And(CVariable* dst, CVariable* src0, CVariable* src1);
        inline void Pln(CVariable* dst, CVariable* src0, CVariable* src1);
        inline void SendC(CVariable* dst, CVariable* src, uint exDesc, CVariable* messDescriptor);
        inline void SendC(CVariable* dst, CVariable* src, uint ffid, CVariable* exDesc, CVariable* messDescriptor);
        inline void LoadMS(EOPCODE subOpcode, uint writeMask, CVariable* offset, const ResourceDescriptor& resource, uint numSources, CVariable* dst, llvm::SmallVector<CVariable*, 4> & payload, bool feedbackEnable);
        inline void SetP(CVariable* dst, CVariable* src);
        inline void Gather(CVariable* dst, CVariable* bufidx, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize);
        inline void TypedRead4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV, CVariable* pR, CVariable* pLOD, CVariable* pDst, uint writeMask);
        inline void TypedWrite4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV, CVariable* pR, CVariable* pLOD, CVariable* pSrc, uint writeMask);
        inline void Scatter(CVariable* val, CVariable* bufidx, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize);
        inline void IShr(CVariable* dst, CVariable* src0, CVariable* src1);
        inline void Min(CVariable* dst, CVariable* src0, CVariable* src1);
        inline void Max(CVariable* dst, CVariable* src0, CVariable* src1);
        inline void UAddC(CVariable* dst, CVariable* dstCarryBorrow, CVariable* src0, CVariable* src1);
        inline void USubB(CVariable* dst, CVariable* dstCarryBorrow, CVariable* src0, CVariable* src1);
        inline void IEEESqrt(CVariable* dst, CVariable* src0);
        inline void IEEEDivide(CVariable* dst, CVariable* src0, CVariable* src1);
        void AddPair(CVariable* Lo, CVariable* Hi, CVariable* L0, CVariable* H0, CVariable* L1, CVariable* H1 = nullptr);
        void SubPair(CVariable* Lo, CVariable* Hi, CVariable* L0, CVariable* H0, CVariable* L1, CVariable* H1);
        inline void dp4a(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
        void Lifetime(VISAVarLifetime StartOrEnd, CVariable* dst);
        void dpas(CVariable* dst, CVariable* input, CVariable* weight, PrecisionType weight_precision,
            CVariable* actication, PrecisionType activation_precision, uint8_t systolicDepth,
            uint8_t repeatCount, bool IsDpasw);
        void fcvt(CVariable* dst, CVariable* src);
        void srnd(CVariable* D, CVariable* S0, CVariable* R);
        void Bfn(uint8_t booleanFuncCtrl, CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2);
        void QWGather(CVariable* dst, const ResourceDescriptor& resource, CVariable* offset, unsigned elementSize, unsigned numElems);
        void QWScatter(CVariable* src, const ResourceDescriptor& resource, CVariable* offset, unsigned elementSize, unsigned numElems);
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
            CVariable* outputVar);

        void SendVmeFbr(
            CVariable* bindingTableIndex,
            CVariable* uniInputVar,
            CVariable* fbrInputVar,
            CVariable* FBRMbModeVar,
            CVariable* FBRSubMbShapeVar,
            CVariable* FBRSubPredModeVar,
            CVariable* outputVar);

        void SendVmeSic(
            CVariable* bindingTableIndex,
            CVariable* uniInputVar,
            CVariable* sicInputVar,
            CVariable* outputVar);

        // VA
        void SendVideoAnalytic(
            llvm::GenIntrinsicInst* inst,
            CVariable* vaResult,
            CVariable* coords,
            CVariable* size,
            CVariable* srcImg,
            CVariable* sampler);

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
        void SetSrcRegion(uint srcNum, uint vStride, uint width, uint hStride, e_instance instance = EINSTANCE_UNSPECIFIED);
        void SetDstRegion(uint hStride);
        inline void SetNoMask();
        inline void SetMask(e_mask mask);
        inline void SetSimdSize(SIMDMode size);
        inline SIMDMode GetSimdSize();
        inline void SetUniformSIMDSize(SIMDMode size);
        inline void SetSubSpanDestination(bool subspan);
        inline bool IsSubSpanDestination();
        inline void SetSecondHalf(bool secondHalf);
        inline bool IsSecondHalf();
        inline void SetSecondNibble(bool secondNibble);
        inline bool IsSecondNibble();

        inline void SetIsCodePatchCandidate(bool v);
        inline bool IsCodePatchCandidate();
        inline unsigned int GetPayloadEnd();
        inline void SetPayloadEnd(unsigned int payloadEnd);
        inline void SetHasPrevKernel(bool v);
        inline bool HasPrevKernel();
        inline void BeginForcedNoMaskRegion();
        inline void EndForcedNoMaskRegion();
        inline bool GetUniqueExclusiveLoad();
        inline void SetUniqueExcusiveLoad(bool v);

        void Wait();

        VISAKernel* GetVISAKernel() const { return vKernel; }
        VISABuilder* GetVISABuilder() const { return vbuilder; }
        void Init();
        void Push();

        void initCR(VISAKernel* vKernel);
        void SetVectorMask(bool vMask);

        // Switches from actualRM to newRM
        void SetRoundingMode_FP(ERoundingMode actualRM, ERoundingMode newRM);
        void SetRoundingMode_FPCvtInt(ERoundingMode actualRM, ERoundingMode newRM);

        static uint GetCISADataTypeSize(VISA_Type type) {return CVariable::GetCISADataTypeSize(type);}
        static e_alignment GetCISADataTypeAlignment(VISA_Type type) {return CVariable::GetCISADataTypeAlignment(type);}

        static VISASampler3DSubOpCode ConvertSubOpcode(EOPCODE subOpcode, bool zeroLOD);

        // Wrappers for (potentially) common queries on types
        static bool IsIntegerType(VISA_Type type);
        static bool IsFloatType(VISA_Type type);

        void SetVISAWaTable(WA_TABLE const& waTable);

        /// \brief Initialize per function states and starts vISA emission
        /// as a vISA subroutine
        void BeginSubroutine(llvm::Function* F);
        /// \brief Initialize per function states and starts vISA emission
        /// as a vISA stack-call function
        void BeginStackFunction(llvm::Function* F);
        /// \brief Initialize interpolation section for vISA emission
        void BeginPayloadSection();

        void DestroyVISABuilder();

        void AddVISASymbol(std::string& symName, CVariable* cvar);

        std::string GetVariableName(CVariable* var);
        std::string GetDumpFileName(std::string extension);

        bool IsPayloadSectionAsPrimary()    {return vKernel == vPayloadSection;}
        void SetPayloadSectionAsPrimary()   {vKernelTmp = vKernel; vKernel = vPayloadSection;}
        void SetPayloadSectionAsSecondary() {vKernel = vKernelTmp;}

        std::string GetUniqueInlineAsmLabel();

        bool IsVisaCompiledSuccessfully() const { return m_vIsaCompileStatus == VISA_SUCCESS; }
        bool IsVisaCompileStatusFailure() const { return m_vIsaCompileStatus == VISA_FAILURE; }

    private:
        // helper functions
        VISA_VectorOpnd* GetSourceOperand(CVariable* var, const SModifier& mod);
        VISA_VectorOpnd* GetSourceOperandNoModifier(CVariable* var);
        VISA_VectorOpnd* GetDestinationOperand(CVariable* var, const SModifier& mod);
        VISA_RawOpnd* GetRawSource(CVariable* var, uint offset = 0);
        VISA_RawOpnd* GetPairedResourceOperand(const ResourceDescriptor& pairedResource);
        VISA_RawOpnd* GetRawDestination(CVariable* var, unsigned offset = 0);
        VISA_PredOpnd* GetFlagOperand(const SFlag& flag);
        VISA_StateOpndHandle* GetVISASurfaceOpnd(e_predefSurface surfaceType, CVariable* var);
        VISA_StateOpndHandle* GetVISASurfaceOpnd(const ResourceDescriptor& resource);
        VISA_LabelOpnd* GetOrCreateLabel(uint label, VISA_Label_Kind kind = LABEL_BLOCK);
        VISA_LabelOpnd* GetFuncLabel(llvm::Function* F);
        void InitLabelMap(const llvm::Function* F);
        CName CreateVisaLabelName(const llvm::StringRef &L = "");
        std::string CreateShortLabel(unsigned labelIndex) const;
        // Compiler labels must start with something a user won't use in inline
        // assembly.
        static const char *GetCompilerLabelPrefix() {return "_";}

        VISAFunction* GetStackFunction(llvm::Function* F);

        VISA_VectorOpnd* GetUniformSource(CVariable* var);
        VISA_StateOpndHandle* GetBTIOperand(uint bindingTableIndex);
        VISA_StateOpndHandle* GetSamplerOperand(CVariable* sampleIdx);
        VISA_StateOpndHandle* GetSamplerOperand(const SamplerDescriptor& sampler);
        void GetRowAndColOffset(CVariable* var, unsigned int subVar, unsigned int subreg, unsigned char& rowOff, unsigned char& colOff);

        VISA_GenVar* GetVISAVariable(CVariable* var);
        VISA_GenVar* GetVISAVariable(CVariable* var, e_instance instance);
        VISA_EMask_Ctrl ConvertMaskToVisaType(e_mask mask, bool noMask);

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
        // dst may be special `null` CVariable, if only carry/borrow bits matter
        void CarryBorrowArith(ISA_Opcode opcode, CVariable* dst, CVariable* dstCarryBorrow,
            CVariable* src0, CVariable* src1);
        void ScatterGather(
            ISA_Opcode opcode,
            CVariable* srcdst,
            CVariable* bufId,
            CVariable* offset,
            CVariable* gOffset,
            e_predefSurface surface,
            int elementSize);
        void TypedReadWrite(
            ISA_Opcode opcode,
            const ResourceDescriptor& resource,
            CVariable* pU,
            CVariable* pV,
            CVariable* pR,
            CVariable* pLOD,
            CVariable* pSrcDst,
            uint writeMask);

        VISA_Exec_Size  GetAluExecSize(CVariable* dst) const;
        VISA_EMask_Ctrl GetAluEMask(CVariable* dst);
        bool IsSat();
        bool isSamplerIdxLT16(const SamplerDescriptor& sampler);

        // Variable splitting facilities (if crosses 2 GRF boundary).
        bool NeedSplitting(CVariable* var, const SModifier& mod,
            unsigned& numParts, bool isSource = false) const;
        SModifier SplitVariable(VISA_Exec_Size fromExecSize,
            VISA_Exec_Size toExecSize,
            unsigned thePart,
            CVariable* var, const SModifier& mod,
            bool isSource = false) const;
        VISA_Exec_Size SplitExecSize(VISA_Exec_Size fromExecSize,
            unsigned numParts) const;
        VISA_EMask_Ctrl SplitEMask(VISA_Exec_Size fromExecSize,
            VISA_Exec_Size toExecSize,
            unsigned thePart,
            VISA_EMask_Ctrl execMask) const;

        // Split SIMD16 message data payload(MDP) for scattered/untyped write
        // messages into two SIMD8 MDPs : V0 and V1.
        void SplitPayloadToLowerSIMD(CVariable* MDP, uint32_t MDPOfst, uint32_t NumBlks, CVariable* V0, CVariable* V1, uint32_t fromSize = 16);
        // Merge two SIMD8 MDPs (V0 & V1) for scattered/untyped read messages into one SIMD16 message : MDP
        void MergePayloadToHigherSIMD(CVariable* V0, CVariable* V1, uint32_t NumBlks, CVariable* MDP, uint32_t MDPOfst, uint32_t toSize = 16);

        // save compile time by avoiding retry if the amount of spill is (very) small
        bool AvoidRetryOnSmallSpill() const;

        // CreateSymbolTable, CreateRelocationTable and CreateFuncAttributeTable will create symbols, relococations and FuncAttributes in
        // two formats. One in given buffer that will be later parsed as patch token based format, another as struct type that will be parsed
        // as ZE binary format

        // CreateSymbolTable
        // Note that this function should be called only once even if there are multiple kernels in a program. Current IGC
        // flow will create all symbols in the first kernel and all the other kernels won't contain symbols
        typedef std::vector<std::pair<llvm::Value*, vISA::GenSymEntry>> ValueToSymbolList;
        void CreateSymbolTable(ValueToSymbolList& symbolTableList);
        // input/output: buffer, bufferSize, tableEntries: for patch-token-based format.
        void CreateSymbolTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries);
        // input/output: symbols: for ZEBinary foramt
        void CreateSymbolTable(SProgramOutput::ZEBinFuncSymbolTable& funcSyms, SOpenCLProgramInfo::ZEBinProgramSymbolTable& programSyms);
        // Create local symbols for kernels. This is ZEBinary format only.
        void CreateLocalSymbol(const std::string& kernelName, vISA::GenSymType type,
            unsigned offset, unsigned size, SProgramOutput::ZEBinFuncSymbolTable& symbols);

        // CreateRelocationTable
        // input/output: buffer, bufferSize, tableEntries: for patch-token-based format.
        void CreateRelocationTable(VISAKernel* pMainKernel, void*& buffer, unsigned& bufferSize, unsigned& tableEntries);
        // input/output: relocations: for ZEBinary foramt
        void CreateRelocationTable(VISAKernel* pMainKernel, SProgramOutput::RelocListTy& relocations);

        // CreateFuncAttributeTable
        void CreateFuncAttributeTable(VISAKernel *pMainKernel,
                                      GenXFunctionGroupAnalysis *pFga);

        // CreateGlobalHostAccessTable
        typedef std::vector<vISA::HostAccessEntry> HostAccessList;
        void CreateGlobalHostAccessTable(void*& buffer, unsigned& bufferSize, unsigned& tableEntries);
        // input/output: hostAccessMap: for patch-token-based format.
        void CreateGlobalHostAccessTable(HostAccessList& hostAccessMap);
        // input/output: global host access names: for ZEBinary format
        void CreateGlobalHostAccessTable(SOpenCLProgramInfo::ZEBinGlobalHostAccessTable& globalHostAccessTable);

        uint32_t getGRFSize() const;

        bool needsSplitting(VISA_Exec_Size ExecSize) const
        {
            if (getGRFSize() == 64)
            {
                return ExecSize == EXEC_SIZE_32;
            }
            return ExecSize == EXEC_SIZE_16;
        }

        // Note that GEN can set both fpCvtInt_rtz and any of FP rounding modes
        // at the same time. If fpCvtInt uses a rounding mode other than rtz,
        // they both uses FP rounding bits.
        //
        // RM bits in CR0.0.
        //    float RM bits: [5:4];
        //    int RM (float -> int): Bit 12: 0 -> rtz; 1 -> using Float RM
        enum RMEncoding {
            // float rounding mode (fp operations, cvt to fp)
            RoundToNearestEven = 0x00,
            RoundToPositive = 0x10,
            RoundToNegative = 0x20,
            RoundToZero = 0x30,
            // int rounding mode (fp cvt int only), use FP RM for all rounding modes but rtz.
            RoundToNearestEven_int = 0x1000,
            RoundToPositive_int = 0x1010,
            RoundToNegative_int = 0x1020,
            RoundToZero_int_unused = 0x1030,
            RoundToZero_int = 0x0000,    // use this for rtz, bit 12 = 0

            IntAndFPRoundingModeMask = 0x1030
        };
        void SetRoundingMode(RMEncoding actualRM, RMEncoding newRM);
        // Get Encoding bit values for rounding mode
        RMEncoding getEncoderRoundingMode_FP(ERoundingMode FP_RM);
        RMEncoding getEncoderRoundingMode_FPCvtInt(ERoundingMode FCvtI_RM);
        unsigned GetRawOpndSplitOffset(VISA_Exec_Size fromExecSize,
            VISA_Exec_Size toExecSize,
            unsigned thePart, CVariable* var) const;

        std::tuple<CVariable*, uint32_t> splitRawOperand(CVariable* var, bool isFirstHalf, VISA_EMask_Ctrl execMask);

        uint32_t getNumChannels(CVariable* var) const;

        void SaveOption(vISAOptions option, bool val);
        void SaveOption(vISAOptions option, uint32_t val);
        void SaveOption(vISAOptions option, const char* val);
        void SetBuilderOptions(VISABuilder* pbuilder);

        unsigned int GetSpillThreshold(SIMDMode simdmode);
    private:
        // helper functions for compile flow
        /// shaderOverrideVISAFirstPass - pre-process shader overide dir for visa shader override.
        /// return if there are visa files to be overriden
        bool shaderOverrideVISAFirstPass(
            std::vector<std::string> &visaOverrideFiles, std::string& kernelName);

        /// shaderOverrideVISASecondPassOrInlineAsm - handle visa inputs of shader override
        /// or inline asm
        /// Return the VISAKernel built from overrided visa or inline asm. Return nullptr if
        /// failed to create VISAKernel
        VISAKernel* shaderOverrideVISASecondPassOrInlineAsm(
            bool visaAsmOverride, bool hasSymbolTable, bool emitVisaOnly,
            const std::vector<const char *> *additionalVISAAsmToLink,
            const std::vector<std::string> &visaOverrideFiles,
            const std::string kernelName);

        // setup m_retryManager according to jitinfo and other factors
        void SetKernelRetryState(CodeGenContext* context, vISA::FINALIZER_INFO* jitInfo, GenXFunctionGroupAnalysis*& pFGA);

        /// create symbol tables and GlobalHostAccessTable according to if it's zebin
        /// or patch-token formats
        void createSymbolAndGlobalHostAccessTables(bool hasSymbolTable,
                                                   VISAKernel &pMainKernel,
                                                   unsigned int scratchOffset);

        /// create relocation according to if it's zebin or patch-token formats
        void createRelocationTables(VISAKernel &pMainKernel);

        /// Estimate vISA stack size according to FunctionGroup information.
        /// Conservatively increase the stack size when having recursive or indirect
        /// calls if the given function is a Function Group Head. Function group
        /// heads are kernels/entry functions those their stack size will be used to
        /// reported back to the Runtime, so we set a conservative value. For other
        /// non-head functions, we set the precise stack usage of the function.
        ///
        /// For group head functions, when having the stack call, the spillSize
        /// returned by vISA is the sum of required stack size of all potential
        /// callee and without considering recursive or indirect calls.
        uint32_t getSpillMemSizeWithFG(const llvm::Function &curFunc,
                                       uint32_t curSize,
                                       GenXFunctionGroupAnalysis *fga,
                                       uint32_t gtpinScratchUse);

    const vISA::KernelCostInfo* createKernelCostInfo(VISAKernel &pMainKenrel);

    protected:
        // encoder states
        SEncoderState m_encoderState = {};

        llvm::DenseMap<SAlias, CVariable*, SAliasMapInfo> m_aliasesMap;

        // vISA needs its own Wa-table as some of the W/A are applicable
        // only to certain APIs/shader types/reg key settings/etc.
        WA_TABLE m_vISAWaTable = {};

        enum OpType
        {
            ET_BOOL,
            ET_INT32,
            ET_CSTR
        };
        struct OptionValue
        {
            OpType type;
            bool vBool;
            uint32_t vInt32;
            const char* vCstr;
        };
        // List of vISA user options
        std::vector<std::pair<vISAOptions, OptionValue>> m_visaUserOptions;

        // Typically IGC just use ones vKernel for every vISA::compile call,
        // in those cases, vKernel and vMainKernel should be the same.
        // Only when using stack-call, vKernel pointer changes every time
        // IGC addes a vISA kernel or function object, but the vMainKernel
        // always pointing to the first kernel added during InitEncoder.
        VISAKernel* vKernel;
        VISAKernel* vMainKernel;
        VISABuilder* vbuilder;
        VISABuilder* vAsmTextBuilder;

        // This is for CodePatch to split payload interpolation from a shader
        VISAKernel* vPayloadSection;
        VISAKernel* vKernelTmp;
        bool m_hasPrevKernel = false;
        unsigned int m_payloadEnd = 0;
        unsigned int m_argumentStackSize = 0;

        bool m_isCodePatchCandidate = false;

        bool m_hasUniqueExclusiveLoad = false;

        int m_nestLevelForcedNoMaskRegion = 0;

        bool m_enableVISAdump = false;
        bool m_hasInlineAsm = false;

        std::vector<VISA_LabelOpnd*> labelMap;
        std::vector<CName> labelNameMap; // parallel to labelMap

        /// Per kernel label counter
        unsigned labelCounter = 0;
        /// Per kernel label counter for each inline asm block
        unsigned labelInlineAsmCounter = 0;
        /// Each kernel might emit several functions;
        /// we pre-increment this for each new function we process (InitLabelMap)
        /// The first function will see 0, ...
        unsigned labelFunctionIndex = (unsigned)-1;
        ///
        /// The name of the current function; set if we are emitting labels
        CName currFunctionName;

        /// Keep a map between a function and its label, per kernel state.
        llvm::MapVector<llvm::Function*, VISA_LabelOpnd*> funcLabelMap;
        /// Keep a map between a stack-called function and the corresponding vISA function
        llvm::MapVector<llvm::Function*, VISAFunction*> stackFuncMap;

        // dummy variables
        VISA_SurfaceVar* dummySurface;
        VISA_SamplerVar* samplervar;

        CShader* m_program;
        int m_vIsaCompileStatus = VISA_FAILURE;

        // Keep a map between a function and its per-function attributes needed for function pointer support
        struct FuncAttrib
        {
            bool isKernel = false;
            bool hasBarrier = false;
            unsigned argumentStackSize = 0;
            unsigned allocaStackSize = 0;
        };
        llvm::MapVector<llvm::Function*, FuncAttrib> funcAttributeMap;

    public:
        // Used by EmitVISAPass to set function attributes
        void InitFuncAttribute(llvm::Function* F, bool isKernel = false) {
            funcAttributeMap[F].isKernel = isKernel;
        }
        void SetFunctionHasBarrier(llvm::Function* F) {
            if (funcAttributeMap.find(F) != funcAttributeMap.end())
                funcAttributeMap[F].hasBarrier = true;
        }
        void SetFunctionMaxArgumentStackSize(llvm::Function* F, unsigned size) {
            if (funcAttributeMap.find(F) != funcAttributeMap.end())
                m_argumentStackSize = funcAttributeMap[F].argumentStackSize = MAX(funcAttributeMap[F].argumentStackSize, size);

        }
        void SetFunctionAllocaStackSize(llvm::Function* F, unsigned size) {
            if (funcAttributeMap.find(F) != funcAttributeMap.end())
                funcAttributeMap[F].allocaStackSize = size;
        }
    };

    inline void CEncoder::Jump(uint label)
    {
        Jump(NULL, label);
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

    inline void CEncoder::MulH(CVariable* dst, CVariable* src0, CVariable* src1)
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

    inline void CEncoder::Mod(CVariable* dst, CVariable* src0, CVariable* src1)
    {
        Arithmetic(ISA_MOD, dst, src0, src1);
    }

    inline void CEncoder::Rsqrt(CVariable* dst, CVariable* src0)
    {
        Arithmetic(ISA_RSQRT, dst, src0);
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

    // src0 * src1 + src2
    inline void CEncoder::Madw(CVariable* dst, CVariable* src0, CVariable* src1, CVariable* src2)
    {
        Arithmetic(ISA_MADW, dst, src0, src1, src2);
    }

    // src0 * src1 + src2
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

    inline void CEncoder::UAddC(CVariable* dst, CVariable* dstCarryBorrow, CVariable* src0, CVariable* src1)
    {
        CarryBorrowArith(ISA_ADDC, dst, dstCarryBorrow, src0, src1);
    }

    inline void CEncoder::USubB(CVariable* dst, CVariable* dstCarryBorrow, CVariable* src0, CVariable* src1)
    {
        CarryBorrowArith(ISA_SUBB, dst, dstCarryBorrow, src0, src1);
    }

    inline void CEncoder::LoadMS(EOPCODE subOpcode, uint writeMask, CVariable* offset,
        const ResourceDescriptor& resource, uint numSources, CVariable* dst,
        llvm::SmallVector<CVariable*, 4> & payload, bool feedbackEnable)
    {
        ResourceDescriptor pairedResource{};
        Load(
            subOpcode,
            writeMask,
            offset,
            resource,
            pairedResource,
            numSources,
            dst,
            payload,
            false,
            feedbackEnable);
    }

    inline void CEncoder::Gather(CVariable* dst, CVariable* bufId, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize)
    {
        ScatterGather(ISA_GATHER, dst, bufId, offset, gOffset, surface, elementSize);
    }

    inline void CEncoder::TypedRead4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV,
        CVariable* pR, CVariable* pLOD, CVariable* pDst, uint writeMask)
    {
        TypedReadWrite(ISA_GATHER4_TYPED, resource, pU, pV, pR, pLOD, pDst, writeMask);
    }

    inline void CEncoder::TypedWrite4(const ResourceDescriptor& resource, CVariable* pU, CVariable* pV,
        CVariable* pR, CVariable* pLOD, CVariable* pSrc, uint writeMask)
    {
        TypedReadWrite(ISA_SCATTER4_TYPED, resource, pU, pV, pR, pLOD, pSrc, writeMask);
    }

    inline void CEncoder::Scatter(CVariable* val, CVariable* bufidx, CVariable* offset, CVariable* gOffset, e_predefSurface surface, int elementSize)
    {
        ScatterGather(ISA_SCATTER, val, bufidx, offset, gOffset, surface, elementSize);
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

    inline void CEncoder::SetIsCodePatchCandidate(bool v)
    {
        m_isCodePatchCandidate = v;
    }

    inline bool CEncoder::IsCodePatchCandidate()
    {
        return m_isCodePatchCandidate;
    }

    inline void CEncoder::SetPayloadEnd(unsigned int payloadEnd)
    {
        m_payloadEnd = payloadEnd;
    }

    inline unsigned int CEncoder::GetPayloadEnd()
    {
        return m_payloadEnd;
    }

    inline void CEncoder::SetHasPrevKernel(bool v)
    {
        m_hasPrevKernel = v;
    }

    inline bool CEncoder::HasPrevKernel()
    {
        return m_hasPrevKernel;
    }

    inline bool CEncoder::GetUniqueExclusiveLoad()
    {
        return m_hasUniqueExclusiveLoad;
    }

    inline void CEncoder::SetUniqueExcusiveLoad(bool v)
    {
        m_hasUniqueExclusiveLoad = v;
    }

    inline void CEncoder::BeginForcedNoMaskRegion()
    {
        ++m_nestLevelForcedNoMaskRegion;
        // Start submitting insts with NoMask control
        m_encoderState.m_noMask = true;
    }

    inline void CEncoder::EndForcedNoMaskRegion()
    {
        --m_nestLevelForcedNoMaskRegion;
        IGC_ASSERT_MESSAGE(m_nestLevelForcedNoMaskRegion >= 0, "Invalid nesting of Unmasked regions");
        // Out of unmasked region, return to submitting insts
        // with Mask control
        if (m_nestLevelForcedNoMaskRegion == 0)
            m_encoderState.m_noMask = false;
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

    inline SIMDMode CEncoder::GetSimdSize()
    {
        return m_encoderState.m_simdSize;
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

    inline void CEncoder::SetSecondNibble(bool secondNibble)
    {
        m_encoderState.m_secondNibble = secondNibble;
    }

    inline bool CEncoder::IsSecondNibble()
    {
        return m_encoderState.m_secondNibble;
    }

    inline bool CEncoder::IsSubSpanDestination()
    {
        return m_encoderState.m_SubSpanDestination;
    }

    VISA_Modifier ConvertModifierToVisaType(e_modifier modifier);
    VISA_Cond_Mod ConvertCondModToVisaType(e_predicate condMod);
    VISA_Oword_Num  ConvertSizeToVisaType(uint size);
    VISAChannelMask ConvertChannelMaskToVisaType(uint mask);
    VISASourceSingleChannel ConvertSingleSourceChannel(uint srcChannel);


    GenPrecision ConvertPrecisionToVisaType(PrecisionType P);
}
