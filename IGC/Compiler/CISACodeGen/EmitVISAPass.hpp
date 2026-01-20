/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "LoopCountAnalysis.hpp"
#include "BlockCoalescing.hpp"
#include "PatternMatchPass.hpp"
#include "ShaderCodeGen.hpp"
#include "CoalescingEngine.hpp"
#include "Simd32Profitability.hpp"
#include "GenCodeGenModule.h"
#include "VariableReuseAnalysis.hpp"
#include "CastToGASAnalysis.h"
#include "ResourceLoopAnalysis.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

#include <functional>
#include <optional>
#include <type_traits>

namespace llvm {
class GenIntrinsicInst;
}

namespace IGC {
// Forward declaration
class IDebugEmitter;
struct PSSignature;
void initializeEmitPassPass(llvm::PassRegistry &);

class EmitPass : public llvm::FunctionPass {
  template <typename P> void addRequired(llvm::AnalysisUsage &AU) const {
    static_assert(std::is_base_of_v<llvm::FunctionPass, P> || std::is_base_of_v<llvm::ImmutablePass, P>);
    AU.addRequired<P>();
  }

public:
  EmitPass();
  EmitPass(CShaderProgram::KernelShaderMap &shaders, SIMDMode mode, bool canAbortOnSpill, ShaderDispatchMode shaderMode,
           PSSignature *pSignature = nullptr);

  virtual ~EmitPass();

  EmitPass(const EmitPass &) = delete;
  EmitPass &operator=(const EmitPass &) = delete;

  // Note:  all analysis passes should be function passes. If a module analysis pass
  //        is used, it would invalidate function analysis passes and therefore cause
  //        those analysis passes to be invoked twice, which increases compiling time.
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();

    addRequired<llvm::DominatorTreeWrapperPass>(AU);
    addRequired<WIAnalysis>(AU);
    addRequired<LiveVarsAnalysis>(AU);
    addRequired<CodeGenPatternMatch>(AU);
    addRequired<DeSSA>(AU);
    addRequired<BlockCoalescing>(AU);
    addRequired<CoalescingEngine>(AU);
    addRequired<MetaDataUtilsWrapper>(AU);
    addRequired<Simd32ProfitabilityAnalysis>(AU);
    addRequired<CodeGenContextWrapper>(AU);
    addRequired<VariableReuseAnalysis>(AU);
    addRequired<CastToGASInfo>(AU);
    addRequired<ResourceLoopAnalysis>(AU);
  }

  virtual bool runOnFunction(llvm::Function &F) override;
  virtual llvm::StringRef getPassName() const override { return "EmitPass"; }

  void CreateKernelShaderMap(CodeGenContext *ctx, IGC::IGCMD::MetaDataUtils *pMdUtils, llvm::Function &F);

  bool isVectorEmissionPossible(const SSource sources[2], CVariable *src[2]);

  void Frc(const SSource &source, const DstModifier &modifier);
  void Floor(const SSource &source, const DstModifier &modifier);
  void Mad(const SSource sources[3], const DstModifier &modifier);
  void Lrp(const SSource sources[3], const DstModifier &modifier);
  void Cmp(llvm::CmpInst::Predicate pred, const SSource sources[2], const DstModifier &modifier,
           uint8_t clearTagMask = 0);
  void VectorCMP(llvm::CmpInst::Predicate pred, const SSource sources[2], const DstModifier &modifier,
                 uint8_t clearTagMask = 0);

  void Sub(const SSource[2], const DstModifier &mofidier);
  void Xor(const SSource[2], const DstModifier &modifier);
  void FDiv(const SSource[2], const DstModifier &modifier);
  void VectorMad(const SSource sources[3], const DstModifier &modifier);
  void Pow(const SSource sources[2], const DstModifier &modifier);
  void Avg(const SSource sources[2], const DstModifier &modifier);
  void Rsqrt(const SSource &source, const DstModifier &modifier);

  void VectorSelect(const SSource sources[3], const DstModifier &modifier);
  void Select(const SSource sources[3], const DstModifier &modifier);
  void PredAdd(const SSource &pred, bool invert, const SSource sources[2], const DstModifier &modifier);
  void Mul(const SSource[2], const DstModifier &modifier);
  void Div(const SSource[2], const DstModifier &modifier);
  void Inv(const SSource[2], const DstModifier &modifier);
  void Exp2(const SSource[2], const DstModifier &modifier);
  void MaxNum(const SSource sources[2], const DstModifier &modifier);
  void Add(const SSource[2], const DstModifier &modifier);
  void FPTrunc(const SSource[2], const DstModifier &modifier);
  void Powi(const SSource[2], const DstModifier &modifier);
  void Mov(const SSource &source, const DstModifier &modifier);
  void Unary(e_opcode opCode, const SSource sources[1], const DstModifier &modifier);
  void Binary(e_opcode opCode, const SSource sources[2], const DstModifier &modifier);
  void Tenary(e_opcode opCode, const SSource sources[3], const DstModifier &modifier);
  void Bfn(uint8_t booleanFuncCtrl, const SSource sources[3], const DstModifier &modifier);
  void CmpBfn(llvm::CmpInst::Predicate predicate, const SSource cmpSources[2], uint8_t booleanFuncCtrl,
              const SSource bfnSources[3], const DstModifier &modifier);

  enum class ResultScope { Low = 0b01, Hi = 0b10, All = 0b11 };
  bool canUseMul64SOA() const;
  void emitMul64SOA(CVariable *DstL, CVariable *DstH, CVariable *L0, CVariable *L1, CVariable *H0, CVariable *H1,
                    std::optional<SIMDMode> Mode = std::nullopt, bool NoMask = false,
                    const SSource Sources[4] = nullptr) const;
  void emitShflIdx4(Instruction *inst, bool packed);
  void emitShflIdx4Lut(Instruction *inst);
  void Mul64(CVariable *dst, CVariable *src[2], SIMDMode simdMode, bool noMask = false,
             ResultScope mulResultScope = ResultScope::Low, CVariable *outDstHi = nullptr) const;

  template <int N> void Alu(e_opcode opCode, const SSource sources[N], const DstModifier &modifier);

  void BinaryUnary(llvm::Instruction *inst, const SSource source[2], const DstModifier &modifier);
  void CmpBoolOp(Pattern *cmpPattern, llvm::BinaryOperator *inst, const SSource &bitSource,
                 const DstModifier &modifier);
  void emitAluConditionMod(Pattern *aluPattern, llvm::Instruction *alu, llvm::CmpInst *cmp, int aluOprdNum);

  void EmitGenericPointersCmp(llvm::Instruction *inst, const SSource source[2], const DstModifier &modifier,
                              uint8_t clearTagMask);
  void EmitAluIntrinsic(llvm::CallInst *I, const SSource source[2], const DstModifier &modifier);
  void EmitSimpleAlu(llvm::Instruction *inst, const SSource source[2], const DstModifier &modifier);
  void EmitSimpleAlu(llvm::Instruction *inst, CVariable *dst, CVariable *src0, CVariable *src1);
  void EmitSimpleAlu(EOPCODE opCode, const SSource source[2], const DstModifier &modifier, bool isUnsigned = false);
  void EmitSimpleAlu(EOPCODE opCode, CVariable *dst, CVariable *src0, CVariable *src1);
  void EmitMinMax(bool isMin, bool isUnsigned, const SSource source[2], const DstModifier &modifier);
  void EmitUAdd(llvm::BinaryOperator *inst, const DstModifier &modifier);
  void EmitFullMul32(bool isUnsigned, const SSource srcs[2], const DstModifier &dstMod);
  void EmitFPToIntWithSat(bool isUnsigned, bool needBitCast, VISA_Type type, const SSource &source,
                          const DstModifier &modifier);
  void EmitNoModifier(llvm::Instruction *inst);
  void EmitIntrinsicMessage(llvm::IntrinsicInst *inst);
  void EmitGenIntrinsicMessage(llvm::GenIntrinsicInst *inst);
  void EmitSIToFPZExt(const SSource &source, const DstModifier &dstMod);
  void EmitIntegerTruncWithSat(bool isSignedDst, bool isSignedSrc, const SSource &source, const DstModifier &dstMod);
  void EmitPack4i8(const std::array<EOPCODE, 4> &opcodes, const std::array<SSource, 4> &sources0,
                   const std::array<SSource, 4> &sources1, const std::array<bool, 4> isSat, const DstModifier &dstMod);
  void EmitUnpack4i8(const SSource &source, uint32_t index, bool isUnsigned, const DstModifier &dstMod);
  void EmitRepack4i8(const std::array<SSource, 4> &sources, const std::array<uint32_t, 4> &mappings,
                     const DstModifier &dstMod);
  void EmitAddPair(llvm::GenIntrinsicInst *GII, const SSource Sources[4], const DstModifier &DstMod);
  void EmitSubPair(llvm::GenIntrinsicInst *GII, const SSource Sources[4], const DstModifier &DstMod);
  void EmitMulPair(llvm::GenIntrinsicInst *GII, const SSource Sources[4], const DstModifier &DstMod);
  void EmitPtrToPair(llvm::GenIntrinsicInst *GII, const SSource Sources[1], const DstModifier &DstMod);
  void EmitInlineAsm(llvm::CallInst *inst);
  void EmitInitializePHI(llvm::PHINode *phi);

  void emitPairToPtr(llvm::GenIntrinsicInst *GII);

  void emitMulAdd16(llvm::Instruction *I, const SSource source[2], const DstModifier &dstMod);
  void emitCall(llvm::CallInst *inst);
  void emitReturn(llvm::ReturnInst *inst);
  void EmitInsertValueToStruct(llvm::InsertValueInst *II);
  void EmitExtractValueFromStruct(llvm::ExtractValueInst *EI);
  void EmitInsertValueToLayoutStruct(llvm::InsertValueInst *IVI);
  void EmitExtractValueFromLayoutStruct(llvm::ExtractValueInst *EVI);
  void EmitSelectStruct(llvm::SelectInst *SI);
  void emitVectorCopyToAOS(uint32_t AOSBytes, CVariable *Dst, CVariable *Src, uint32_t nElts,
                           uint32_t DstSubRegOffset = 0, uint32_t SrcSubRegOffset = 0) {
    emitVectorCopyToOrFromAOS(AOSBytes, Dst, Src, nElts, DstSubRegOffset, SrcSubRegOffset, true);
  }
  void emitVectorCopyFromAOS(uint32_t AOSBytes, CVariable *Dst, CVariable *Src, uint32_t nElts,
                             uint32_t DstSubRegOffset = 0, uint32_t SrcSubRegOffset = 0) {
    emitVectorCopyToOrFromAOS(AOSBytes, Dst, Src, nElts, DstSubRegOffset, SrcSubRegOffset, false);
  }
  void emitVectorCopyToOrFromAOS(uint32_t AOSBytes, CVariable *Dst, CVariable *Src, uint32_t nElts,
                                 uint32_t DstSubRegOffset, uint32_t SrcSubRegOffset, bool IsToAOS);
  void emitCopyToOrFromLayoutStruct(llvm::Value *D, llvm::Value *S);
  void emitLayoutStructCopyAOSToAOS(uint32_t AOSBytes, CVariable *Dst, CVariable *Src, uint32_t nElts,
                                    uint32_t DstSubRegOffset = 0, uint32_t SrcSubRegOffset = 0);

  /// stack-call code-gen functions
  void emitStackCall(llvm::CallInst *inst);
  void emitStackFuncEntry(llvm::Function *F);
  void emitStackFuncExit(llvm::ReturnInst *inst);
  void InitializeKernelStack(llvm::Function *pKernel, CVariable *stackBufferBase = nullptr);

  /// stack-call functions for reading and writing argument/retval data to stack
  typedef SmallVector<std::tuple<CVariable *, uint32_t, uint32_t, uint32_t, bool>, 8> StackDataBlocks;
  uint CalculateStackDataBlocks(StackDataBlocks &blkData, std::vector<CVariable *> &Args);
  void ReadStackDataBlocks(StackDataBlocks &blkData, uint offsetS);
  void WriteStackDataBlocks(StackDataBlocks &blkData, uint offsetS);
  void emitCopyGRFBlock(CVariable *Dst, CVariable *Src, Type *type, uint32_t BlkOffset, unsigned numInstance,
                        bool isWriteToBlk);

  // emits the visa relocation instructions for function/global symbols
  void emitSymbolRelocation(llvm::Function &F);

  void emitOutput(llvm::GenIntrinsicInst *inst);

  // TODO: unify the functions below and clean up
  void emitStore(llvm::StoreInst *inst, llvm::Value *varOffset, llvm::ConstantInt *immOffset,
                 ConstantInt *immScale = nullptr, llvm::Value *uniformBase = nullptr, bool signExtendOffset = false,
                 bool zeroExtendOffset = false);
  void emitPredicatedStore(llvm::Instruction *inst);
  void emitStore3DInner(llvm::Value *pllValToStore, llvm::Value *pllDstPtr, llvm::Value *pllElmIdx);

  void emitLoad(llvm::LoadInst *inst, llvm::Value *varOffset, llvm::ConstantInt *immOffset,
                ConstantInt *immScale = nullptr, llvm::Value *uniformBase = nullptr, bool signExtendOffset = false,
                bool zeroExtendOffset = false); // single load, no pattern
  void emitPredicatedLoad(llvm::Instruction *inst);
  void emitLoad3DInner(llvm::LdRawIntrinsic *inst, ResourceDescriptor &resource, llvm::Value *elemIdxV);

  // when resource is dynamically indexed, load/store must use special intrinsics
  void emitLoadRawIndexed(llvm::LdRawIntrinsic *inst, llvm::Value *varOffset, llvm::ConstantInt *immScale,
                          llvm::ConstantInt *immOffset);
  void emitStoreRawIndexed(llvm::StoreRawIntrinsic *inst, llvm::Value *varOffset, llvm::ConstantInt *immScale,
                           llvm::ConstantInt *immOffset);
  void emitGetBufferPtr(llvm::GenIntrinsicInst *inst);
  // \todo, remove this function after we lower all GEP to IntToPtr before CodeGen.
  // Only remaining GEPs are for scratch in GFX path
  void emitGEP(llvm::Instruction *inst);


  // Emit lifetime start right before inst V. If ForAllInstance is true, emit lifestart
  // for both instances; otherwise, just the current instance set in the calling context.
  void emitLifetimeStart(CVariable *Var, llvm::BasicBlock *BB, llvm::Instruction *I, bool ForAllInstance);
  bool waveShuffleCase(CVariable *Var, BasicBlock *BB, Instruction *I, bool ForAllInstance);

  // Helper methods for message emit functions.
  template <typename T>
  void prepareRenderTargetWritePayload(T *inst, llvm::DenseMap<llvm::Value *, CVariable **> &valueToVariableMap,
                                       llvm::Value *color[], uint8_t colorCnt,
                                       // output:
                                       CVariable **src, bool *isUndefined, CVariable *&source0Alpha,
                                       CVariable *&oMaskOpnd, CVariable *&outputDepthOpnd, CVariable *&vStencilOpnd);


  ResourceDescriptor GetSampleResourceHelper(llvm::SampleIntrinsic *inst);

  void interceptSamplePayloadCoalescing(llvm::SampleIntrinsic *inst, uint numPart,
                                        llvm::SmallVector<CVariable *, 4> &payload, bool &payloadCovered);

  // message emit functions
  void emitSimdLaneId(llvm::Instruction *inst);
  void emitSimdLaneIdReplicate(llvm::Instruction *inst);
  void emitSimdSize(llvm::Instruction *inst);
  void emitSimdShuffle(llvm::Instruction *inst);
  void emitSimdClusteredBroadcast(llvm::Instruction *inst);
  void emitCrossInstanceMov(const SSource &source, const DstModifier &modifier);
  void emitSimdShuffleDown(llvm::Instruction *inst);
  void emitSimdShuffleXor(llvm::Instruction *inst);
  void emitSimdBlockRead(llvm::Instruction *inst, llvm::Value *ptrVal = nullptr);
  void emitSimdBlockWrite(llvm::Instruction *inst, llvm::Value *ptrVal = nullptr);
  void emitLegacySimdBlockWrite(llvm::Instruction *inst, llvm::Value *ptrVal = nullptr);
  void emitLegacySimdBlockRead(llvm::Instruction *inst, llvm::Value *ptrVal = nullptr);
  void emitLSCSimdBlockWrite(llvm::Instruction *inst, llvm::Value *ptrVal = nullptr);
  void emitLSCSimdBlockRead(llvm::Instruction *inst, llvm::Value *ptrVal = nullptr);
  void emitSimdMediaBlockRead(llvm::Instruction *inst);
  void emitSimdMediaBlockWrite(llvm::Instruction *inst);
  void emitMediaBlockIO(const llvm::GenIntrinsicInst *inst, bool isRead);
  void emitMediaBlockRectangleRead(llvm::Instruction *inst);
  void emitSampleInstruction(llvm::SampleIntrinsic *inst);
  void emitLdInstruction(llvm::Instruction *inst);
  void emitInfoInstruction(llvm::InfoIntrinsic *inst);
  void emitGather4Instruction(llvm::SamplerGatherIntrinsic *inst);
  void emitLdmsInstruction(llvm::Instruction *inst);
  void emitTypedRead(llvm::Instruction *inst);
  void emitTypedWrite(llvm::Instruction *inst);
  void emitThreadGroupBarrier(llvm::Instruction *inst);
  void emitThreadGroupNamedBarriersSignal(llvm::Instruction *inst);
  void emitThreadGroupNamedBarriersWait(llvm::Instruction *inst);
  void emitLSCTypedRead(llvm::Instruction *inst);
  void emitLSCTypedWrite(llvm::Instruction *inst);
  void emitLSCAtomicTyped(llvm::GenIntrinsicInst *inst);
  void emitLscUniformAtomicCounter(llvm::GenIntrinsicInst *pInst);
  void emitCastSelect(CVariable *flag, CVariable *dst, CVariable *src0, CVariable *src1);
  void emitMemoryFence(llvm::Instruction *inst);
  void emitMemoryFence(void);
  void emitTypedMemoryFence(llvm::Instruction *inst);
  void emitFlushSamplerCache();
  void emitSurfaceInfo(llvm::GenIntrinsicInst *intrinsic);

  void emitStackAlloca(llvm::GenIntrinsicInst *intrinsic);
  void emitVLAStackAlloca(llvm::GenIntrinsicInst *intrinsic);

  void emitUAVSerialize();

  void emitScalarAtomics(llvm::Instruction *pInst, ResourceDescriptor &resource, AtomicOp atomic_op,
                         CVariable *pUniformBaseAddr, CVariable *pDstAddr, CVariable *pU, CVariable *pV, CVariable *pR,
                         CVariable *pSrc, bool isA64, int bitSize, int immOffset, int immScale, LSC_ADDR_SIZE addrSize);

  void emitScalarAtomicLoad(llvm::Instruction *pInst, ResourceDescriptor &resource, CVariable *pUniformBase,
                            CVariable *pDstAddr, CVariable *pU, CVariable *pV, CVariable *pR, CVariable *pSrc,
                            bool isA64, int bitWidth, int immOffset, int immScale, LSC_ADDR_SIZE addrSize);

  /// wave/subgroup support
  /// reduction and prefix/postfix facilities
  static bool ScanReduceIs64BitType(VISA_Type type);
  static bool ScanReduceIsInt64Mul(e_opcode op, VISA_Type type);
  bool ScanReduceIsInt64EmulationNeeded(e_opcode op, VISA_Type type);
  CVariable *ScanReducePrepareSrc(VISA_Type type, uint64_t identityValue, bool negate, bool secondHalf, CVariable *src,
                                  CVariable *dst, CVariable *flag = nullptr);
  CVariable *ReductionReduceHelper(e_opcode op, VISA_Type type, SIMDMode simd, CVariable *src,
                                   CVariable *srcSecondHalf = nullptr);
  void ReductionExpandHelper(e_opcode op, VISA_Type type, CVariable *src, CVariable *dst);
  void ReductionClusteredSrcHelper(CVariable *(&pSrc)[2], CVariable *src, uint16_t numLanes, VISA_Type type,
                                   uint numInst, bool secondHalf);
  CVariable *ReductionClusteredReduceHelper(e_opcode op, VISA_Type type, SIMDMode simd, bool secondHalf, CVariable *src,
                                            CVariable *dst);
  void ReductionClusteredExpandHelper(e_opcode op, VISA_Type type, SIMDMode simd, const uint clusterSize,
                                      bool secondHalf, CVariable *src, CVariable *dst);
  /// reduction and prefix/postfix emitters
  void emitReductionAll(e_opcode op, uint64_t identityValue, VISA_Type type, bool negate, CVariable *src,
                        CVariable *dst);
  void emitReductionTree(e_opcode op, VISA_Type type, CVariable *src, CVariable *dst);
  void emitReductionTrees(e_opcode op, VISA_Type type, SIMDMode simdMode, CVariable *src, CVariable *dst,
                          unsigned int startIdx, unsigned int endIdx);
  void emitReductionClustered(const e_opcode op, const uint64_t identityValue, const VISA_Type type, const bool negate,
                              const unsigned int clusterSize, CVariable *const src, CVariable *const dst);
  void emitReductionInterleave(const e_opcode op, const uint64_t identityValue, const VISA_Type type, const bool negate,
                               const unsigned int step, CVariable *const src, CVariable *const dst);
  void emitReductionInterleave(const e_opcode op, const VISA_Type type, const SIMDMode simd, const unsigned int step,
                               const bool noMaskBroadcast, CVariable *const src1, CVariable *const src2,
                               CVariable *const dst);
  void emitReductionClusteredInterleave(const e_opcode op, const uint64_t identityValue, const VISA_Type type,
                                        const bool negate, const unsigned int clusterSize,
                                        const unsigned int interleaveStep, CVariable *const src, CVariable *const dst);
  void emitPreOrPostFixOp(e_opcode op, uint64_t identityValue, VISA_Type type, bool negateSrc, CVariable *src,
                          CVariable *result[2], CVariable *Flag = nullptr, bool isPrefix = false, bool isQuad = false,
                          int clusterSize = 0);
  void emitPreOrPostFixOpScalar(e_opcode op, uint64_t identityValue, VISA_Type type, bool negateSrc, CVariable *src,
                                CVariable *result[2], CVariable *Flag, bool isPrefix, int clusterSize = 0);

  bool IsUniformAtomic(llvm::Instruction *pInst);
  void emitAtomicRaw(llvm::GenIntrinsicInst *pInst, Value *varOffset, ConstantInt *immOffset = nullptr,
                     ConstantInt *immScale = nullptr, Value *uniformBase = nullptr, bool signExtendOffset = false,
                     bool zeroExtendOffset = false);
  void emitAtomicTyped(llvm::GenIntrinsicInst *pInst);
  void emitAtomicCounter(llvm::GenIntrinsicInst *pInst);
  void emitFastClear(llvm::LoadInst *inst);
  void emitFastClearSend(llvm::Instruction *pInst);
  void setRovCacheCtrl(llvm::GenIntrinsicInst *inst);
  std::optional<LSC_CACHE_OPTS> cacheOptionsForConstantBufferLoads(Instruction *inst, LSC_L1_L3_CC Ctrl) const;
  std::optional<LSC_CACHE_OPTS> cacheOptionsForConstantBufferLoads(Instruction *inst) const;
  bool useRasterizerOrderedByteAddressBuffer(llvm::GenIntrinsicInst *inst);
  void emitUniformAtomicCounter(llvm::GenIntrinsicInst *pInst);

  void emitDiscard(llvm::Instruction *inst);

  void emitcycleCounter(llvm::Instruction *inst);
  void emitSetDebugReg(llvm::Instruction *inst);
  void emitInsert(llvm::Instruction *inst);
  void emitExtract(llvm::Instruction *inst);
  void emitBitCast(llvm::BitCastInst *btCst);
  void emitBitcastfromstruct(llvm::GenIntrinsicInst *BCFromStruct);
  void emitBitcasttostruct(llvm::GenIntrinsicInst *BCToStruct);
  void emitPtrToInt(llvm::PtrToIntInst *p2iCst);
  void emitIntToPtr(llvm::IntToPtrInst *i2pCst);
  void emitAddrSpaceCast(llvm::AddrSpaceCastInst *addrSpaceCast);
  void emitBranch(llvm::BranchInst *br, const SSource &cond, e_predMode predMode);
  void emitDiscardBranch(llvm::BranchInst *br, const SSource &cond);
  void emitAluNoModifier(llvm::GenIntrinsicInst *inst);

  CVariable *GetVMaskPred(CVariable *&predicate);
  void createVMaskPred(CVariable *&predicate);
  void UseVMaskPred();
  CVariable *GetCombinedVMaskPred(CVariable *basePredicate = nullptr);
  CVariable *m_vMaskPredForSubplane = nullptr;

  void emitGradientX(const SSource &source, const DstModifier &modifier);
  void emitGradientY(const SSource &source, const DstModifier &modifier);
  void emitGradientXFine(const SSource &source, const DstModifier &modifier);
  void emitGradientYFine(const SSource &source, const DstModifier &modifier);

  void emitf32tof16_rtz(llvm::GenIntrinsicInst *inst);
  void emitfitof(llvm::GenIntrinsicInst *inst);
  void emitFPOWithNonDefaultRoundingMode(llvm::GenIntrinsicInst *inst);
  void emitftoi(llvm::GenIntrinsicInst *inst);
  void emitCtlz(const SSource &source);

  void emitBfn(llvm::GenIntrinsicInst *inst);


  // VME
  void emitVMESendIME(llvm::GenIntrinsicInst *inst);
  void emitVMESendFBR(llvm::GenIntrinsicInst *inst);
  void emitVMESendSIC(llvm::GenIntrinsicInst *inst);
  void emitVMESendIME2(llvm::GenIntrinsicInst *inst);
  void emitVMESendFBR2(llvm::GenIntrinsicInst *inst);
  void emitVMESendSIC2(llvm::GenIntrinsicInst *inst);
  void emitCreateMessagePhases(llvm::GenIntrinsicInst *inst);
  void emitSetMessagePhaseX_legacy(llvm::GenIntrinsicInst *inst);
  void emitSetMessagePhase_legacy(llvm::GenIntrinsicInst *inst);
  void emitGetMessagePhaseX(llvm::GenIntrinsicInst *inst);
  void emitSetMessagePhaseX(llvm::GenIntrinsicInst *inst);
  void emitGetMessagePhase(llvm::GenIntrinsicInst *inst);
  void emitSetMessagePhase(llvm::GenIntrinsicInst *inst);
  void emitSimdGetMessagePhase(llvm::GenIntrinsicInst *inst);
  void emitBroadcastMessagePhase(llvm::GenIntrinsicInst *inst);
  void emitSimdSetMessagePhase(llvm::GenIntrinsicInst *inst);
  void emitSimdMediaRegionCopy(llvm::GenIntrinsicInst *inst);
  void emitExtractMVAndSAD(llvm::GenIntrinsicInst *inst);
  void emitCmpSADs(llvm::GenIntrinsicInst *inst);

  // VA
  void emitVideoAnalyticSLM(llvm::GenIntrinsicInst *inst, const DWORD responseLen);
  // New VA without using SLM and barriers (result is returned in GRF).
  void emitVideoAnalyticGRF(llvm::GenIntrinsicInst *inst, const DWORD responseLen);

  // CrossLane Instructions
  void emitWaveBallot(llvm::GenIntrinsicInst *inst);
  void emitWaveClusteredBallot(llvm::GenIntrinsicInst *inst);
  void emitBallotUniform(llvm::GenIntrinsicInst *inst, CVariable **destination, bool disableHelperLanes);
  void emitWaveInverseBallot(llvm::GenIntrinsicInst *inst);
  void emitWaveShuffleIndex(llvm::GenIntrinsicInst *inst);
  void emitWavePrefix(llvm::WavePrefixIntrinsic *I);
  void emitQuadPrefix(llvm::QuadPrefixIntrinsic *I);
  void emitWaveClusteredPrefix(llvm::GenIntrinsicInst *I);
  void emitWaveAll(llvm::GenIntrinsicInst *inst);
  void emitWaveClustered(llvm::GenIntrinsicInst *inst);
  void emitWaveInterleave(llvm::GenIntrinsicInst *inst);
  void emitWaveClusteredInterleave(llvm::GenIntrinsicInst *inst);

  // Those three "vector" version shall be combined with
  // non-vector version.
  bool isUniformStoreOCL(llvm::StoreInst *SI);
  bool isUniformStoreOCL(llvm::Value *ptr, llvm::Value *storeVal);
  void emitVectorBitCast(llvm::BitCastInst *BCI);
  void emitVectorLoad(llvm::LoadInst *LI, llvm::Value *offset, llvm::ConstantInt *immOffset);
  void emitVectorStore(llvm::StoreInst *SI, llvm::Value *offset, llvm::ConstantInt *immOffset);
  void emitLSCVectorLoad(llvm::Instruction *Inst, llvm::Value *Ptr, llvm::Value *uniformBase, llvm::Value *offset,
                         llvm::ConstantInt *immOffset, ConstantInt *immScale, LSC_CACHE_OPTS cacheOpts,
                         LSC_DOC_ADDR_SPACE addrSpace, bool signExtendOffset, bool zeroExtendOffset);
  void emitLSCVectorStore(llvm::Value *Ptr, Value *uniformBase, llvm::Value *offset, llvm::ConstantInt *immOffset,
                          llvm::ConstantInt *immScale, llvm::Value *storedVal, llvm::BasicBlock *BB,
                          LSC_CACHE_OPTS cacheOpts, alignment_t align, bool dontForceDMask,
                          LSC_DOC_ADDR_SPACE addrSpace, bool signExtendOffset, bool zeroExtendOffset,
                          llvm::Value *predicate = nullptr);
  void emitUniformVectorCopy(CVariable *Dst, CVariable *Src, uint32_t nElts, uint32_t DstSubRegOffset = 0,
                             uint32_t SrcSubRegOffset = 0, bool allowLargerSIMDSize = false,
                             CVariable *predicate = nullptr);
  void emitVectorCopy(CVariable *Dst, CVariable *Src, uint32_t nElts, uint32_t DstSubRegOffset = 0,
                      uint32_t SrcSubRegOffset = 0, bool allowLargerSIMDSize = false, CVariable *predicate = nullptr);
  void emitConstantVector(CVariable *Dst, uint64_t value = 0);
  void emitCopyAll(CVariable *Dst, CVariable *Src, llvm::Type *Ty);
  void emitCopyAllInstances(CVariable *Dst, CVariable *Src, llvm::Type *Ty);

  void emitPredicatedVectorCopy(CVariable *Dst, CVariable *Src, CVariable *pred);
  void emitPredicatedVectorSelect(CVariable *Dst, CVariable *Src0, CVariable *Src1, CVariable *pred);

  void emitPushFrameToStack(Function *ParentFunction, unsigned &pushSize);
  // emitMul64 - emulate 64bit multiply by 32-bit operations.
  // Dst must be a 64-bit type variable.
  // Src0 and Src1 must be in 32-bit type variable/immediate
  void emitMul64_UDxUD(CVariable *Dst, CVariable *Src0, CVariable *Src1);
  void emitAddPointer(CVariable *Dst, CVariable *Src, CVariable *offset);
  // emitAddPair - emulate 64bit addtition by 32-bit operations.
  // Dst and Src0 must be a 64-bit type variable.
  // Src1 must be in 32/64-bit type variable/immediate
  void emitAddPair(CVariable *Dst, CVariable *Src0, CVariable *Src1);

  void emitSqrt(llvm::Instruction *inst);
  void emitUmin(llvm::IntrinsicInst *inst);
  void emitSmin(llvm::IntrinsicInst *inst);
  void emitUmax(llvm::IntrinsicInst *inst);
  void emitSmax(llvm::IntrinsicInst *inst);
  void emitCanonicalize(llvm::Instruction *inst, const DstModifier &modifier);
  void emitRsq(llvm::Instruction *inst);
  void emitFrc(llvm::GenIntrinsicInst *inst);

  void emitLLVMbswap(llvm::IntrinsicInst *inst);
  void emitDP4A(llvm::GenIntrinsicInst *GII, const SSource *source = nullptr,
                const DstModifier &modifier = DstModifier(), bool isAccSigned = true);

  void emitLLVMStackSave(llvm::IntrinsicInst *inst);
  void emitLLVMStackRestore(llvm::IntrinsicInst *inst);

  void emitUnmaskedRegionBoundary(bool start);
  LSC_CACHE_OPTS getDefaultRaytracingCachePolicy(bool isLoad) const;
  void emitAsyncStackID(llvm::GenIntrinsicInst *I);
  void emitTraceRay(llvm::TraceRayIntrinsic *I, bool RayQueryEnable);

  void emitTraceRayEff64(TraceRayIntrinsic *instruction, bool rayQueryEnable);
  void emitRayQueryCheckReleaseEff64(llvm::GenIntrinsicInst *instruction, bool rayQueryCheckEnable = false,
                                     bool rayQueryReleaseEnable = false);

  void emitReadTraceRaySync(llvm::GenIntrinsicInst *I);

  void emitRayQueryCheckRelease(llvm::GenIntrinsicInst *I, bool RayQueryCheckEnable = false,
                                bool RayQueryReleaseEnable = false);

  void emitPreemptionDisable(llvm::PreemptionDisableIntrinsic *PDI);
  void emitPreemptionEnable(llvm::PreemptionEnableIntrinsic *PEI);

  enum PreemptionEncoding { PreemptionDisabled = (0 << 14), PreemptionEnabled = (1 << 14) };

  static PreemptionEncoding getEncoderPreemptionMode(EPreemptionMode preemptionMode);

  void emitBTD(CVariable *GlobalBufferPtr, CVariable *StackID, CVariable *ShaderRecord, CVariable *Flag,
               bool releaseStackID);

  void emitBTDEff64(CVariable *globalBufferPtr, CVariable *stackID, CVariable *shaderRecord, CVariable *flag,
                    bool releaseStackID);

  void emitBindlessThreadDispatch(llvm::BTDIntrinsic *I);
  void emitStackIDRelease(llvm::StackIDReleaseIntrinsic *I);
  void emitGetShaderRecordPtr(llvm::GetShaderRecordPtrIntrinsic *I);
  void emitGlobalBufferPtr(llvm::GenIntrinsicInst *I);
  void emitLocalBufferPtr(llvm::GenIntrinsicInst *I);
  void emitKSPPointer(llvm::KSPPointerIntrinsic *KPI);
  void emitInlinedDataValue(llvm::GenIntrinsicInst *I);
  void emitBdpas(llvm::GenIntrinsicInst *GII);
  void emitDpas(llvm::GenIntrinsicInst *GII, const SSource *source, const DstModifier &modifier);
  void emitfcvt(llvm::GenIntrinsicInst *GII);
  void emitLfsr(llvm::GenIntrinsicInst *GII);
  void emitMaxReduce(llvm::GenIntrinsicInst *GII);

  void emitSystemMemoryFence(llvm::GenIntrinsicInst *I);
  void emitUrbFence();
  void emitHDCuncompressedwrite(llvm::GenIntrinsicInst *I);
  ////////////////////////////////////////////////////////////////////
  // LSC related functions
  bool tryOverrideCacheOpts(LSC_CACHE_OPTS &cacheOpts, bool isLoad, bool isTGM, const llvm::Value *warningContextValue,
                            CacheControlOverride m_CacheControlOption) const;
  LSC_CACHE_OPTS translateLSCCacheControlsEnum(LSC_L1_L3_CC l1l3cc, bool isLoad,
                                               const llvm::Value *warningContextValue) const;
  LSC_CACHE_OPTS translateLSCCacheControlsFromValue(llvm::Value *value, bool isLoad) const;
  LSC_CACHE_OPTS translateLSCCacheControlsFromMetadata(llvm::Instruction *inst, bool isLoad, bool isTGM = 0) const;
  struct LscMessageFragmentInfo {
    LSC_DATA_ELEMS fragElem;
    int fragElemCount;
    int addrOffsetDelta;
    int grfOffsetDelta;
    bool lastIsV1; // e.g. splitting a V3 up is a V2 + V1
  };
  LscMessageFragmentInfo checkForLscMessageFragmentation(LSC_DATA_SIZE size, LSC_DATA_ELEMS elems) const;

  // (CVariable* gatherDst, int fragIx, LSC_DATA_ELEMS fragElems, int fragImmOffset)
  using LscIntrinsicFragmentEmitter = std::function<void(CVariable *, int, LSC_DATA_ELEMS, int)>;

  void emitLscIntrinsicFragments(CVariable *gatherDst, LSC_DATA_SIZE dataSize, LSC_DATA_ELEMS dataElems,
                                 int immOffsetBytes, const LscIntrinsicFragmentEmitter &emitter);

  void emitLscIntrinsicLoad(llvm::GenIntrinsicInst *GII);
  void emitLscIntrinsicPrefetch(llvm::GenIntrinsicInst *GII);
  void emitLscIntrinsicTypedLoadStatus(llvm::GenIntrinsicInst *GII);
  void emitLscSimdBlockPrefetch(llvm::GenIntrinsicInst *GII);
  void emitLscIntrinsicStore(llvm::GenIntrinsicInst *GII);
  void emitLscIntrinsicLoadCmask(llvm::GenIntrinsicInst *inst);
  void emitLscIntrinsicStoreCmask(llvm::GenIntrinsicInst *GII);

  void emitLSCFence(llvm::GenIntrinsicInst *inst);
  void emitLSC2DBlockOperation(llvm::GenIntrinsicInst *inst);
  void emitLSC2DBlockAddrPayload(llvm::GenIntrinsicInst *GII);
  void emitLSC2DBlockReadWriteWithAddrPayload(llvm::GenIntrinsicInst *GII);
  void emitLSC2DBlockSetAddrPayloadField(llvm::GenIntrinsicInst *GII);

  void emitLSCAtomic(llvm::GenIntrinsicInst *inst);
  void emitLSCIntrinsic(llvm::GenIntrinsicInst *GII);
  void emitLSCLoad(llvm::Instruction *inst, CVariable *dst, CVariable *uniformBase, CVariable *offset,
                   unsigned elemSize, unsigned numElems, unsigned blockOffset, ResourceDescriptor *resource,
                   LSC_ADDR_SIZE addr_size, LSC_DATA_ORDER data_order, int immOffset, int immScale);
  void emitLSCLoad(LSC_CACHE_OPTS cacheOpts, CVariable *dst, CVariable *uniformBase, CVariable *offset,
                   unsigned elemSize, unsigned numElems, unsigned blockOffset, ResourceDescriptor *resource,
                   LSC_ADDR_SIZE addr_size, LSC_DATA_ORDER data_order, int immOffset, int immScale,
                   LSC_DOC_ADDR_SPACE addrSpace);
  void emitLSCLoad(llvm::Instruction *inst, CVariable *dst, CVariable *offset, unsigned elemSize, unsigned numElems,
                   unsigned blockOffset, ResourceDescriptor *resource, LSC_ADDR_SIZE addr_size,
                   LSC_DATA_ORDER data_order, int immOffset, int immScale);
  void emitLSCLoad(LSC_CACHE_OPTS cacheOpts, CVariable *dst, CVariable *offset, unsigned elemSize, unsigned numElems,
                   unsigned blockOffset, ResourceDescriptor *resource, LSC_ADDR_SIZE addr_size,
                   LSC_DATA_ORDER data_order, int immOffset, int immScale, LSC_DOC_ADDR_SPACE addrSpace);
  void emitLSCStore(llvm::Instruction *inst, CVariable *uniformBase, CVariable *src, CVariable *offset,
                    unsigned elemSize, unsigned numElems, unsigned blockOffset, ResourceDescriptor *resource,
                    LSC_ADDR_SIZE addr_size, LSC_DATA_ORDER data_order, int immOffset, int immScale);
  void emitLSCStore(LSC_CACHE_OPTS cacheOpts, CVariable *uniformBase, CVariable *src, CVariable *offset,
                    unsigned elemSize, unsigned numElems, unsigned blockOffset, ResourceDescriptor *resource,
                    LSC_ADDR_SIZE addr_size, LSC_DATA_ORDER data_order, int immOffset, int immScale,
                    LSC_DOC_ADDR_SPACE addrSpace);
  void emitLSCStore(llvm::Instruction *inst, CVariable *src, CVariable *offset, unsigned elemSize, unsigned numElems,
                    unsigned blockOffset, ResourceDescriptor *resource, LSC_ADDR_SIZE addr_size,
                    LSC_DATA_ORDER data_order, int immOffset, int immScale);
  void emitLSCStore(LSC_CACHE_OPTS cacheOpts, CVariable *src, CVariable *offset, unsigned elemSize, unsigned numElems,
                    unsigned blockOffset, ResourceDescriptor *resource, LSC_ADDR_SIZE addr_size,
                    LSC_DATA_ORDER data_order, int immOffset, int immScale, LSC_DOC_ADDR_SPACE addrSpace);
  ////////////////////////////////////////////////////////////////////
  // NOTE: for vector load/stores instructions pass the
  // optional instruction argument checks additional constraints
  static Tristate shouldGenerateLSCQuery(const CodeGenContext &Ctx, llvm::Instruction *vectorLdStInst = nullptr,
                                         SIMDMode Mode = SIMDMode::UNKNOWN);
  bool shouldGenerateLSC(llvm::Instruction *vectorLdStInst = nullptr, bool isTGM = false);
  bool forceCacheCtrl(llvm::Instruction *vectorLdStInst = nullptr);
  uint32_t totalBytesToStoreOrLoad(llvm::Instruction *vectorLdStInst);
  void emitSrnd(llvm::GenIntrinsicInst *GII);
  void emitDnscl(llvm::GenIntrinsicInst *GII);
  void emitInt4VectorUnpack(llvm::GenIntrinsicInst *GII);
  void emitInt4VectorPack(llvm::GenIntrinsicInst *GII);
  void emitStaticConstantPatchValue(llvm::StaticConstantPatchIntrinsic *staticConstantPatch32);
  // Debug Built-Ins
  void emitStateRegID(uint32_t BitStart, uint32_t BitEnd);
  void emitThreadPause(llvm::GenIntrinsicInst *inst);

  void MovPhiSources(llvm::BasicBlock *bb);

  void InitConstant(llvm::BasicBlock *BB);
  void emitLifetimeStartResourceLoopUnroll(llvm::BasicBlock *BB);
  void emitLifetimeStartAtEndOfBB(llvm::BasicBlock *BB);
  void emitDebugPlaceholder(llvm::GenIntrinsicInst *I);
  void emitDummyInst(llvm::GenIntrinsicInst *GII);
  void emitLaunder(llvm::GenIntrinsicInst *GII);
  void emitImplicitArgIntrinsic(llvm::GenIntrinsicInst *I);
  void emitStoreImplBufferPtr(llvm::GenIntrinsicInst *I);
  void emitSetStackCallsBaseAddress(llvm::GenIntrinsicInst *I);
  void emitSaveInReservedArgSpace(llvm::SaveInReservedArgSpaceIntrinsic *I);
  void emitReadFromReservedArgSpace(llvm::ReadFromReservedArgSpaceIntrinsic *I);
  void emitStoreLocalIdBufferPtr(llvm::GenIntrinsicInst *I);
  void emitStoreGlobalBufferArg(llvm::GenIntrinsicInst *I);
  void emitLoadImplBufferPtr(llvm::GenIntrinsicInst *I);
  void emitLoadLocalIdBufferPtr(llvm::GenIntrinsicInst *I);
  void emitLoadGlobalBufferArg(llvm::GenIntrinsicInst *I);
  void emitLoadKABPKernelArg(llvm::GenIntrinsicInst *I);
  void emitLoadgetTileID(llvm::GenIntrinsicInst *I);
  void emitLoadgetEngineID(llvm::GenIntrinsicInst *I);

  void emitMayUnalignedVectorCopy(CVariable *D, uint32_t D_off, CVariable *S, uint32_t S_off, llvm::Type *Ty);
  Function *findStackOverflowDetectionFunction(Function *ParentFunction, bool FindInitFunction);
  void emitStackOverflowDetectionCall(llvm::Function *ParentFunction, bool EmitInitFunction);

  std::pair<llvm::Value *, llvm::Value *> getPairOutput(llvm::Value *) const;

  // helper function
  void SplitSIMD(llvm::Instruction *inst, uint numSources, uint headerSize, CVariable *payload, SIMDMode mode,
                 uint half);
  template <size_t N> void JoinSIMD(CVariable *(&tempdst)[N], uint responseLength, SIMDMode mode);
  CVariable *BroadcastIfUniform(CVariable *pVar, bool nomask = false);
  bool IsNoMaskAllowed(llvm::Instruction *inst);
  bool IsSubspanDestination(llvm::Instruction *inst);
  uint DecideInstanceAndSlice(const llvm::BasicBlock &blk, SDAG &sdag, bool &slicing);
  bool IsUndefOrZeroImmediate(const llvm::Value *value);
  inline bool isUndefOrConstInt0(const llvm::Value *val) {
    if (val == nullptr || llvm::isa<llvm::UndefValue>(val) ||
        (llvm::isa<llvm::ConstantInt>(val) && llvm::cast<llvm::ConstantInt>(val)->getZExtValue() == 0)) {
      return true;
    }
    return false;
  }
  inline llvm::Value *getOperandIfExist(llvm::Instruction *pInst, unsigned op) {
    if (llvm::CallInst *pCall = llvm::dyn_cast<llvm::CallInst>(pInst)) {
      if (op < IGCLLVM::getNumArgOperands(pCall)) {
        return pInst->getOperand(op);
      }
    }
    return nullptr;
  }

  bool IsGRFAligned(CVariable *pVar, e_alignment requiredAlign) const {
    e_alignment align = pVar->GetAlign();
    if (requiredAlign == EALIGN_BYTE) {
      // trivial
      return true;
    }
    if (requiredAlign == EALIGN_AUTO || align == EALIGN_AUTO) {
      // Can only assume that AUTO only matches AUTO (?)
      // (keep the previous behavior unchanged.)
      return align == requiredAlign;
    }
    return align >= requiredAlign;
  }

  CVariable *ExtendVariable(CVariable *pVar, e_alignment uniformAlign);
  CVariable *BroadcastAndExtend(CVariable *pVar);
  CVariable *TruncatePointer(CVariable *pVar, bool TruncBothHalves = false);
  CVariable *ReAlignUniformVariable(CVariable *pVar, e_alignment align);
  CVariable *ReAlignOrBroadcastUniformVarIfNotNull(bool destIsUniform, CVariable *pVar, e_alignment align,
                                                   bool nomask = false);
  CVariable *BroadcastAndTruncPointer(CVariable *pVar);
  CVariable *IndexableResourceIndex(CVariable *indexVar, uint btiIndex);
  ResourceDescriptor GetResourceVariable(llvm::Value *resourcePtr, bool Check = false);
  SamplerDescriptor GetSamplerVariable(llvm::Value *samplerPtr);
  CVariable *ComputeSampleIntOffset(llvm::Instruction *sample, uint sourceIndex);
  void emitPlnInterpolation(CVariable *bary, CVariable *inputvar);

  // the number of lanes of the entire dispatch. It is read only as it is cached for reuse.
  CVariable *GetNumActiveLanes();

  CVariable *CastFlagToVariable(CVariable *flag);
  CVariable *GetExecutionMask();
  CVariable *GetExecutionMask(CVariable *&vecMaskVar);
  CVariable *GetHalfExecutionMask();
  CVariable *UniformCopy(CVariable *var, bool doSub = false);
  CVariable *UniformCopy(CVariable *var, CVariable *&LaneOffset, CVariable *eMask = nullptr, bool doSub = false,
                         bool safeGuard = false, CVariable *predicate = nullptr);

  // generate loop header to process sample instruction with varying resource/sampler
  bool ResourceLoopHeader(const CVariable *destination, ResourceDescriptor &resource, SamplerDescriptor &sampler,
                          CVariable *&flag, uint &label, uint ResourceLoopMarker = 0, int *subInteration = nullptr);
  bool ResourceLoopHeader(const CVariable *destination, ResourceDescriptor &resource, CVariable *&flag, uint &label,
                          uint ResourceLoopMarker = 0, int *subInteration = nullptr);
  bool ResourceLoopSubIteration(ResourceDescriptor &resource, SamplerDescriptor &sampler, CVariable *&flag, uint &label,
                                uint ResourceLoopMarker = 0, int iteration = 0, CVariable *prevFlag = nullptr);
  bool ResourceLoopSubIteration(ResourceDescriptor &resource, CVariable *&flag, uint &label,
                                uint ResourceLoopMarker = 0, int iteration = 0, CVariable *prevFlag = nullptr);
  void ResourceLoopBackEdge(bool needLoop, CVariable *flag, uint label, uint ResourceLoopMarker = 0);
  bool ResourceLoopNeedsLoop(ResourceDescriptor &resource, SamplerDescriptor &sampler, CVariable *&flag,
                             uint ResourceLoopMarker);
  bool ResourceLoopNeedsLoop(ResourceDescriptor &resource, CVariable *&flag, uint ResourceLoopMarker);
  template <typename Func>
  void ResourceLoop(ResourceDescriptor &resource, SamplerDescriptor &sampler, const Func &Fn,
                    uint ResourceLoopMarker = 0) {
    uint label = 0;
    CVariable *flag = nullptr;

    // 0 - default (loop header is set up)
    // 1 - first unroll (no safe guard)
    // 2 - second unroll, and so on.
    int subInteration = 0;
    int iterations = m_pCtx->platform.hasSlowSameSBIDLoad() ? IGC_GET_FLAG_VALUE(ResourceLoopUnrollIteration) : 1;

    CVariable *currentDestination = m_destination;
    std::vector<std::pair<CVariable *, CVariable *>> fills;

    // check if need loop
    bool needLoop = ResourceLoopNeedsLoop(resource, sampler, flag, ResourceLoopMarker);

    std::vector<CVariable *> cumulativeFlags;

    // nested unroll won't need loop as the resources are uniformed
    if (needLoop) {
      // we init this before label;
      for (int iter = 0; iter < iterations - 1; iter++) {
        cumulativeFlags.push_back(m_currShader->ImmToVariable(0x0, ISA_TYPE_BOOL));
      }

      // label resource loop
      ResourceLoopHeader(currentDestination, resource, sampler, flag, label, ResourceLoopMarker, &subInteration);
    }

    // subInteration == 0 could mean no resource loop tag indicated
    // iterations == 1 could mean no subiteration unroll
    if (subInteration == 0 || iterations == 1) {
      // get exclusive load info from nested loop unroll meta data
      if (m_encoder->GetUniqueExclusiveLoad() && m_destination &&
          IGC_IS_FLAG_DISABLED(DisableResourceLoopUnrollExclusiveLoad)) {
        m_encoder->MarkAsExclusiveLoad(m_destination);
      }
      ResourceLoopSubIteration(resource, sampler, flag, label, ResourceLoopMarker);
      Fn(flag, m_destination, resource, needLoop);
    } else {
      // This will be sum of lanes that did something so exit loop
      CVariable *flagSumMask = m_currShader->ImmToVariable(0x0, ISA_TYPE_BOOL);

      // This will be used as remaining exec mask
      CVariable *flagExecMask = nullptr;
      // it's also the remaining exec mask but in dword (for fbl)
      CVariable *dwordPrevFlag = GetExecutionMask(flagExecMask);
      // save the original input resource, as resource will be used in Fn()
      ResourceDescriptor resourceOrig = resource;

      if ((iterations > 1) && IGC_IS_FLAG_DISABLED(DisableResourceLoopUnrollExclusiveLoad)) {
        m_encoder->MarkAsExclusiveLoad(currentDestination);
      }

      for (int iter = 0; iter < iterations; iter++, subInteration++) {
        CVariable *flagSameLaneFlag = nullptr;

        // Use original reource as ResourceLoopHeader needs non-uniform
        resource = resourceOrig;
        ResourceLoopSubIteration(resource, sampler, flagSameLaneFlag, label, ResourceLoopMarker, subInteration,
                                 dwordPrevFlag);

        // First iteration does not need to safeguard.
        if (iter > 0 && flagSameLaneFlag) {
          // We safeguard against case when all lanes were the same in first addr
          // like <10 10 10 10> -> we handled all in first iteration
          // so we want to zero other iterations, so we don't load 3 times the same
          m_encoder->SetNoMask();
          m_encoder->And(flagSameLaneFlag, flagSameLaneFlag, flagExecMask);
          m_encoder->Push();
        }

        // need a temp (iter > 0) to save the unroll dst result to avoid shared SBID
        Fn(flagSameLaneFlag, currentDestination, resource, needLoop);

        if (flagSameLaneFlag) {
          m_encoder->SetNoMask();
          // Sum lanes that did something (for correct goto at the end)
          m_encoder->Or(flagSumMask, flagSumMask, flagSameLaneFlag);
          m_encoder->Push();

          // Last iteration does not need this
          if ((iter < (iterations - 1)) && flagExecMask) {
            m_encoder->SetNoMask();
            // mask out handled lanes out of remaining ExecMask
            m_encoder->Xor(flagExecMask, flagExecMask, flagSameLaneFlag);
            m_encoder->Cast(dwordPrevFlag, flagExecMask);
            m_encoder->Push();
          }
        }
      }

      flag = flagSumMask;
    }

    ResourceLoopBackEdge(needLoop, flag, label, ResourceLoopMarker);
  }

  void ForceDMask(bool createJmpForDiscard = true);
  void ResetVMask(bool createJmpForDiscard = true);
  void setPredicateForDiscard(CVariable *pPredicate = nullptr);

  void PackSIMD8HFRet(CVariable *dst);
  unsigned int GetPrimitiveTypeSizeInRegisterInBits(const llvm::Type *Ty) const;
  unsigned int GetPrimitiveTypeSizeInRegister(const llvm::Type *Ty) const;
  unsigned int GetScalarTypeSizeInRegisterInBits(const llvm::Type *Ty) const;
  unsigned int GetScalarTypeSizeInRegister(const llvm::Type *Ty) const;

  /// return true if succeeds, false otherwise.
  bool setCurrentShader(llvm::Function *F);

  /// checks FunctionInfo metadata and returns highest register pressure in the func group
  /// metadata is stored so subroutines have combined pressure (theirs & calling context)
  unsigned int getMaxRegPressureInFunctionGroup(llvm::Function *F, const IGCMD::MetaDataUtils *pM);

  /// check if symbol table is needed
  bool isSymbolTableRequired(llvm::Function *F);

  // Arithmetic operations with constant folding
  // Src0 and Src1 are the input operands
  // DstPrototype is a prototype of the result of operation and may be used for cloning to a new variable
  // Return a variable with the result of the compute which may be one the the sources, an immediate or a variable
  CVariable *Mul(CVariable *Src0, CVariable *Src1, const CVariable *DstPrototype);
  CVariable *Add(CVariable *Src0, CVariable *Src1, const CVariable *DstPrototype);

  // temporary helper function
  CVariable *GetSymbol(llvm::Value *v) const;

  // Check if stateless indirect access is available
  // If yes, increase the counter, otherwise do nothing
  void CountStatelessIndirectAccess(llvm::Value *pointer, ResourceDescriptor resource);

  // An indirect access happens when GPU loads from an address that was not directly given as one of the kernel
  // arguments. It's usually a pointer loaded from memory pointed by a kernel argument. Otherwise the access is direct.
  bool IsIndirectAccess(llvm::Value *value);

  CVariable *GetSrcVariable(const SSource &source, bool fromConstPool = false);
  void SetSourceModifiers(unsigned int sourceIndex, const SSource &source) const;

  SBasicBlock *getCurrentBlock() const {
    // if m_currentBlock is not set to initial value then return current block
    return m_currentBlock == UINT32_MAX ? nullptr : &(m_pattern->m_blocks[m_currentBlock]);
  }

  CodeGenContext *m_pCtx = nullptr;
  CVariable *m_destination = nullptr;
  GenXFunctionGroupAnalysis *m_FGA = nullptr;
  CodeGenPatternMatch *m_pattern = nullptr;
  DeSSA *m_deSSA = nullptr;
  BlockCoalescing *m_blockCoalescing = nullptr;
  const SIMDMode m_SimdMode;
  const ShaderDispatchMode m_ShaderDispatchMode;
  CShaderProgram::KernelShaderMap &m_shaders;
  CShader *m_currShader;
  CEncoder *m_encoder;
  const llvm::DataLayout *m_DL = nullptr;
  CoalescingEngine *m_CE = nullptr;
  VariableReuseAnalysis *m_VRA = nullptr;
  ResourceLoopAnalysis *m_RLA = nullptr;
  CollectLoopCount *m_CLC = nullptr;
  ModuleMetaData *m_moduleMD = nullptr;
  bool m_canAbortOnSpill;
  PSSignature *const m_pSignature;
  llvm::DenseSet<llvm::Value *> m_alreadyInitializedPHI;

  // Debug info emitter
  IDebugEmitter *m_pDebugEmitter = nullptr;

  llvm::DominatorTree *m_pDT = nullptr;
  static char ID;
  inline void ContextSwitchPayloadSection(bool first = true);
  inline void ContextSwitchShaderBody(bool last = true);

private:
  uint m_labelForDMaskJmp = 0;

  llvm::DenseMap<llvm::Instruction *, bool> instrMap;

  // caching the number of instances for the current inst.
  int16_t m_currInstNumInstances = -1;
  inline void resetCurrInstNumInstances() { m_currInstNumInstances = -1; }
  inline void setCurrInstNumInstances(int16_t aV) { m_currInstNumInstances = aV; }
  inline int16_t getCurrInstNumInstances() const { return m_currInstNumInstances; }

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

  uint m_currentBlock = UINT32_MAX;

  bool m_currFuncHasSubroutine = false;

  bool m_canGenericPointToPrivate = false;
  bool m_canGenericPointToLocal = false;

  typedef struct {
    CVariable *var;
    CVariable *broadcastedVar[2];
    llvm::BasicBlock *BB;
  } ConstVectorStoreData;
  llvm::DenseMap<llvm::Constant *, ConstVectorStoreData> m_constantVectorStores;

  // Used to relocate phi-mov to different BB. phiMovToBB is the map from "fromBB"
  // to "toBB" (meaning to move phi-mov from "fromBB" to "toBB"). See MovPhiSources.
  llvm::DenseMap<llvm::BasicBlock *, llvm::BasicBlock *> phiMovToBB;

  // Used to check for the constraint types with the actual llvmIR params for inlineASM instructions
  bool validateInlineAsmConstraints(llvm::CallInst *inst, llvm::SmallVector<llvm::StringRef, 8> &constraints);

  void emitGetMessagePhaseType(llvm::GenIntrinsicInst *inst, VISA_Type type, uint32_t width);
  void emitSetMessagePhaseType(llvm::GenIntrinsicInst *inst, VISA_Type type);
  void emitSetMessagePhaseType_legacy(llvm::GenIntrinsicInst *inst, VISA_Type type);

  void emitScan(llvm::Value *Src, IGC::WaveOps Op, bool isInclusiveScan, llvm::Value *Mask, bool isQuad,
                bool noMask = false);

  // Cached per lane offset variables. This is a per basic block data
  // structure. For each entry, the first item is the scalar type size in
  // bytes, the second item is the corresponding symbol.
  llvm::SmallVector<std::pair<unsigned, CVariable *>, 4> PerLaneOffsetVars;

  // Helper function to reduce common code for emitting indirect address
  // computation.
  CVariable *getOrCreatePerLaneOffsetVariable(unsigned TypeSizeInBytes) {
    for (const auto &Item : PerLaneOffsetVars) {
      if (Item.first == TypeSizeInBytes) {
        IGC_ASSERT_MESSAGE(Item.second, "null variable");
        return Item.second;
      }
    }
    CVariable *Var = m_currShader->GetPerLaneOffsetsReg(TypeSizeInBytes);
    PerLaneOffsetVars.push_back(std::make_pair(TypeSizeInBytes, Var));
    return Var;
  }

  // If constant vector is stored and there is already var instance for it
  // try reusing it (if it was defined in the same basic block)
  // or create a new var instance and make it available for reusing in further stores
  CVariable *tryReusingConstVectorStoreData(llvm::Value *storedVal, llvm::BasicBlock *BB, bool isBroadcast);

  CVariable *tryReusingXYZWPayload(llvm::Value *storedVal, llvm::BasicBlock *BB, unsigned numElems, VISA_Type type,
                                   CVariable *pSrc_X, CVariable *pSrc_Y, CVariable *pSrc_Z, CVariable *pSrc_W,
                                   const unsigned int numEltGRF, bool transposeSIMD1 = false);

  // Emit code in slice starting from (reverse) iterator I. Return the
  // iterator to the next pattern to emit.
  SBasicBlock::reverse_iterator emitInSlice(SBasicBlock &block, SBasicBlock::reverse_iterator I);

  /**
   * Reuse SampleDescriptor for sampleID, so that we can pass it to
   * ResourceLoop to generate loop for non-uniform values.
   */
  inline SamplerDescriptor getSampleIDVariable(llvm::Value *sampleIdVar) {
    SamplerDescriptor sampler;
    sampler.m_sampler = GetSymbol(sampleIdVar);
    return sampler;
  }

  CVariable *UnpackOrBroadcastIfUniform(CVariable *pVar);

  int getGRFSize() const { return m_currShader->getGRFSize(); }

  void initDefaultRoundingMode();
  void SetRoundingMode_FP(ERoundingMode RM_FP);
  void SetRoundingMode_FPCvtInt(ERoundingMode RM_FPCvtInt);
  void ResetRoundingMode(llvm::Instruction *inst);

  // A64 load/store with HWA that make sure the offset hi part is the same per LS call
  // addrUnifrom: if the load/store address is uniform, we can skip A64 WA
  void emitGatherA64(llvm::Value *loadInst, CVariable *dst, CVariable *offset, unsigned elemSize, unsigned numElems,
                     bool addrUniform);
  void emitGather4A64(llvm::Value *loadInst, CVariable *dst, CVariable *offset, bool addrUniform);
  void emitScatterA64(CVariable *val, CVariable *offset, unsigned elementSize, unsigned numElems, bool addrUniform);
  void emitScatter4A64(CVariable *src, CVariable *offset, bool addrUniform);

  // Helper functions that create loop for above WA
  void A64LSLoopHead(CVariable *addr, CVariable *&curMask, CVariable *&lsPred, uint &label);
  void A64LSLoopTail(CVariable *curMask, CVariable *lsPred, uint label);

  // Helper function to check if A64 WA is required
  bool hasA64WAEnable() const;

  bool shouldForceEarlyRecompile(IGCMD::MetaDataUtils *pMdUtils, llvm::Function *F);

  bool isHalfGRFReturn(CVariable *dst, SIMDMode simdMode);

  void emitFeedbackEnable();

  void emitAddrSpaceToGenericCast(llvm::AddrSpaceCastInst *addrSpaceCast, CVariable *srcV, unsigned tag);

  // used for loading/storing uniform value using scatter/gather messages.
  CVariable *prepareAddressForUniform(CVariable *AddrVar, uint32_t EltBytes, uint32_t NElts, uint32_t ExecSz,
                                      e_alignment Align);
  CVariable *prepareDataForUniform(CVariable *DataVar, uint32_t ExecSz, e_alignment Align);
  // sub-function of vector load/store
  void emitLSCVectorLoad_subDW(LSC_CACHE_OPTS CacheOpts, bool UseA32, ResourceDescriptor &Resource, CVariable *Dest,
                               CVariable *UniformBaseVar, CVariable *Offset, int ImmOffset, int ImmScale,
                               uint32_t NumElts, uint32_t EltBytes, LSC_DOC_ADDR_SPACE AddrSpace,
                               LSC_ADDR_SIZE AddrSize, CVariable *inputPredicate = nullptr,
                               CVariable *mergeVal = nullptr);
  void emitLSCVectorLoad_uniform(LSC_CACHE_OPTS CacheOpts, bool UseA32, ResourceDescriptor &Resource, CVariable *Dest,
                                 CVariable *UniformBaseVar, CVariable *Offset, int ImmOffset, int ImmScale,
                                 uint32_t NumElts, uint32_t EltBytes, uint64_t Align, uint32_t Addrspace,
                                 LSC_DOC_ADDR_SPACE UserAddrSpace, LSC_ADDR_SIZE AddrSize,
                                 CVariable *inputPredicate = nullptr, CVariable *mergeVal = nullptr);
  void emitLSCVectorStore_subDW(LSC_CACHE_OPTS CacheOpts, bool UseA32, ResourceDescriptor &Resource,
                                CVariable *UniformBaseVar, CVariable *StoreVar, CVariable *Offset, int ImmOffset,
                                int ImmScale, uint32_t NumElts, uint32_t EltBytes, alignment_t Align,
                                LSC_DOC_ADDR_SPACE AddrSpace, LSC_ADDR_SIZE AddrSize, llvm::Value *predicate = nullptr);
  void emitLSCVectorStore_uniform(LSC_CACHE_OPTS CacheOpts, bool UseA32, ResourceDescriptor &Resource,
                                  CVariable *UniformBaseVar, CVariable *StoreVar, CVariable *Offset, int ImmOffset,
                                  int ImmScale, uint32_t NumElts, uint32_t EltBytes, alignment_t Align,
                                  LSC_DOC_ADDR_SPACE AddrSpace, LSC_ADDR_SIZE AddrSize,
                                  llvm::Value *predicate = nullptr);
  LSC_FENCE_OP getLSCMemoryFenceOp(bool IsGlobalMemFence, bool InvalidateL1, bool EvictL1) const;

  CVariable *getStackSizePerThread(llvm::Function *parentFunc);
  uint32_t getReqBlkBitsForBlockStLd(llvm::CallInst *call);

  bool m_isDuplicate;
  CVariable *m_tmpDest = nullptr;
  std::set<CoalescingEngine::CCTuple *> lifetimeStartAdded;
  tuple<CVariable *, CVariable *, CVariable *> addToCachedPayloadUVR(CVariable *pU, CVariable *pV, CVariable *pR);
  std::map<tuple<Value *, Value *, Value *>, tuple<CVariable *, CVariable *, CVariable *>> atomic_shared_pUVR;
};

} // namespace IGC
