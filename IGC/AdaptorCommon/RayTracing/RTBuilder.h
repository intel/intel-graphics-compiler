/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "RTStackFormat.h"
#include "common/IGCIRBuilder.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CodeGenPublic.h"
#include "visa_igc_common_header.h"

namespace IGC {
    class RTArgs;
    class TraceRayRTArgs;
}

namespace llvm {

class RTBuilder : public IGCIRBuilder<>
{
public:
    // Here are more specialized values that enforce more type safety
    // between the accessor methods of RTBuilder at the C++ level to avoid
    // passing the wrong values in.
    class PtrVal : public Value {};
    class IntVal : public Value {};
    class StackPointerVal       : public PtrVal {};
    class AsyncStackPointerVal  : public StackPointerVal {};
    class SyncStackPointerVal   : public StackPointerVal {};
    class RayQueryObjectVal : public PtrVal {};
    class SWHotZonePtrVal : public PtrVal {};
    class StackOffsetPtrVal : public PtrVal {};
    class StackOffsetIntVal : public IntVal {};
    // This represents an i8* to the base of the SWStack
    class SWStackPtrVal : public PtrVal {};
    static_assert(sizeof(PtrVal) == sizeof(Value), "added something?");
public:
    static constexpr char *MergeFuncName = "__mergeContinuation";
    static constexpr char *BTDTarget = "btd.target";
    static constexpr char* SpillSize = "spill.size";
    static constexpr char* IsContinuation = "is.continuation";
    enum RTMemoryAccessMode
    {
        STATELESS,
        STATEFUL,
    };

    static uint32_t getRTAsyncStackStatefulAddrSpace(const IGC::ModuleMetaData &MMD);
    static uint32_t getSWHotZoneStatefulAddrSpace(const IGC::ModuleMetaData &MMD);
    static uint32_t getSWStackStatefulAddrSpace(const IGC::ModuleMetaData &MMD);
    static uint32_t getRTSyncStackStatefulAddrSpace(const IGC::ModuleMetaData& MMD);

    static uint32_t getSWStackAddrSpace(const IGC::ModuleMetaData &MMD);
private:
    const IGC::CodeGenContext& Ctx;
    const GT_SYSTEM_INFO SysInfo;

    uint32_t NumDSSPerSlice            = 0;
    uint32_t EuCountPerDSS             = 0;
    uint32_t ThreadCountPerEU          = 0;
    uint32_t MaxDualSubSlicesSupported = 0;

    //TODO: this is hardcoded string, we might want to put all "printf" of different adaptors to one place eventually
    static constexpr char *PrintfFuncName = "printf";


    enum RayQueryArrayIndexes {
        RAY_ORIG = 0,
        RAY_DIR,
        T_MIN,
        T_MAX,
        RAY_FLAGS,
        INSTANCE_INCLUSION_MASK,
        BVH_PTR,
        COMMITTED_HIT_DISTANCE,
        COMMITTED_HIT_BAR_U,
        COMMITTED_HIT_BAR_V,
        COMMITTED_HIT_INFO,
        COMMITTED_HIT_PRIM_LEAF_PTR,
        COMMITTED_HIT_INSTANCE_LEAF_PTR,
        POTENTIAL_HIT_DISTANCE,
        POTENTIAL_HIT_BAR_U,
        POTENTIAL_HIT_BAR_V,
        POTENTIAL_HIT_INFO,
        POTENTIAL_HIT_PRIM_LEAF_PTR,
        POTENTIAL_HIT_INSTANCE_LEAF_PTR,
        POTENTIAL_RAY_ORIG,
        POTENTIAL_RAY_DIR,
        TRACE_RAY_CTRL,
    };

    // Field for explicit GlobalBufferPtr - used on OpenCL path.
    Value* GlobalBufferPtr = nullptr;
    bool DisableRTGlobalsKnownValues = false;

    void init()
    {
        // KMD behavior across operating systems differs currently, the value of MaxSlicesSupported is reported to be 1 on Linux.
        // If this is the case, rely on SliceInfo structure passed from runtime.
        uint32_t enabledSlices = 0;
        for (unsigned int sliceID = 0; sliceID < GT_MAX_SLICE; ++sliceID)
        {
            if (SysInfo.SliceInfo[sliceID].Enabled) {
                enabledSlices++;
            }
        }

        if (Ctx.platform.isProductChildOf(IGFX_PVC))
        {
            NumDSSPerSlice = SysInfo.MaxSubSlicesSupported / std::max(SysInfo.MaxSlicesSupported, enabledSlices);
            EuCountPerDSS = SysInfo.MaxEuPerSubSlice;
            MaxDualSubSlicesSupported = SysInfo.MaxSubSlicesSupported;

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

            for (unsigned int sliceID = 0; sliceID < GT_MAX_SLICE; ++sliceID)
            {
                if (SysInfo.SliceInfo[sliceID].Enabled)
                {
                    // SubSliceInfo size is GT_MAX_SUBSLICE_PER_SLICE, but
                    // actual number, calculated for given platform, of SubSlices is used
                    // to iterate only through SubSlices present on the platform.
                    for (unsigned int ssID = 0; ssID < NumDSSPerSlice; ++ssID)
                    {
                        if (SysInfo.SliceInfo[sliceID].SubSliceInfo[ssID].Enabled)
                        {
                            MaxDualSubSlicesSupported = std::max(MaxDualSubSlicesSupported, (sliceID * NumDSSPerSlice) + ssID + 1);
                        }
                    }
                }
            }
        }
        else
        {
            NumDSSPerSlice = SysInfo.MaxDualSubSlicesSupported / std::max(SysInfo.MaxSlicesSupported, enabledSlices);
            EuCountPerDSS = SysInfo.EUCount / SysInfo.DualSubSliceCount;
            MaxDualSubSlicesSupported = SysInfo.MaxDualSubSlicesSupported;

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

            for (unsigned int sliceID = 0; sliceID < GT_MAX_SLICE; ++sliceID)
            {
                if (SysInfo.SliceInfo[sliceID].Enabled)
                {
                    // DSSInfo size is GT_MAX_DUALSUBSLICE_PER_SLICE, but
                    // actual number, calculated for given platform, of DualSubSlices is used
                    // to iterate only through DualSubSlices present on the platform.
                    for (unsigned int dssID = 0; dssID < NumDSSPerSlice; ++dssID)
                    {
                        if (SysInfo.SliceInfo[sliceID].DSSInfo[dssID].Enabled)
                        {
                            MaxDualSubSlicesSupported = std::max(MaxDualSubSlicesSupported, (sliceID * NumDSSPerSlice) + dssID + 1);
                        }
                    }
                }
            }
        }

        ThreadCountPerEU = SysInfo.ThreadCount / SysInfo.EUCount;

        DisableRTGlobalsKnownValues = IGC_IS_FLAG_ENABLED(DisableRTGlobalsKnownValues);
    }

public:
    RTBuilder(LLVMContext& pCtx, const IGC::CodeGenContext &Ctx)
        : IGCIRBuilder<>(pCtx),
          Ctx(Ctx),
          SysInfo(Ctx.platform.GetGTSystemInfo()) {
        init();
    }

    explicit RTBuilder(
        BasicBlock* TheBB,
        const IGC::CodeGenContext& Ctx,
        MDNode* FPMathTag = nullptr)
        : IGCIRBuilder<>(TheBB, FPMathTag),
          Ctx(Ctx),
          SysInfo(Ctx.platform.GetGTSystemInfo()) {
        init();
    }

    explicit RTBuilder(
        Instruction* IP,
        const IGC::CodeGenContext& Ctx,
        MDNode* FPMathTag = nullptr,
        ArrayRef<OperandBundleDef> OpBundles = None)
        : IGCIRBuilder<>(IP, FPMathTag, OpBundles),
          Ctx(Ctx),
          SysInfo(Ctx.platform.GetGTSystemInfo()) {
        init();
    }

    RTBuilder(
        BasicBlock* TheBB,
        BasicBlock::iterator IP,
        const IGC::CodeGenContext& Ctx,
        MDNode* FPMathTag = nullptr,
        ArrayRef<OperandBundleDef> OpBundles = None)
        : IGCIRBuilder<>(TheBB->getContext(), FPMathTag, OpBundles),
          Ctx(Ctx),
          SysInfo(Ctx.platform.GetGTSystemInfo()) {
        init();
        this->SetInsertPoint(TheBB, IP);
    }

    ~RTBuilder() {}

    inline Value* createGroupId(unsigned int dim, const Twine &Name = "")
    {
        Module* module = this->GetInsertBlock()->getModule();
        Function* pFunc = GenISAIntrinsic::getDeclaration(
            module,
            GenISAIntrinsic::GenISA_DCL_SystemValue,
            this->getFloatTy());
        return this->CreateBitCast(
            this->CreateCall(
                pFunc,
                this->getInt32(IGC::THREAD_GROUP_ID_X + dim)),
            this->getInt32Ty(),
            Name);
    }

    inline Value* createSubgroupId()
    {
        Module* module = this->GetInsertBlock()->getModule();
        Function* systemValueIntrinsic = GenISAIntrinsic::getDeclaration(
            module,
            GenISAIntrinsic::GenISA_DCL_SystemValue,
            this->getInt16Ty());

        Value* subgroupId = this->CreateCall(
            systemValueIntrinsic,
            this->getInt32(IGC::THREAD_ID_WITHIN_THREAD_GROUP),
            VALUE_NAME("TID"));

        return subgroupId;
    }

    inline Value* createTileXOffset(
        uint32_t TileXDim, uint32_t SubtileXDim, uint32_t SubtileYDim)
    {
        Module* module = this->GetInsertBlock()->getModule();
        Function* pFunc = GenISAIntrinsic::getDeclaration(
            module,
            GenISAIntrinsic::GenISA_TileXOffset);
        return this->CreateCall4(
            pFunc,
            this->createSubgroupId(),
            this->getInt16(int_cast<uint16_t>(TileXDim)),
            this->getInt16(int_cast<uint16_t>(SubtileXDim)),
            this->getInt16(int_cast<uint16_t>(SubtileYDim)),
            VALUE_NAME("TileXOffset"));
    }

    inline Value* createTileYOffset(
        uint32_t TileXDim, uint32_t SubtileXDim, uint32_t SubtileYDim)
    {
        Module* module = this->GetInsertBlock()->getModule();
        Function* pFunc = GenISAIntrinsic::getDeclaration(
            module,
            GenISAIntrinsic::GenISA_TileYOffset);
        return this->CreateCall4(
            pFunc,
            this->createSubgroupId(),
            this->getInt16(int_cast<uint16_t>(TileXDim)),
            this->getInt16(int_cast<uint16_t>(SubtileXDim)),
            this->getInt16(int_cast<uint16_t>(SubtileYDim)),
            VALUE_NAME("TileYOffset"));
    }

    inline Value* createThreadLocalId(unsigned int dim)
    {
        Module* module = this->GetInsertBlock()->getModule();
        Function* pFunc = GenISAIntrinsic::getDeclaration(
            module,
            GenISAIntrinsic::GenISA_DCL_SystemValue,
            this->getInt16Ty());
        return this->CreateCall(pFunc, this->getInt32(IGC::THREAD_ID_IN_GROUP_X + dim));
    }

    inline Value* getSimdSize(void)
    {
        Module* module = this->GetInsertBlock()->getModule();
        Function* pFunc = GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_simdSize);
        return this->CreateCall(pFunc);
    }

    inline Value* get32BitLaneID(void)
    {
        Module* module = this->GetInsertBlock()->getModule();
        Function* pFunc = GenISAIntrinsic::getDeclaration(
            module,
            GenISAIntrinsic::GenISA_simdLaneId);
        Value* int16LaneId = this->CreateCall(pFunc);
        return this->CreateZExt(int16LaneId, this->getInt32Ty());
    }

    std::pair<BasicBlock*, BasicBlock*> createTriangleFlow(
        Value* Cond,
        Instruction* InsertPoint,
        const Twine &TrueBBName = "",
        const Twine &JoinBBName = "");

    Value* getRtMemBasePtr(void);
    Value* getpRtMemBasePtr(void);
    Value* getCallStackHandler(void);
    Value* getStackSizePerRay(void);
    Value* getpStackSizePerRay(void);
    Value* getSWStackSizePerRay(void);
    Value* getNumDSSRTStacks(void);
    Value* getpNumDSSRTStacks(void);
    Value* getMaxBVHLevels(void);
    Value* getHitGroupBasePtr(void);
    Value* getpHitGroupBasePtr(void);
    Value* getMissShaderBasePtr(void);
    Value* getpMissShaderBasePtr(void);
    Value* getCallableShaderBasePtr(void);
    Value* getpCallableShaderBasePtr(void);
    Value* getBindlessHeapBasePtr(void);
    Value* getBaseSSHOffset(void);
    Value* getPrintfBufferBasePtr(void);
    Value* getHitGroupStride(void);
    Value* getpHitGroupStride(void);
    Value* getMissShaderStride(void);
    Value* getpMissShaderStride(void);
    Value* getCallableShaderStride(void);
    Value* getpCallableShaderStride(void);
    Value* getDispatchRayDimension(unsigned int dim);
    Value* getLeafType(
        StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy);
    Value* getIsFrontFace(
        StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy);
    Value* getTriangleHitKind(
        StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy);
    StackOffsetIntVal* getFirstFrameOffset();
    SWStackPtrVal* bumpStackPtr(
        SWStackPtrVal* FrameAddr,
        uint64_t Amount,
        StackOffsetIntVal* FrameOffset,
        StackOffsetPtrVal* Ptr);
    void writeNewStackOffsetVal(
        Value* Offset,
        StackOffsetPtrVal* Ptr);
    Value* CreateSWHotZonePtrIntrinsic(Value *Addr, Type *PtrTy, bool AddDecoration);
    Value* CreateAsyncStackPtrIntrinsic(Value *Addr, Type *PtrTy, bool AddDecoration);
    Value* CreateSyncStackPtrIntrinsic(Value* Addr, Type* PtrTy, bool AddDecoration);


    CallInst* CreateSWStackPtrIntrinsic(
        Value *Addr, bool AddDecoration, const Twine &Name = "");
    SWStackPtrVal* getSWStackPointer(
        const Twine& Name = "");
    SWStackPtrVal* getSWStackPointer(
        // The raygen shader should not pass in a stack offset since the
        // offset has not been initialized yet!
        Optional<StackOffsetIntVal*> StackOffset,
        int32_t FrameSize,
        bool SubtractFrameSize,
        SWStackPtrVal* &CurStackVal,
        const Twine& Name = "");
    SWHotZonePtrVal* getSWHotZonePointer(bool BuildAddress = false);
    AsyncStackPointerVal* getAsyncStackPointer(bool BuildAddress = false);
    SyncStackPointerVal*  getSyncStackPointer();
    CallInst* CreateBTDCall(Value* RecordPointer);
    StackIDReleaseIntrinsic* CreateStackIDRelease(
        Value* StackID = nullptr, Value* Flag = nullptr);
    CallInst* createMergeCall();

    // Note: 'traceRayCtrl' should be already by 8 bits to its location
    // in the payload before passing as an argument to this function.
    // getTraceRayPayload() just ORs together the bvh, ctrl, and stack id.
    Value* createTraceRay(
        Value* bvhLevel,
        Value* traceRayCtrl,
        bool isRayQuery,
        const Twine& PayloadName = "");

    void createReadSyncTraceRay(Value* val);

    Value* createSyncTraceRay(
        Value* bvhLevel,
        Value* traceRayCtrl,
        const Twine& PayloadName = "");

    Value* createSyncTraceRay(
        uint8_t bvhLevel,
        Value* traceRayCtrl,
        const Twine& PayloadName = "");

    Value* createASyncTraceRay(
        Value* bvhLevel,
        unsigned int traceRayCtrl,
        const Twine& PayloadName = "");

    Value* createASyncTraceRay(
        uint8_t bvhLevel,
        unsigned int traceRayCtrl,
        const Twine& PayloadName = "");

    Value* createASyncTraceRay(
        Value* bvhLevel,
        Value* traceRayCtrl,
        const Twine& PayloadName = "");

    void WriteBlockData(Value* dstPtr, Value* srcPtr, uint32_t size, const DenseMap<uint32_t, Value*>& vals, const Twine& dstName = "", const Twine& srcName = "");

    std::pair<BasicBlock*, PHINode*> validateInstanceLeafPtr(RTBuilder::StackPointerVal* perLaneStackPtr, Instruction* I, bool forCommitted);

    std::pair<Value*, Value*> createAllocaRayQueryObjects(unsigned int size, bool bShrinkSMStack, const llvm::Twine& Name = "");
    void SpillRayQueryShadowMemory(Value* dst, Value* shMem, uint64_t size, unsigned align);
    Value* LoadInstanceContributionToHitGroupIndex(Value* instLeafPtr);
    void FillRayQueryShadowMemory(Value* shMem, Value* src, uint64_t size, unsigned align);
    void MemCpyPotentialHit2CommitHit(RTBuilder::StackPointerVal* StackPointer);
    void setPotentialDoneBit(StackPointerVal* StackPointer);
    void CreateAbort(StackPointerVal* StackPointer);
    static uint32_t getSWHotZoneSize();
    Value* getNumAsyncStackSlots();
    Value* getSumSWHotZoneSize();
    uint32_t getSyncStackSize();
    uint32_t getNumSyncStackSlots();
    uint32_t getSumSyncStackSize();
    Value* alignVal(Value* V, uint64_t Align);
    Value* getRayInfoPtr(StackPointerVal* StackPointer, uint32_t Idx, uint32_t BvhLevel);
    Value* getRayInfo(StackPointerVal* perLaneStackPtr, uint32_t Idx);
    Value* getRayTMin(StackPointerVal* perLaneStackPtr);
    Value* getRayFlagsPtr(SyncStackPointerVal* perLaneStackPtr);
    Value* getRayFlags(SyncStackPointerVal* perLaneStackPtr);
    void   setRayFlags(SyncStackPointerVal* perLaneStackPtr, Value* V);
    Value* getRayFlagsPtr(AsyncStackPointerVal* perLaneStackPtr);
    Value* getRayFlags(AsyncStackPointerVal* perLaneStackPtr);
    Value* getWorldRayOrig(StackPointerVal* perLaneStackPtr, uint32_t dim);
    Value* getWorldRayDir(StackPointerVal* perLaneStackPtr, uint32_t dim);
    Value* getObjRayOrig(
        StackPointerVal* perLaneStackPtr, uint32_t dim, IGC::CallableShaderTypeMD ShaderTy,
        Instruction* I = nullptr, bool checkInstanceLeafPtr = false);

    Value* getObjRayDir(
        StackPointerVal* perLaneStackPtr, uint32_t dim, IGC::CallableShaderTypeMD ShaderTy,
        Instruction* I = nullptr, bool checkInstanceLeafPtr = false);

    Value* getRayTCurrent(
        StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy);

    Value* getInstance(
        StackPointerVal* perLaneStackPtr, uint32_t infoKind, IGC::CallableShaderTypeMD ShaderTy,
        Instruction* I, bool checkInstanceLeafPtr);

    Value* getPrimitiveIndex(
        StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy, bool checkInstanceLeafPtr);

    Value* getGeometryIndex(
        StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy, bool checkInstanceLeafPtr);


    Value* getObjToWorld(
        StackPointerVal* perLaneStackPtr,
        uint32_t dim,
        IGC::CallableShaderTypeMD ShaderTy,
        Instruction* I = nullptr,
        bool checkInstanceLeafPtr = false);

    Value* getWorldToObj(
        StackPointerVal* perLaneStackPtr,
        uint32_t dim,
        IGC::CallableShaderTypeMD ShaderTy,
        Instruction* I = nullptr,
        bool checkInstanceLeafPtr = false);

    //ptr to MemHit::t
    Value* getCommittedHitPtr(StackPointerVal* perLaneStackPtr);
    //ptr to MemHit::t
    Value* getPotentialHitPtr(StackPointerVal* perLaneStackPtr);
    Value* getMemRayPtr(StackPointerVal* perLaneStackPtr, bool isTopLevel = true);

    Value* getHitGroupRecPtr(Value* hitGroupRecPtr0, Value* hitGroupRecPtr1);
    Value* getHitGroupRecPtrFromPrimAndInstVals(Value* PotentialPrimVal, Value* PotentialInstVal);
    Value* CreateShaderType();

    static bool isNonLocalAlloca(const AllocaInst* AI);
    static bool isNonLocalAlloca(uint32_t AddrSpace);
    static Function* updateIntrinsicMangle(
        FunctionType* NewFuncTy, Function& F);

    static void setInvariantLoad(LoadInst* LI);

    Value* getDispatchRayIndex(SWHotZonePtrVal* SWHotZonePtr, uint32_t Dim);
    void   setDispatchRayIndices(
        SWHotZonePtrVal* SWHotZonePtr, const std::vector<Value*> &Indices);
    StackOffsetIntVal* getSWStackOffset(SWHotZonePtrVal* SWHotZonePtr);
    StackOffsetIntVal* getSWStackOffset(StackOffsetPtrVal* OffsetPtr);
    StackOffsetPtrVal* getAddrOfSWStackOffset(SWHotZonePtrVal* SWHotZonePtr);

    //Helper function to store data into the RT Stack
    void storeContinuationAddress(
        IGC::TraceRayRTArgs &Args,
        Type* PayloadTy,
        ContinuationHLIntrinsic* intrin,
        SWStackPtrVal* StackFrameVal);
    SmallVector<StoreInst*, 2> storePayload(
        IGC::TraceRayRTArgs &Args,
        Value* Payload,
        SWStackPtrVal* StackFrameVal);

    Value* computeReturnIP(
        const IGC::RayDispatchShaderContext& RayCtx,
        Function &F);

    CallInst* CreateLSCFence(LSC_SFID SFID, LSC_SCOPE Scope, LSC_FENCE_OP FenceOp);
public:
    // Utilities
    Type* getInt64PtrTy(unsigned int AddrSpace = 0U) const;
    Type* getInt32PtrTy(unsigned int AddrSpace = 0U) const;
public:
    Value* getRootNodePtr(Value* BVHPtr);
    void setRayInfo(
        StackPointerVal* StackPointer,
        Value* V,
        uint32_t Idx,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    Value* getNodePtrAndFlagsPtr(
        StackPointerVal* StackPointer,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    //MemRay::topOfNodePtrAndFlags
    Value* getNodePtrAndFlags(
        StackPointerVal* StackPointer,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    void setNodePtrAndFlags(
        StackPointerVal* StackPointer,
        Value* V,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    void setHitGroupPtrAndStride(
        StackPointerVal* StackPointer,
        Value* V,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    Value* getMissShaderPtr(
        StackPointerVal* StackPointer,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    void setMissShaderPtr(
        StackPointerVal* StackPointer,
        Value* V,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    //MemRay::topOfInstanceLeafPtr
    Value* getInstLeafPtrAndRayMask(
        StackPointerVal* StackPointer,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    void setInstLeafPtrAndRayMask(
        StackPointerVal* StackPointer,
        Value* V,
        uint32_t BvhLevel = RTStackFormat::TOP_LEVEL_BVH);
    void setCommittedHitT(
        StackPointerVal* StackPointer,
        Value* V);
    Value* isDoneBitNotSet(Value* HitInfoVal);
    Value* isDoneBitSet(Value* HitInfoVal);
    Value* getMemHitBvhLevel(Value* MemHitInfoVal);
    Value* getPotentialHitBvhLevel(StackPointerVal* StackPointer);
    //MemHit::t
    Value* getPotentialHitT(StackPointerVal* StackPointer);
    void   setPotentialHitT(StackPointerVal* StackPointer, Value *V);
    Value* getPotentialHitTopPrimLeafPtr(StackPointerVal* StackPointer);
    void   setCommittedHitTopPrimLeafPtr(StackPointerVal* StackPointer, Value* V);
    Value* getPotentialHitTopInstLeafPtr(StackPointerVal* StackPointer);
    void   setCommittedHitTopInstLeafPtr(StackPointerVal* StackPointer, Value* V);
    Value* getHitValid(StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy);
    void   setHitValid(StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy);
    Value* getPotentialHitInfo(RTBuilder::StackPointerVal* StackPointer, const Twine& Name = "PotentialHitInfo");
    Value* getSyncTraceRayControl(Value* ptrCtrl);
    void   setSyncTraceRayControl(Value* ptrCtrl, unsigned ctrl);
    Value* getHitBaryCentric(StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, uint32_t dim);
    Value* setHitBaryCentric(StackPointerVal* StackPointer, Value* V, IGC::CallableShaderTypeMD ShaderTy, uint32_t dim);
    Value* getInstContToHitGroupIndex(RTBuilder::StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy);
    //MemHit::topOfPrimIndexDelta/frontFaceDword/hitInfoDWord
    Value* getHitInfoDWordPtr(RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, const Twine& Name);
    Value* getHitInfoDWord(RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, const Twine& Name = "HitInfo");
    void   setHitInfoDWord(RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, Value* V, const Twine& Name = "HitInfo");
    //MemHit::topOfPrimLeafPtr
    Value* getHitTopOfPrimLeafPtr(StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy);
    Value* getHitInstanceLeafPtr(StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy);
    Value* createAllocaNumber(const AllocaInst *AI, uint32_t Number);
    static DenseMap<const AllocaInst*, uint32_t> getAllocaNumberMap(const Function &F);
    static DenseMap<uint32_t, const AllocaInst*> getNumberAllocaMap(const Function &F);
    Value* getGlobalBufferPtr();
    Value* getLocalBufferPtr(llvm::Optional<uint32_t> root_sig_size);
    CallInst* getInlineData(Type* RetTy, uint32_t QwordOffset, uint32_t Alignment, llvm::Optional<uint32_t> root_sig_size = None);
    Value* getAsyncStackID();
    Value* getSyncStackID();
    Value* getSyncStackOffset(bool rtMemBasePtr = true);
    void setProceduralHitKind(
        IGC::RTArgs &Args,
        SWStackPtrVal* FrameAddr, Value* V);
    Value* getProceduralHitKind(
        IGC::RTArgs &Args,
        SWStackPtrVal* FrameAddr);
    PayloadPtrIntrinsic* getPayloadPtrIntrinsic(
        Value* PayloadPtr, SWStackPtrVal* FrameAddr);
    ContinuationSignpostIntrinsic* getContinuationSignpost(Value* FrameAddr, Value* Offset);

    SpillValueIntrinsic* getSpillValue(Value* Val, uint64_t Offset);
    FillValueIntrinsic* getFillValue(Type* Ty, uint64_t Offset, const Twine& Name = "");

    void setGlobalBufferPtr(Value* GlobalBufferPtr);
    void setDisableRTGlobalsKnownValues(bool shouldDisable);
    GenIntrinsicInst* getSpillAnchor(Value* V);
    static void setSpillSize(ContinuationHLIntrinsic& CI, uint32_t SpillSize);
    static Optional<uint32_t> getSpillSize(
        const ContinuationHLIntrinsic& CI);
    static void markAsContinuation(Function& F);
    static bool isContinuation(const Function& F);

    GetShaderRecordPtrIntrinsic* getShaderRecordPtr(Function* F);
public:
    static Instruction* getEntryFirstInsertionPt(
        Function &F,
        const std::vector<Value*>* pAdditionalInstructionsToSkip = nullptr);
public:
    static Instruction* LowerPayload(
        Function *F, IGC::RTArgs &Args, SWStackPtrVal *FrameAddr);
    static void lowerIntersectionAttributeFromMemHit(
        Function &F,
        const IGC::RTArgs &Args,
        StackPointerVal* StackPointer);
    static void loadCustomHitAttribsFromStack(
        Function &F,
        IGC::RTArgs &Args,
        SWStackPtrVal *FrameAddr);
    uint32_t getNumSIMDLanesPerDSS();
private:
    Value* canonizePointer(Value* Ptr);

    Value* getGlobalAsyncStackID();
    AsyncStackPointerVal* getAsyncStackPointer(
        Value* asyncStackOffset,
        RTMemoryAccessMode Mode);
    Value* GetAsyncStackOffset();
    Value* getGlobalSyncStackID();
    Value* getSyncRTStackSize();
    Value* getAsyncRTStackSize();
    Value* getRTStackSize(uint32_t Align);
    SyncStackPointerVal* getSyncStackPointer(Value* syncStackOffset, RTBuilder::RTMemoryAccessMode Mode);
    Value* getHitUPtr(StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy);
    Value* getHitBaryCentricPtr(StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, uint32_t dim);
    std::pair<Value*, BasicBlock*> getGeometryIndex(
        StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy);
    PHINode* getPrimitiveIndex(
        StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy);
    Value* getInstance(
        StackPointerVal* perLaneStackPtr, uint32_t infoKind, IGC::CallableShaderTypeMD ShaderTy);
    Value* TransformWorldToObject(
        StackPointerVal* StackPointerVal,
        unsigned int dim,
        bool isOrigin,
        IGC::CallableShaderTypeMD ShaderTy,
        Instruction* I,
        bool checkInstanceLeafPtr);

    Value* TransformWorldToObject(
        StackPointerVal* StackPointerVal,
        unsigned int dim,
        bool isOrigin,
        IGC::CallableShaderTypeMD ShaderTy);

    Value* getMatrixPtr(
        Value* InstanceLeafPtr,
        uint32_t infoKind,
        uint32_t dim);

    Value* getObjWorldAndWorldObj(
        StackPointerVal* perLaneStackPtr,
        uint32_t infoKind,
        uint32_t dim,
        IGC::CallableShaderTypeMD ShaderTy,
        Instruction* I,
        bool checkInstanceLeafPtr);

    Value* getObjWorldAndWorldObj(
        StackPointerVal* perLaneStackPtr,
        uint32_t infoKind,
        uint32_t dim,
        IGC::CallableShaderTypeMD ShaderTy);

    Value* emitStateRegID(uint32_t BitStart, uint32_t BitEnd);
    std::pair<uint32_t, uint32_t> getSliceIDBitsInSR0() const;
    std::pair<uint32_t, uint32_t> getSubsliceIDBitsInSR0() const;
    std::pair<uint32_t, uint32_t> getDualSubsliceIDBitsInSR0() const;
    Value* getSliceID();
    Value* getSubsliceID();
    Value* getDualSubsliceID();

    Value* getGlobalDSSID();

    Value* getInstanceLeafPtr(Value* instLeafTopPtr);

    void setReturnAlignment(CallInst *CI, uint32_t AlignVal);
    void setNoAlias(CallInst *CI);
    void setDereferenceable(CallInst *CI, uint32_t Size);

    void printf(Constant* formatStrPtr, ArrayRef<Value*> Args = None);
    const IGC::RayDispatchShaderContext& RtCtx() const;
//printf
public:

    Value* getTraceRayPayload(
        Value* bvhLevel,
        Value* traceRayCtrl,
        bool isRayQuery,
        const Twine& PayloadName = "");

    void printTraceRay(const TraceRayAsyncHLIntrinsic* trace);
    void printDispatchRayIndex(const std::vector<Value*>& Indices);
public:
    static void injectPadding(
        Module& M,
        SmallVector<Type*, 4> & Tys,
        uint32_t Align,
        bool IsPacked);

    static bool checkAlign(
        Module &M,
        StructType *StructTy,
        uint32_t Align);
private:
#include "AutoGenRTStackAccess.h"
public:
    static Type* getRTStack2PtrTy(
        const IGC::CodeGenContext& Ctx, RTMemoryAccessMode Mode, bool async = true);
    static Type* getHWRayData2PtrTy(Module &M);
    static Type* getSWHotZonePtrTy(
        const IGC::CodeGenContext& Ctx,
        RTMemoryAccessMode Mode);
    static Type* getRayDispatchGlobalDataPtrTy(Module &M);
    static Type* getInstanceLeafPtrTy(Module &M);
    static Type* getQuadLeafPtrTy(Module &M);
    static Type* getProceduralLeafPtrTy(Module &M);
    static Type* getBVHPtrTy(Module &M);
    static StructType* getRTGlobalsAndRootSig(
        Module &M, Type* TypeHoleGlobalRootSig, StringRef Name);
    static StructType* getShaderRecordTy(
        Module &M, Type* TypeHoleLocalRootSig, StringRef Name);
};

} // namespace llvm

