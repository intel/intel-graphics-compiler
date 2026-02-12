/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "RTStackFormat.h"
#include "RTStackReflectionIRBG/RTStackReflectionBuilder.h"
#include "RenderSurfaceStateIRBG/RenderSurfaceStateBuilder.h"
#include "common/IGCIRBuilder.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CodeGenPublic.h"
#include "visa_igc_common_header.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Module.h"
#include <optional>
#include "Probe/Assertion.h"


namespace llvm {

class RTBuilder : public IGCIRBuilder<>,
                  public RTStackReflectionBuilder<RTBuilder>,
                  public RenderSurfaceStateBuilder<RTBuilder> {
  // Allow the CRTP base class to access private members
  friend class RTStackReflectionBuilder<RTBuilder>;

public:
  const IGC::CodeGenContext &getCtx() const { return Ctx; }
  unsigned getSurfaceStateSize() const { return Ctx.m_DriverInfo.getSurfaceStateSize(); }
  // Here are more specialized values that enforce more type safety
  // between the accessor methods of RTBuilder at the C++ level to avoid
  // passing the wrong values in.

  class PtrVal : public Value {};
  class StackPointerVal : public PtrVal {};
  class SyncStackPointerVal : public StackPointerVal {};

  static_assert(sizeof(PtrVal) == sizeof(Value), "added something?");

  enum RTMemoryAccessMode {
    STATELESS,
    STATEFUL,
  };

  static uint32_t getSWStackStatefulAddrSpace(const IGC::ModuleMetaData &MMD);
  static uint32_t getRTSyncStackStatefulAddrSpace(const IGC::ModuleMetaData &MMD);

private:
  const IGC::CodeGenContext &Ctx;
  const GT_SYSTEM_INFO SysInfo;

  uint32_t NumDSSPerSlice = 0;
  uint32_t EuCountPerDSS = 0;
  uint32_t MaxDualSubSlicesSupported = 0;

  // TODO: this is hardcoded string, we might want to put all "printf" of different adaptors to one place eventually
  static constexpr const char *PrintfFuncName = "printf";


  // Field for explicit GlobalBufferPtr - used on OpenCL path.
  Value *GlobalBufferPtr = nullptr;
  bool DisableRTGlobalsKnownValues = false;

  void init() {
    // KMD behavior across operating systems differs currently, the value of MaxSlicesSupported is reported to be 1 on
    // Linux. If this is the case, rely on SliceInfo structure passed from runtime.
    uint32_t enabledSlices = 0;
    for (unsigned int sliceID = 0; sliceID < GT_MAX_SLICE; ++sliceID) {
      if (SysInfo.SliceInfo[sliceID].Enabled) {
        enabledSlices++;
      }
    }

    if (Ctx.platform.isCoreChildOf(IGFX_XE2_HPG_CORE) || Ctx.platform.isProductChildOf(IGFX_PVC)) {
      NumDSSPerSlice = SysInfo.MaxSubSlicesSupported / std::max(SysInfo.MaxSlicesSupported, enabledSlices);
      EuCountPerDSS = SysInfo.MaxEuPerSubSlice;
      MaxDualSubSlicesSupported = 0;

      // GTSystem info fills MaxSubSlicesSupported with the number
      // of enabled SubSlices. But this number does not specify the
      // index of the last used SubSlice. It may happen, that all
      // enabled SubSlices lie in higher Slices, e.g.
      //
      // Slice_0 - disabled, 4 SubSlices disabled
      // Slice_1 - disabled, 4 SubSlices disabled
      // Slice_2 - enabled, 4 SubSlices enabled
      // Slice_3 - enabled, 4 SubSlices enabled
      //
      // In this case MaxSubSlicesSupported will be 8, but
      // the index of last used SubSlice should be 16.
      // To get it, iterate through SliceInfo and SubSliceInfo, and
      // find the highest enabled SubSlice.
      //
      // As the number of SubSlices per Slice may be different across
      // platforms, it must calculated before finding the index of the
      // SubSlice.

      IGC_ASSERT(NumDSSPerSlice <= GT_MAX_SUBSLICE_PER_SLICE);

      for (unsigned int sliceID = 0; sliceID < GT_MAX_SLICE; ++sliceID) {
        if (SysInfo.SliceInfo[sliceID].Enabled) {
          // SubSliceInfo size is GT_MAX_SUBSLICE_PER_SLICE, but
          // actual number, calculated for given platform, of SubSlices is used
          // to iterate only through SubSlices present on the platform.
          for (unsigned int ssID = 0; ssID < NumDSSPerSlice; ++ssID) {
            if (SysInfo.SliceInfo[sliceID].SubSliceInfo[ssID].Enabled) {
              MaxDualSubSlicesSupported = std::max(MaxDualSubSlicesSupported, (sliceID * NumDSSPerSlice) + ssID + 1);
            }
          }
        }
      }

      if (!MaxDualSubSlicesSupported)
        MaxDualSubSlicesSupported = SysInfo.MaxSubSlicesSupported;
    } else {
      NumDSSPerSlice = SysInfo.MaxDualSubSlicesSupported / std::max(SysInfo.MaxSlicesSupported, enabledSlices);
      EuCountPerDSS = SysInfo.EUCount / SysInfo.DualSubSliceCount;
      MaxDualSubSlicesSupported = 0;

      // GTSystem info fills MaxDualSubSlicesSupported with the number
      // of enabled DualSubSlices. But this number does not specify the
      // index of the last used DualSubSlice. It may happen, that all
      // enabled DualSubSlices lie in higher Slices, e.g.
      //
      // Slice_0 - disabled, 4 DualSubSlices disabled
      // Slice_1 - disabled, 4 DualSubSlices disabled
      // Slice_2 - enabled, 4 DualSubSlices enabled
      // Slice_3 - enabled, 4 DualSubSlices enabled
      //
      // In this case MaxDualSubSlicesSupported will be 8, but
      // the index of last used DualSubSlice should be 16.
      // To get it, iterate through SliceInfo and DSSInfo, and
      // find the highest enabled DualSubSlice.
      //
      // As the number of DualSubSlices per Slice may be different across
      // platforms, it must calculated before finding the index of the
      // DualSubSlice.

      IGC_ASSERT(NumDSSPerSlice <= GT_MAX_DUALSUBSLICE_PER_SLICE);

      for (unsigned int sliceID = 0; sliceID < GT_MAX_SLICE; ++sliceID) {
        if (SysInfo.SliceInfo[sliceID].Enabled) {
          // DSSInfo size is GT_MAX_DUALSUBSLICE_PER_SLICE, but
          // actual number, calculated for given platform, of DualSubSlices is used
          // to iterate only through DualSubSlices present on the platform.
          for (unsigned int dssID = 0; dssID < NumDSSPerSlice; ++dssID) {
            if (SysInfo.SliceInfo[sliceID].DSSInfo[dssID].Enabled) {
              MaxDualSubSlicesSupported = std::max(MaxDualSubSlicesSupported, (sliceID * NumDSSPerSlice) + dssID + 1);
            }
          }
        }
      }

      if (!MaxDualSubSlicesSupported)
        MaxDualSubSlicesSupported = SysInfo.MaxDualSubSlicesSupported;
    }
    DisableRTGlobalsKnownValues = IGC_IS_FLAG_ENABLED(DisableRTGlobalsKnownValues);
  }

public:
  RTBuilder(LLVMContext &pCtx, const IGC::CodeGenContext &Ctx)
      : IGCIRBuilder<>(pCtx), Ctx(Ctx), SysInfo(Ctx.platform.GetGTSystemInfo()) {
    init();
  }

  explicit RTBuilder(BasicBlock *TheBB, const IGC::CodeGenContext &Ctx, MDNode *FPMathTag = nullptr)
      : IGCIRBuilder<>(TheBB, FPMathTag), Ctx(Ctx), SysInfo(Ctx.platform.GetGTSystemInfo()) {
    init();
  }

  explicit RTBuilder(Instruction *IP, const IGC::CodeGenContext &Ctx, MDNode *FPMathTag = nullptr,
                     ArrayRef<OperandBundleDef> OpBundles = ArrayRef<OperandBundleDef>())
      : IGCIRBuilder<>(IP, FPMathTag, OpBundles), Ctx(Ctx), SysInfo(Ctx.platform.GetGTSystemInfo()) {
    init();
  }

  RTBuilder(BasicBlock *TheBB, BasicBlock::iterator IP, const IGC::CodeGenContext &Ctx, MDNode *FPMathTag = nullptr,
            ArrayRef<OperandBundleDef> OpBundles = ArrayRef<OperandBundleDef>())
      : IGCIRBuilder<>(TheBB->getContext(), FPMathTag, OpBundles), Ctx(Ctx), SysInfo(Ctx.platform.GetGTSystemInfo()) {
    init();
    this->SetInsertPoint(TheBB, IP);
  }

  ~RTBuilder() {}
  RTBuilder(const RTBuilder &) = delete;
  RTBuilder &operator=(const RTBuilder &) = delete;

  inline Value *getMaxSimdSize(void) { return this->getInt32(Ctx.platform.getMaxSimdSize()); }

  inline Value *getMaxThreadsPerEU(void) {
    return this->getInt32(Ctx.platform.GetGTSystemInfo().ThreadCount / Ctx.platform.GetGTSystemInfo().EUCount);
  }

  inline Value *get32BitLaneID(void) {
    Module *module = this->GetInsertBlock()->getModule();
    Function *pFunc = GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_simdLaneId);
    Value *int16LaneId = this->CreateCall(pFunc);
    return this->CreateZExt(int16LaneId, this->getInt32Ty());
  }

  inline Value *get32BitLaneIDReplicate(void) {
    Module *module = this->GetInsertBlock()->getModule();
    Function *pFunc = GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_simdLaneIdReplicate);
    Value *int16LaneId = this->CreateCall(pFunc);
    return this->CreateZExt(int16LaneId, this->getInt32Ty());
  }

  std::pair<BasicBlock *, BasicBlock *> createTriangleFlow(Value *Cond, Instruction *InsertPoint,
                                                           const Twine &TrueBBName = "", const Twine &JoinBBName = "");

  Value *getRtMemBasePtr(Value *globalBufferPtr = nullptr);
  Value *getStackSizePerRay(void);
  Value *getNumDSSRTStacks(void);
  Value *getBaseSurfaceStatePointer(void);
  Value *getBaseSurfaceStatePointer(Value *RayDispatchGlobalData);
  Value *getMaxBVHLevels(void);
  Value *getStatelessScratchPtr(void);
  Value *getLeafType(StackPointerVal *StackPointer, Value *CommittedHit);
  Value *getIsFrontFace(StackPointerVal *StackPointer, Value *ShaderTy);
  // Xe3: memhit->leafNodeSubType
  Value *getLeafNodeSubType(StackPointerVal *StackPointer, Value *CommittedHit);

  Value *CreateSyncStackPtrIntrinsic(Value *Addr, Type *PtrTy, bool AddDecoration);

  RayQueryCheckIntrinsic *CreateRayQueryCheckIntrinsic(Value *predicate = nullptr);
  RayQueryReleaseIntrinsic *CreateRayQueryReleaseIntrinsic(Value *predicate = nullptr);

  PreemptionDisableIntrinsic *CreatePreemptionDisableIntrinsic();
  PreemptionEnableIntrinsic *CreatePreemptionEnableIntrinsic(Value *Flag = nullptr);

  SyncStackPointerVal *
  getSyncStackPointer(Value *globalBufferPtr = nullptr,
                      std::optional<RTBuilder::RTMemoryAccessMode> forceRTStackAccessMode = std::nullopt);

  void createReadSyncTraceRay(Value *val);

  TraceRaySyncIntrinsic *createSyncTraceRay(Value *bvhLevel, Value *traceRayCtrl, Value *globalBufferPointer = nullptr,
                                            const Twine &PayloadName = "");
  TraceRaySyncIntrinsic *createSyncTraceRay(uint32_t bvhLevel, Value *traceRayCtrl,
                                            Value *globalBufferPointer = nullptr, const Twine &PayloadName = "");


  std::pair<BasicBlock *, PHINode *> validateInstanceLeafPtr(RTBuilder::StackPointerVal *perLaneStackPtr,
                                                             Instruction *I, Value *forCommitted);
  std::pair<AllocaInst *, AllocaInst *> createAllocaRayQueryObjects(unsigned int size, bool bShrinkSMStack,
                                                                    const llvm::Twine &Name = "");

  void setDoneBit(StackPointerVal *StackPointer, bool Committed);

  Value *alignVal(Value *V, uint64_t Align);
  uint32_t getSyncStackSize();
  uint32_t getNumSyncStackSlots();

  Value *getRayTMin(StackPointerVal *perLaneStackPtr);
  Value *getRayFlags(SyncStackPointerVal *perLaneStackPtr);
  void setRayFlags(SyncStackPointerVal *perLaneStackPtr, Value *V);
  Value *getRayInfo(StackPointerVal *perLaneStackPtr, uint32_t Idx, uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);

  Value *getWorldRayOrig(StackPointerVal *perLaneStackPtr, uint32_t dim);
  Value *getMemRayOrig(StackPointerVal *perLaneStackPtr, uint32_t dim, uint32_t BvhLevel, const Twine &Name = "");
  Value *getMemRayDir(StackPointerVal *perLaneStackPtr, uint32_t dim, uint32_t BvhLevel, const Twine &Name = "");
  Value *getWorldRayDir(StackPointerVal *perLaneStackPtr, uint32_t dim);
  Value *getObjRayOrig(StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy, Instruction *I = nullptr,
                       bool checkInstanceLeafPtr = false);

  Value *getObjRayDir(StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy, Instruction *I = nullptr,
                      bool checkInstanceLeafPtr = false);

  Value *getRayTCurrent(StackPointerVal *perLaneStackPtr, Value *ShaderTy);

  Value *getInstanceIndex(StackPointerVal *perLaneStackPtr, Value *ShaderTy, Instruction *I, bool checkInstanceLeafPtr);

  Value *getInstanceID(StackPointerVal *perLaneStackPtr, Value *ShaderTy, Instruction *I, bool checkInstanceLeafPtr);

  Value *getPrimitiveIndex(StackPointerVal *perLaneStackPtr, Instruction *I, Value *leafType, Value *ShaderTy,
                           bool checkInstanceLeafPtr);

  Value *getGeometryIndex(StackPointerVal *perLaneStackPtr, Instruction *I, Value *leafType, Value *ShaderTy,
                          bool checkInstanceLeafPtr);

  Value *getInstanceContributionToHitGroupIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Value *ShaderTy);

  Value *getRayMask(RTBuilder::StackPointerVal *perLaneStackPtr);

  Value *getObjToWorld(StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy, Instruction *I = nullptr,
                       bool checkInstanceLeafPtr = false);

  Value *getWorldToObj(StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy, Instruction *I = nullptr,
                       bool checkInstanceLeafPtr = false);


  static void setInvariantLoad(LoadInst *LI);
  CallInst *CreateLSCFence(LSC_SFID SFID, LSC_SCOPE Scope, LSC_FENCE_OP FenceOp);

  // Utilities
  Type *getInt64PtrTy(unsigned int AddrSpace = 0U) const;
  Type *getInt32PtrTy(unsigned int AddrSpace = 0U) const;

  IGC::RTMemoryStyle getMemoryStyle() const;

  Value *getRootNodePtr(Value *BVHPtr);

  Value *isDoneBitNotSet(StackPointerVal *StackPointer, bool Committed);
  Value *getBvhLevel(StackPointerVal *StackPointer, bool Committed);
  // MemHit::t
  void setHitT(StackPointerVal *StackPointer, Value *V, bool Committed);
  Value *getHitT(StackPointerVal *StackPointer, bool Committed);
  Value *getHitValid(StackPointerVal *StackPointer, Value *CommittedHit);
  void setHitValid(StackPointerVal *StackPointer, bool CommittedHit);
  LoadInst *getSyncTraceRayControl(GetElementPtrInst *ptrCtrl);
  void setSyncTraceRayControl(GetElementPtrInst *ptrCtrl, RTStackFormat::TraceRayCtrl ctrl);
  Value *getHitBaryCentric(StackPointerVal *StackPointer, uint32_t idx, Value *CommittedHit);


  Value *getGlobalBufferPtr(IGC::ADDRESS_SPACE Addrspace = IGC::ADDRESS_SPACE_CONSTANT);
  Value *getGlobalBufferPtrForSlot(IGC::ADDRESS_SPACE Addrspace, Value *slot);
  Value *getSyncStackID();
  Value *getSyncStackOffset(bool rtMemBasePtr = true);
  SpillValueIntrinsic *getSpillValue(Value *Val, uint64_t Offset);
  FillValueIntrinsic *getFillValue(Type *Ty, uint64_t Offset, const Twine &Name = "");
  void setGlobalBufferPtr(Value *GlobalBufferPtr);
  void setDisableRTGlobalsKnownValues(bool shouldDisable);


  static Instruction *getEntryFirstInsertionPt(Function &F,
                                               const std::vector<Value *> *pAdditionalInstructionsToSkip = nullptr);

  Value *getTraceRayPayload(Value *bvhLevel, Value *traceRayCtrl, bool isRayQuery, const Twine &PayloadName = "");



  static bool checkAlign(Module &M, StructType *StructTy, uint32_t Align);

  Value *getGlobalSyncStackID();
  Value *getGlobalSyncStackIDBase();

  Value *getGlobalDSSID();
  uint32_t getRTStackSectorSize(uint32_t Align);

  Value *getSyncRTStackSize();

private:
  TraceRayIntrinsic *createTraceRay(Value *bvhLevel, Value *traceRayCtrl, bool isRayQuery,
                                    Value *globalBufferPointer = nullptr, const Twine &PayloadName = "");

  Value *canonizePointer(Value *Ptr);
  uint32_t getRTStack2Size() const;
  Value *getRTStackSize(uint32_t Align);
  SyncStackPointerVal *getSyncStackPointer(RTBuilder::RTMemoryAccessMode Mode, Value *syncStackOffset,
                                           Value *globalBufferPtr = nullptr);
  Value *getGeometryIndex(StackPointerVal *perLaneStackPtr, Value *leafType, Value *ShaderTy);
  Value *getPrimitiveIndex(StackPointerVal *perLaneStackPtr, Value *leafType, Value *ShaderTy);
  Value *getInstanceIndex(StackPointerVal *perLaneStackPtr, Value *ShaderTy);
  Value *getInstanceID(StackPointerVal *perLaneStackPtr, Value *ShaderTy);
  Value *TransformWorldToObject(StackPointerVal *StackPointerVal, unsigned int dim, bool isOrigin, Value *ShaderTy,
                                Instruction *I, bool checkInstanceLeafPtr);

  Value *TransformWorldToObject(StackPointerVal *StackPointerVal, unsigned int dim, bool isOrigin, Value *ShaderTy);

  Value *getObjWorldAndWorldObj(StackPointerVal *perLaneStackPtr, uint32_t infoKind, uint32_t dim, Value *ShaderTy,
                                Instruction *I, bool checkInstanceLeafPtr);

  Value *getObjWorldAndWorldObj(StackPointerVal *perLaneStackPtr, uint32_t infoKind, uint32_t dim, Value *ShaderTy);

  GenIntrinsicInst *getSr0_0();
  Value *emitStateRegID(uint32_t BitStart, uint32_t BitEnd);
  std::pair<uint32_t, uint32_t> getSliceIDBitsInSR0() const;
  std::pair<uint32_t, uint32_t> getDualSubsliceIDBitsInSR0() const;
  std::pair<uint32_t, uint32_t> getSubsliceIDBitsInSR0() const;


  void setReturnAlignment(CallInst *CI, uint32_t AlignVal);
  void setDereferenceable(CallInst *CI, uint32_t Size);


public:
  Type *getSMStack2Ty() const;
  Type *getRTStack2Ty() const;
  Type *getRTStack2PtrTy(bool async = true) const;
  Type *getRTStack2PtrTy(RTMemoryAccessMode Mode, bool async = true) const;
  Type *getRayDispatchGlobalDataPtrTy(Module &M, IGC::ADDRESS_SPACE Addrspace);

  ConstantInt *supportStochasticLod();

  ConstantInt *isRayQueryReturnOptimizationEnabled();


  GenIntrinsicInst *createDummyInstID(Value *pSrcVal);

  CallInst *ctlz(Value *V);
  CallInst *cttz(Value *V);

  void createPotentialHit2CommittedHit(StackPointerVal *StackPtr);

  void createTraceRayInlinePrologue(StackPointerVal *StackPtr, Value *RayInfo, Value *RootNodePtr, Value *RayFlags,
                                    Value *InstanceInclusionMask, Value *ComparisonValue, Value *TMax,
                                    bool updateFlags = true, bool initialDoneBitValue = false);

  void emitSingleRQMemRayWrite(SyncStackPointerVal *HWStackPtr, SyncStackPointerVal *SMStackPtr, bool singleRQProceed);

  void copyMemHitInProceed(SyncStackPointerVal *HWStackPtr, SyncStackPointerVal *SMStackPtr, bool singleRQProceed);

  Value *syncStackToShadowMemory(SyncStackPointerVal *HWStackPtr, SyncStackPointerVal *SMStackPtr,
                                 Value *ProceedReturnVal, Value *ShadowMemRTCtrlPtr);

  Value *getCommittedStatus(SyncStackPointerVal *SMStackPtr);
  Value *getCandidateType(SyncStackPointerVal *SMStackPtr);

  void commitProceduralPrimitiveHit(SyncStackPointerVal *SMStackPtr, Value *THit);

  Value *getHitAddress(StackPointerVal *StackPtr, bool Committed);
  template <typename StackPointerValT, typename RayInfoIntrinsicT>
  Value *lowerRayInfo(StackPointerValT *perLaneStackPtr, RayInfoIntrinsicT *I, Value *shaderType,
                      std::optional<bool> isProcedural) {
    return lowerRayInfo(perLaneStackPtr, I, shaderType, isProcedural.has_value() ? getInt1(*isProcedural) : nullptr);
  }

  template <typename StackPointerValT, typename RayInfoIntrinsicT>
  Value *lowerRayInfo(StackPointerValT *perLaneStackPtr, RayInfoIntrinsicT *I, Value *shaderType, Value *isProcedural);


};

} // namespace llvm
