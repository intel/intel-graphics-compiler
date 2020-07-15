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
#include "BlockCoalescing.hpp"
#include "PatternMatchPass.hpp"
#include "ShaderCodeGen.hpp"
#include "CoalescingEngine.hpp"
#include "Simd32Profitability.hpp"
#include "GenCodeGenModule.h"
#include "VariableReuseAnalysis.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InlineAsm.h>
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

namespace llvm
{
    class GenIntrinsicInst;
}

namespace IGC
{
// Forward declaration
class IDebugEmitter;
struct PSSignature;

class EmitPass : public llvm::FunctionPass
{
public:
    EmitPass(CShaderProgram::KernelShaderMap& shaders, SIMDMode mode, bool canAbortOnSpill, ShaderDispatchMode shaderMode, PSSignature* pSignature = nullptr);

    virtual ~EmitPass();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<llvm::DominatorTreeWrapperPass>();
        AU.addRequired<WIAnalysis>();
        AU.addRequired<LiveVarsAnalysis>();
        AU.addRequired<CodeGenPatternMatch>();
        AU.addRequired<DeSSA>();
        AU.addRequired<BlockCoalescing>();
        AU.addRequired<CoalescingEngine>();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<Simd32ProfitabilityAnalysis>();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<VariableReuseAnalysis>();
        AU.setPreservesAll();
    }

    virtual bool runOnFunction(llvm::Function& F) override;
    virtual llvm::StringRef getPassName() const  override { return "EmitPass"; }

    void CreateKernelShaderMap(CodeGenContext* ctx, IGC::IGCMD::MetaDataUtils* pMdUtils, llvm::Function& F);

    void Frc(const SSource& source, const DstModifier& modifier);
    void Mad(const SSource sources[3], const DstModifier& modifier);
    void Lrp(const SSource sources[3], const DstModifier& modifier);
    void Cmp(llvm::CmpInst::Predicate pred, const SSource sources[2], const DstModifier& modifier);
    void Sub(const SSource[2], const DstModifier& mofidier);
    void Xor(const SSource[2], const DstModifier& modifier);
    void FDiv(const SSource[2], const DstModifier& modifier);
    void Pow(const SSource sources[2], const DstModifier& modifier);
    void Avg(const SSource sources[2], const DstModifier& modifier);
    void Rsqrt(const SSource& source, const DstModifier& modifier);
    void Sqrt(const SSource& source, const DstModifier& modifier);
    void Select(const SSource sources[3], const DstModifier& modifier);
    void PredAdd(const SSource& pred, bool invert, const SSource sources[2], const DstModifier& modifier);
    void Mul(const SSource[2], const DstModifier& modifier);
    void Mov(const SSource& source, const DstModifier& modifier);
    void Unary(e_opcode opCode, const SSource sources[1], const DstModifier& modifier);
    void Binary(e_opcode opCode, const SSource sources[2], const DstModifier& modifier);
    void Tenary(e_opcode opCode, const SSource sources[3], const DstModifier& modifier);

    void Mul64(CVariable* dst, CVariable* src[2], SIMDMode simdMode, bool noMask = false) const;

    template<int N>
    void Alu(e_opcode opCode, const SSource sources[N], const DstModifier& modifier);

    void BinaryUnary(llvm::Instruction* inst, const  SSource source[2], const DstModifier& modifier);
    void CmpBoolOp(llvm::BinaryOperator* inst,
        llvm::CmpInst::Predicate predicate,
        const  SSource source[2],
        const SSource& bitSource,
        const DstModifier& modifier);
    void emitAluConditionMod(Pattern* aluPattern, llvm::Instruction* alu, llvm::CmpInst* cmp);

    void EmitAluIntrinsic(llvm::CallInst* I, const SSource source[2], const DstModifier& modifier);
    void EmitSimpleAlu(llvm::Instruction* inst, const SSource source[2], const DstModifier& modifier);
    void EmitSimpleAlu(llvm::Instruction* inst, CVariable* dst, CVariable* src0, CVariable* src1);
    void EmitSimpleAlu(EOPCODE opCode, const SSource source[2], const DstModifier& modifier);
    void EmitSimpleAlu(EOPCODE opCode, CVariable* dst, CVariable* src0, CVariable* src1);
    void EmitMinMax(bool isMin, bool isUnsigned, const SSource source[2], const DstModifier& modifier);
    void EmitUAdd(llvm::BinaryOperator* inst, const DstModifier& modifier);
    void EmitFullMul32(bool isUnsigned, const SSource srcs[2], const DstModifier& dstMod);
    void EmitFPToIntWithSat(bool isUnsigned, bool needBitCast, VISA_Type type, const SSource& source, const DstModifier& modifier);
    void EmitNoModifier(llvm::Instruction* inst);
    void EmitIntrinsicMessage(llvm::IntrinsicInst* inst);
    void EmitGenIntrinsicMessage(llvm::GenIntrinsicInst* inst);
    void EmitSIToFPZExt(const SSource& source, const DstModifier& dstMod);
    void EmitIntegerTruncWithSat(bool isSignedDst, bool isSignedSrc, const SSource& source, const DstModifier& dstMod);
    void EmitAddPair(llvm::GenIntrinsicInst* GII, const SSource Sources[4], const DstModifier& DstMod);
    void EmitSubPair(llvm::GenIntrinsicInst* GII, const SSource Sources[4], const DstModifier& DstMod);
    void EmitMulPair(llvm::GenIntrinsicInst* GII, const SSource Sources[4], const DstModifier& DstMod);
    void EmitPtrToPair(llvm::GenIntrinsicInst* GII, const SSource Sources[1], const DstModifier& DstMod);
    void EmitInlineAsm(llvm::CallInst* inst);

    void emitPairToPtr(llvm::GenIntrinsicInst* GII);

    void emitMulAdd16(llvm::Instruction* I, const SSource source[2], const DstModifier& dstMod);
    void emitCall(llvm::CallInst* inst);
    void emitReturn(llvm::ReturnInst* inst);

    /// stack-call code-gen functions
    void emitStackCall(llvm::CallInst* inst);
    void emitStackFuncEntry(llvm::Function* F);
    void emitStackFuncExit(llvm::ReturnInst* inst);
    uint stackCallArgumentAlignment(CVariable* argv);
    uint emitStackArgumentLoadOrStore(std::vector<CVariable*>& Args, bool isWrite);
    void InitializeKernelStack(llvm::Function* pKernel);

    // emits the visa relocation instructions for function/global symbols
    void emitSymbolRelocation(llvm::Function& F);

    void emitOutput(llvm::GenIntrinsicInst* inst);
    void emitGS_SGV(llvm::SGVIntrinsic* inst);
    void emitSampleOffset(llvm::GenIntrinsicInst* inst);

    // TODO: unify the functions below and clean up
    void emitStore(llvm::StoreInst* inst, llvm::Value* offset = nullptr, llvm::ConstantInt* immOffset = nullptr);
    void emitStore3D(llvm::StoreInst* inst, llvm::Value* elemIdxV = nullptr);
    void emitStore3DInner(llvm::Value* pllValToStore, llvm::Value* pllDstPtr, llvm::Value* pllElmIdx);

    void emitLoad(llvm::LoadInst* inst, llvm::Value* offset = nullptr, llvm::ConstantInt* immOffset = nullptr);   // single load, no pattern
    void emitLoad3DInner(llvm::LdRawIntrinsic* inst, ResourceDescriptor& resource, llvm::Value* elemIdxV);

    // when resource is dynamically indexed, load/store must use special intrinsics
    void emitLoadRawIndexed(llvm::GenIntrinsicInst* inst);
    void emitStoreRawIndexed(llvm::GenIntrinsicInst* inst);
    void emitGetBufferPtr(llvm::GenIntrinsicInst* inst);
    // \todo, remove this function after we lower all GEP to IntToPtr before CodeGen.
    // Only remaining GEPs are for scratch in GFX path
    void emitGEP(llvm::Instruction* inst);

    // Emit lifetime start right before inst V. If ForAllInstance is true, emit lifestart
    // for both instances; otherwise, just the current instance set in the calling context.
    void emitLifetimeStart(CVariable* Var, llvm::BasicBlock* BB, llvm::Instruction* I, bool ForAllInstance);

    // set the predicate with current active channels
    void emitPredicateFromChannelIP(CVariable* dst, CVariable* alias = NULL);

    // Helper methods for message emit functions.
    template <typename T>
    void prepareRenderTargetWritePayload(
        T* inst,
        llvm::DenseMap<llvm::Value*, CVariable**>& valueToVariableMap,
        llvm::Value* color[],
        uint8_t colorCnt,
        //output:
        CVariable** src,
        bool* isUndefined,
        CVariable*& source0Alpha,
        CVariable*& oMaskOpnd,
        CVariable*& outputDepthOpnd,
        CVariable*& vStencilOpnd);


    ResourceDescriptor GetSampleResourceHelper(llvm::SampleIntrinsic* inst);

    void interceptSamplePayloadCoalescing(
        llvm::SampleIntrinsic* inst,
        uint numPart,
        llvm::SmallVector<CVariable*, 4> & payload,
        bool& payloadCovered
    );

    template <typename T>
    bool interceptRenderTargetWritePayloadCoalescing(
        T* inst,
        CVariable** src,
        CVariable*& source0Alpha,
        CVariable*& oMaskOpnd,
        CVariable*& outputDepthOpnd,
        CVariable*& vStencilOpnd,
        llvm::DenseMap<llvm::Value*, CVariable**>& valueToVariableMap);

    // message emit functions
    void emitRenderTargetWrite(llvm::RTWritIntrinsic* inst, bool fromRet);
    void emitDualBlendRT(llvm::RTDualBlendSourceIntrinsic* inst, bool fromRet);
    void emitSimdLaneId(llvm::Instruction* inst);
    void emitPatchInstanceId(llvm::Instruction* inst);
    void emitSimdSize(llvm::Instruction* inst);
    void emitSimdShuffle(llvm::Instruction* inst);
    void emitSimdShuffleDown(llvm::Instruction* inst);
    void emitSimdBlockRead(llvm::Instruction* inst, llvm::Value* ptrVal = nullptr);
    void emitSimdBlockWrite(llvm::Instruction* inst, llvm::Value* ptrVal = nullptr);
    void emitLegacySimdBlockWrite(llvm::Instruction* inst, llvm::Value* ptrVal = nullptr);
    void emitLegacySimdBlockRead(llvm::Instruction* inst, llvm::Value* ptrVal = nullptr);
    void emitSimdMediaBlockRead(llvm::Instruction* inst);
    void emitSimdMediaBlockWrite(llvm::Instruction* inst);
    void emitMediaBlockIO(const llvm::GenIntrinsicInst* inst, bool isRead);
    void emitMediaBlockRectangleRead(llvm::Instruction* inst);
    void emitURBWrite(llvm::GenIntrinsicInst* inst);
    void emitURBReadCommon(llvm::GenIntrinsicInst* inst, const QuadEltUnit globalOffset,
        llvm::Value* const perSlotOffset);
    void emitURBRead(llvm::GenIntrinsicInst* inst);
    void emitSampleInstruction(llvm::SampleIntrinsic* inst);
    void emitLdInstruction(llvm::Instruction* inst);
    void emitInfoInstruction(llvm::InfoIntrinsic* inst);
    void emitGather4Instruction(llvm::SamplerGatherIntrinsic* inst);
    void emitLdmsInstruction(llvm::Instruction* inst);
    void emitLdStructured(llvm::Instruction* inst);
    void emitStoreStructured(llvm::Instruction* inst);
    void emitTypedRead(llvm::Instruction* inst);
    void emitTypedWrite(llvm::Instruction* inst);
    void emitThreadGroupBarrier(llvm::Instruction* inst);
    void emitMemoryFence(llvm::Instruction* inst);
    void emitMemoryFence(void);
    void emitTypedMemoryFence(llvm::Instruction* inst);
    void emitFlushSamplerCache();
    void emitSurfaceInfo(llvm::GenIntrinsicInst* intrinsic);

    void emitStackAlloca(llvm::GenIntrinsicInst* intrinsic);

    void emitUAVSerialize();

    void emitScalarAtomics(
        llvm::Instruction* pInst,
        ResourceDescriptor& resource,
        AtomicOp atomic_op,
        CVariable* pDstAddr,
        CVariable* pSrc,
        bool isA64,
        int bitSize);

    void emitScalarAtomicLoad(
        llvm::Instruction* pInst,
        ResourceDescriptor& resource,
        CVariable* pDstAddr,
        CVariable* pSrc,
        bool isA64,
        int bitSize);

    /// reduction and prefix/postfix facilities
    CVariable* ScanReducePrepareSrc(VISA_Type type, uint64_t identityValue, bool negate, bool secondHalf,
        CVariable* src, CVariable* dst, CVariable* flag = nullptr);
    CVariable* ReductionReduceHelper(e_opcode op, VISA_Type type, SIMDMode simd, CVariable* src);
    void ReductionExpandHelper(e_opcode op, VISA_Type type, CVariable* src, CVariable* dst);
    void ReductionClusteredSrcHelper(CVariable* (&pSrc)[2], CVariable* src, uint16_t numLanes,
        VISA_Type type, uint numInst, bool secondHalf);
    CVariable* ReductionClusteredReduceHelper(e_opcode op, VISA_Type type, SIMDMode simd, bool secondHalf,
        CVariable* src, CVariable* dst);
    void ReductionClusteredExpandHelper(e_opcode op, VISA_Type type, SIMDMode simd, const uint clusterSize,
        bool secondHalf, CVariable* src, CVariable* dst);
    /// reduction and prefix/postfix emitters
    void emitReductionAll(
        e_opcode op,
        uint64_t identityValue,
        VISA_Type type,
        bool negate,
        CVariable* src,
        CVariable* dst);
    void emitReductionClustered(
        const e_opcode op,
        const uint64_t identityValue,
        const VISA_Type type,
        const bool negate,
        const unsigned int clusterSize,
        CVariable* const src,
        CVariable* const dst);
    void emitPreOrPostFixOp(
        e_opcode op,
        uint64_t identityValue,
        VISA_Type type,
        bool negateSrc,
        CVariable* src,
        CVariable* result[2],
        CVariable* Flag = nullptr,
        bool isPrefix = false,
        bool isQuad = false);
    void emitPreOrPostFixOpScalar(
        e_opcode op,
        uint64_t identityValue,
        VISA_Type type,
        bool negateSrc,
        CVariable* src,
        CVariable* result[2],
        CVariable* Flag,
        bool isPrefix);

    bool IsUniformAtomic(llvm::Instruction* pInst);
    void emitAtomicRaw(llvm::GenIntrinsicInst* pInst);
    void emitAtomicStructured(llvm::Instruction* pInst);
    void emitAtomicTyped(llvm::GenIntrinsicInst* pInst);
    void emitAtomicCounter(llvm::GenIntrinsicInst* pInst);
    void emitUniformAtomicCounter(llvm::GenIntrinsicInst* pInst);
    void emitRenderTargetRead(llvm::GenIntrinsicInst* inst);

    void emitDiscard(llvm::Instruction* inst);
    void emitInitDiscardMask(llvm::GenIntrinsicInst* inst);
    void emitUpdateDiscardMask(llvm::GenIntrinsicInst* inst);
    void emitGetPixelMask(llvm::GenIntrinsicInst* inst);

    void emitInput(llvm::Instruction* inst);
    void emitcycleCounter(llvm::Instruction* inst);
    void emitSetDebugReg(llvm::Instruction* inst);
    void emitInsert(llvm::Instruction* inst);
    void emitExtract(llvm::Instruction* inst);
    void emitBitCast(llvm::BitCastInst* btCst);
    void emitPtrToInt(llvm::PtrToIntInst* p2iCst);
    void emitIntToPtr(llvm::IntToPtrInst* i2pCst);
    void emitAddrSpaceCast(llvm::AddrSpaceCastInst* addrSpaceCast);
    void emitBranch(llvm::BranchInst* br, const SSource& cond, e_predMode predMode);
    void emitDiscardBranch(llvm::BranchInst* br, const SSource& cond);
    void emitAluNoModifier(llvm::GenIntrinsicInst* inst);

    void emitSGV(llvm::SGVIntrinsic* inst);
    void emitPSSGV(llvm::GenIntrinsicInst* inst);
    void emitCSSGV(llvm::GenIntrinsicInst* inst);
    void getCoarsePixelSize(CVariable* destination, const uint component);
    void getPixelPosition(CVariable* destination, const uint component);
    void emitPixelPosition(llvm::GenIntrinsicInst* inst);
    void emitPhaseOutput(llvm::GenIntrinsicInst* inst);
    void emitPhaseInput(llvm::GenIntrinsicInst* inst);

    void emitPSInput(llvm::Instruction* inst);
    void emitPSInputMADHalf(llvm::Instruction* inst);
    void emitPSInputPln(llvm::Instruction* inst);
    void emitPSInputCst(llvm::Instruction* inst);
    void emitEvalAttribute(llvm::GenIntrinsicInst* inst);
    void emitInterpolate(llvm::GenIntrinsicInst* inst);
    void emitInterpolate2(llvm::GenIntrinsicInst* inst);
    void emitInterpolant(llvm::GenIntrinsicInst* inst);

    void emitGradientX(const SSource& source, const DstModifier& modifier);
    void emitGradientY(const SSource& source, const DstModifier& modifier);
    void emitGradientXFine(const SSource& source, const DstModifier& modifier);
    void emitGradientYFine(const SSource& source, const DstModifier& modifier);

    void emitHSTessFactors(llvm::Instruction* pInst);
    void emitHSSGV(llvm::GenIntrinsicInst* inst);
    void emitf32tof16_rtz(llvm::GenIntrinsicInst* inst);
    void emitfitof(llvm::GenIntrinsicInst* inst);
    void emitFPOrtz(llvm::GenIntrinsicInst* inst);
    void emitftoi(llvm::GenIntrinsicInst* inst);
    void emitCtlz(const SSource& source);

    void emitDSInput(llvm::Instruction* pInst);
    void emitDSSGV(llvm::GenIntrinsicInst* inst);

    // VME
    void emitVMESendIME(llvm::GenIntrinsicInst* inst);
    void emitVMESendFBR(llvm::GenIntrinsicInst* inst);
    void emitVMESendSIC(llvm::GenIntrinsicInst* inst);
    void emitVMESendIME2(llvm::GenIntrinsicInst* inst);
    void emitVMESendFBR2(llvm::GenIntrinsicInst* inst);
    void emitVMESendSIC2(llvm::GenIntrinsicInst* inst);
    void emitCreateMessagePhases(llvm::GenIntrinsicInst* inst);
    void emitSetMessagePhaseX_legacy(llvm::GenIntrinsicInst* inst);
    void emitSetMessagePhase_legacy(llvm::GenIntrinsicInst* inst);
    void emitGetMessagePhaseX(llvm::GenIntrinsicInst* inst);
    void emitSetMessagePhaseX(llvm::GenIntrinsicInst* inst);
    void emitGetMessagePhase(llvm::GenIntrinsicInst* inst);
    void emitSetMessagePhase(llvm::GenIntrinsicInst* inst);
    void emitSimdGetMessagePhase(llvm::GenIntrinsicInst* inst);
    void emitBroadcastMessagePhase(llvm::GenIntrinsicInst* inst);
    void emitSimdSetMessagePhase(llvm::GenIntrinsicInst* inst);
    void emitSimdMediaRegionCopy(llvm::GenIntrinsicInst* inst);
    void emitExtractMVAndSAD(llvm::GenIntrinsicInst* inst);
    void emitCmpSADs(llvm::GenIntrinsicInst* inst);

    // VA
    void emitVideoAnalyticSLM(llvm::GenIntrinsicInst* inst, const DWORD responseLen);
    // New VA without using SLM and barriers (result is returned in GRF).
    void emitVideoAnalyticGRF(llvm::GenIntrinsicInst* inst, const DWORD responseLen);

    // CrossLane Instructions
    void emitWaveBallot(llvm::GenIntrinsicInst* inst);
    void emitWaveInverseBallot(llvm::GenIntrinsicInst* inst);
    void emitWaveShuffleIndex(llvm::GenIntrinsicInst* inst);
    void emitWavePrefix(llvm::WavePrefixIntrinsic* I);
    void emitQuadPrefix(llvm::QuadPrefixIntrinsic* I);
    void emitWaveAll(llvm::GenIntrinsicInst* inst);
    void emitWaveClustered(llvm::GenIntrinsicInst* inst);

    // Those three "vector" version shall be combined with
    // non-vector version.
    bool isUniformStoreOCL(llvm::StoreInst* SI);
    void emitVectorBitCast(llvm::BitCastInst* BCI);
    void emitVectorLoad(llvm::LoadInst* LI, llvm::Value* offset, llvm::ConstantInt* immOffset);
    void emitVectorStore(llvm::StoreInst* SI, llvm::Value* offset, llvm::ConstantInt* immOffset);
    void emitGenISACopy(llvm::GenIntrinsicInst* GenCopyInst);
    void emitVectorCopy(CVariable* Dst, CVariable* Src, uint32_t nElts,
        uint32_t DstSubRegOffset = 0, uint32_t SrcSubRegOffset = 0);
    void emitCopyAll(CVariable* Dst, CVariable* Src, llvm::Type* Ty);

    void emitPushToStack(CVariable* pushOffset);
    void emitAddSP(CVariable* Dst, CVariable* Src, CVariable* offset);
    // emitAddPair - emulate 64bit addtition by 32-bit operations.
    // Dst and Src0 must be a 64-bit type variable.
    // Src1 mist be in 32-bit type variable/immediate
    void emitAddPair(CVariable* Dst, CVariable* Src0, CVariable* Src1);

    void emitSqrt(llvm::Instruction* inst);
    void emitCanonicalize(llvm::Instruction* inst);
    void emitRsq(llvm::Instruction* inst);

    void emitLLVMbswap(llvm::IntrinsicInst* inst);
    void emitDP4A(llvm::GenIntrinsicInst* GII);
    // Debug Built-Ins
    void emitStateRegID(uint64_t and_imm, uint64_t shr_imm);
    void emitThreadPause(llvm::GenIntrinsicInst* inst);

    void MovPhiSources(llvm::BasicBlock* bb);

    void InitConstant(llvm::BasicBlock* BB);
    void emitLifetimeStartAtEndOfBB(llvm::BasicBlock* BB);
    void emitDebugPlaceholder(llvm::GenIntrinsicInst* I);

    std::pair<llvm::Value*, llvm::Value*> getPairOutput(llvm::Value*) const;

    //helper function
    void SplitSIMD(llvm::Instruction* inst, uint numSources, uint headerSize, CVariable* payload, SIMDMode mode, uint half);
    template<size_t N>
    void JoinSIMD(CVariable* (&tempdst)[N], uint responseLength, SIMDMode mode);
    CVariable* BroadcastIfUniform(CVariable* pVar);
    uint DecideInstanceAndSlice(llvm::BasicBlock& blk, SDAG& sdag, bool& slicing);
    inline bool isUndefOrConstInt0(llvm::Value* val)
    {
        if (val == nullptr ||
            llvm::isa<llvm::UndefValue>(val) ||
            (llvm::isa<llvm::ConstantInt>(val) &&
                llvm::cast<llvm::ConstantInt>(val)->getZExtValue() == 0))
        {
            return true;
        }
        return false;
    }
    inline llvm::Value* getOperandIfExist(llvm::Instruction* pInst, unsigned op)
    {
        if (llvm::CallInst * pCall = llvm::dyn_cast<llvm::CallInst>(pInst))
        {
            if (op < pCall->getNumArgOperands())
            {
                return pInst->getOperand(op);
            }
        }
        return nullptr;
    }

    bool IsGRFAligned(CVariable* pVar, e_alignment requiredAlign) const
    {
        e_alignment align = pVar->GetAlign();
        if (requiredAlign == EALIGN_BYTE)
        {
            // trivial
            return true;
        }
        if (requiredAlign == EALIGN_AUTO || align == EALIGN_AUTO)
        {
            // Can only assume that AUTO only matches AUTO (?)
            // (keep the previous behavior unchanged.)
            return align == requiredAlign;
        }
        return align >= requiredAlign;
    }

    CVariable* ExtendVariable(CVariable* pVar, e_alignment uniformAlign);
    CVariable* BroadcastAndExtend(CVariable* pVar);
    CVariable* TruncatePointer(CVariable* pVar);
    CVariable* ReAlignUniformVariable(CVariable* pVar, e_alignment align);
    CVariable* BroadcastAndTruncPointer(CVariable* pVar);
    CVariable* IndexableResourceIndex(CVariable* indexVar, uint btiIndex);
    ResourceDescriptor GetResourceVariable(llvm::Value* resourcePtr);
    SamplerDescriptor GetSamplerVariable(llvm::Value* samplerPtr);
    CVariable* ComputeSampleIntOffset(llvm::Instruction* sample, uint sourceIndex);
    void emitPlnInterpolation(CVariable* bary, CVariable* inputvar);

    CVariable* GetExecutionMask();
    CVariable* GetExecutionMask(CVariable* &vecMaskVar);
    CVariable* GetHalfExecutionMask();
    CVariable* GetDispatchMask();
    CVariable* UniformCopy(CVariable* var);
    CVariable* UniformCopy(CVariable* var, CVariable*& LaneOffset, CVariable* eMask = nullptr, bool doSub = false);
    // generate loop header to process sample instruction with varying resource/sampler
    bool ResourceLoopHeader(
        ResourceDescriptor& resource,
        SamplerDescriptor& sampler,
        CVariable*& flag,
        uint& label);
    bool ResourceLoopHeader(
        ResourceDescriptor& resource,
        CVariable*& flag,
        uint& label);
    void ResourceLoop(bool needLoop, CVariable* flag, uint label);

    void ForceDMask(bool createJmpForDiscard = true);
    void ResetVMask(bool createJmpForDiscard = true);
    void setPredicateForDiscard(CVariable* pPredicate = nullptr);

    void PackSIMD8HFRet(CVariable* dst);
    unsigned int GetScalarTypeSizeInRegister(llvm::Type* Ty) const;

    /// return true if succeeds, false otherwise.
    bool setCurrentShader(llvm::Function* F);

    // Arithmetic operations with constant folding
    // Src0 and Src1 are the input operands
    // DstPrototype is a prototype of the result of operation and may be used for cloning to a new variable
    // Return a variable with the result of the compute which may be one the the sources, an immediate or a variable
    CVariable* Mul(CVariable* Src0, CVariable* Src1, const CVariable* DstPrototype);
    CVariable* Add(CVariable* Src0, CVariable* Src1, const CVariable* DstPrototype);

    // temporary helper function
    CVariable* GetSymbol(llvm::Value* v);

    CVariable* GetSrcVariable(const SSource& source, bool fromConstPool = false);
    void SetSourceModifiers(unsigned int sourceIndex, const SSource& source);

    SBasicBlock& getCurrentBlock() const { return m_pattern->m_blocks[m_currentBlock]; }

    CodeGenContext* m_pCtx;
    CVariable* m_destination;
    GenXFunctionGroupAnalysis* m_FGA;
    CodeGenPatternMatch* m_pattern;
    DeSSA* m_deSSA;
    BlockCoalescing* m_blockCoalescing;
    SIMDMode m_SimdMode;
    ShaderDispatchMode m_ShaderDispatchMode;
    CShaderProgram::KernelShaderMap& m_shaders;
    CShader* m_currShader;
    CEncoder* m_encoder;
    const llvm::DataLayout* m_DL;
    CoalescingEngine* m_CE;
    VariableReuseAnalysis* m_VRA;
    ModuleMetaData* m_moduleMD;
    bool m_canAbortOnSpill;
    PSSignature* m_pSignature;

    // Debug info emitter
    IDebugEmitter* m_pDebugEmitter;

    llvm::DominatorTree* m_pDT;
    static char ID;

private:
    uint m_labelForDMaskJmp;

    // Current rounding Mode
    //   As RM of FPCvtInt and FP could be different, there
    //   are two fields to keep track of their current values.
    //
    // Default rounding modes:
    //   the rounding modes that are pre-defined by each API or
    //   shaders/kernels.
    //
    //   Not all combinations of FP's RM and FPCvtInt's RM can be
    //   used as default. Currently, the default RMs have the
    //   following restrictions:
    //      1. If FPCvtInt's RM = ROUND_TO_ZERO, FP's RM can be any;
    //      2. Otherwise, FPCvtInt's RM must be the same as FP's RM
    //
    //   The default remains unchanged throughout the entire
    //   shaders/kernels. Dynamically setting a different default
    //   rounding mode in the middle of a shader/kernel is not
    //   supported for now. And the default remains unchanged
    //   throughout the entire shaders/kernels.
    //
    //   However, each instruction's RM can be set dynamically,
    //   such as via intrinsics. If an instruction needs setting RMs,
    //   its RMs must follow the above restrictions. So far, an
    //   instruction either relies on FP's RM or FPCvtInt's RM, but
    //   not both, thus setting an instruction's RM dynamically
    //   cannot violate the above restrictions.
    //
    ERoundingMode m_roundingMode_FP;
    ERoundingMode m_roundingMode_FPCvtInt;

    uint m_currentBlock = (uint) -1;

    // Used to relocate phi-mov to different BB. phiMovToBB is the map from "fromBB"
    // to "toBB" (meaning to move phi-mov from "fromBB" to "toBB"). See MovPhiSources.
    llvm::DenseMap<llvm::BasicBlock*, llvm::BasicBlock*>  phiMovToBB;
    bool canRelocatePhiMov(
        llvm::BasicBlock* otherBB, llvm::BasicBlock* phiMovBB, llvm::BasicBlock* phiBB);
    bool isCandidateIfStmt(
        llvm::BasicBlock* ifBB, llvm::BasicBlock*& otherBB, llvm::BasicBlock*& emptyBB);

    // Used to check for the constraint types with the actual llvmIR params for inlineASM instructions
    bool validateInlineAsmConstraints(llvm::CallInst* inst, llvm::SmallVector<llvm::StringRef, 8> & constraints);

    void emitGetMessagePhaseType(llvm::GenIntrinsicInst* inst, VISA_Type type, uint32_t width);
    void emitSetMessagePhaseType(llvm::GenIntrinsicInst* inst, VISA_Type type);
    void emitSetMessagePhaseType_legacy(llvm::GenIntrinsicInst* inst, VISA_Type type);

    void emitScan(llvm::Value* Src, IGC::WaveOps Op,
        bool isInclusiveScan, llvm::Value* Mask, bool isQuad);

    // Cached per lane offset variables. This is a per basic block data
    // structure. For each entry, the first item is the scalar type size in
    // bytes, the second item is the corresponding symbol.
    llvm::SmallVector<std::pair<unsigned, CVariable*>, 4> PerLaneOffsetVars;

    // Helper function to reduce common code for emitting indirect address
    // computation.
    CVariable* getOrCreatePerLaneOffsetVariable(unsigned TypeSizeInBytes)
    {
        for (auto Item : PerLaneOffsetVars)
        {
            if (Item.first == TypeSizeInBytes)
            {
                IGC_ASSERT_MESSAGE(Item.second, "null variable");
                return Item.second;
            }
        }
        CVariable* Var = m_currShader->GetPerLaneOffsetsReg(TypeSizeInBytes);
        PerLaneOffsetVars.push_back(std::make_pair(TypeSizeInBytes, Var));
        return Var;
    }

    // Emit code in slice starting from (reverse) iterator I. Return the
    // iterator to the next pattern to emit.
    SBasicBlock::reverse_iterator emitInSlice(SBasicBlock& block,
        SBasicBlock::reverse_iterator I);

    /**
        * Reuse SampleDescriptor for sampleID, so that we can pass it to
        * ResourceLoop to generate loop for non-uniform values.
        */
    inline SamplerDescriptor getSampleIDVariable(llvm::Value* sampleIdVar)
    {
        SamplerDescriptor sampler;
        sampler.m_sampler = GetSymbol(sampleIdVar);
        return sampler;
    }

    CVariable* UnpackOrBroadcastIfUniform(CVariable* pVar);

    int getGRFSize() const { return m_currShader->getGRFSize(); }

    void initDefaultRoundingMode();
    ERoundingMode GetRoundingMode_FPCvtInt(llvm::Instruction* pInst);
    ERoundingMode GetRoundingMode_FP(llvm::Instruction* inst);
    void SetRoundingMode_FP(ERoundingMode RM_FP);
    void SetRoundingMode_FPCvtInt(ERoundingMode RM_FPCvtInt);
    bool setRMExplicitly(llvm::Instruction* inst);
    void ResetRoundingMode(llvm::Instruction* inst);
    // returns true if the instruction does not care about the rounding mode settings
    bool ignoreRoundingMode(llvm::Instruction* inst) const;

    // A64 load/store with HWA that make sure the offset hi part is the same per LS call
    // addrUnifrom: if the load/store address is uniform, we can skip A64 WA
    void emitGatherA64(llvm::Value* loadInst, CVariable* dst, CVariable* offset, unsigned elemSize, unsigned numElems, bool addrUniform);
    void emitGather4A64(llvm::Value* loadInst, CVariable* dst, CVariable* offset, bool addrUniform);
    void emitScatterA64(CVariable* val, CVariable* offset, unsigned elementSize, unsigned numElems, bool addrUniform);
    void emitScatter4A64(CVariable* src, CVariable* offset, bool addrUniform);

    // Helper functions that create loop for above WA
    void A64LSLoopHead(CVariable* addr, CVariable*& curMask, CVariable*& lsPred, uint& label);
    void A64LSLoopTail(CVariable* curMask, CVariable* lsPred, uint label);

    // Helper function to check if A64 WA is required
    bool hasA64WAEnable() const;

    bool isHalfGRFReturn(CVariable* dst, SIMDMode simdMode);

    void emitFeedbackEnable();

    // used for loading/storing uniform value using scatter/gather messages.
    CVariable* prepareAddressForUniform(
        CVariable* AddrVar, uint32_t EltBytes, uint32_t NElts, uint32_t ExecSz, e_alignment Align);
    CVariable* prepareDataForUniform(CVariable* DataVar, uint32_t ExecSz, e_alignment Align);
};

} // namespace IGC
