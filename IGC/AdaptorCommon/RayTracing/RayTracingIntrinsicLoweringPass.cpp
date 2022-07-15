/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass lowers intrinsics across all raytracing shader types into
/// RTStack operations.
///
/// The assumption is that the SplitAsync pass has already been run such that
/// shaders with calls to TraceRay() and CallShader() have been broken into
/// continuations.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "StackFrameInfo.h"
#include "ContinuationUtils.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "iStdLib/utility.h"
#include "visa_igc_common_header.h"
#include "MemRegionAnalysis.h"
#include "ShaderProperties.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include "Utils.h"

#include <vector>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;
using namespace ShaderProperties;

class RayTracingIntrinsicLoweringPass : public ModulePass
{
public:
    RayTracingIntrinsicLoweringPass()
        : ModulePass(ID),
          m_module(nullptr)
    {
        initializeRayTracingIntrinsicLoweringPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "RayTracingIntrinsicLoweringPass";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;
private:
    static constexpr char* MergeFuncName = RTBuilder::MergeFuncName;

    Value* GetRayInfo(
        DISPATCH_SHADER_RAY_INFO_TYPE infoKind,
        unsigned int dim,
        RTBuilder& builder,
        CallableShaderTypeMD CallableShaderType,
        Instruction* I);
    void LowerTraces(
        Function& F,
        StackFrameInfo &SFI,
        const std::vector<Value*> &Indices,
        RTBuilder::SWStackPtrVal *FrameAddr,
        RTBuilder::SWStackPtrVal *CurStackVal,
        RTBuilder::StackOffsetPtrVal *StackPtrLocPtr,
        RTBuilder::StackOffsetIntVal *FrameOffset,
        RTBuilder::SWHotZonePtrVal* SWHotZonePtr);
    void LowerCallShaders(
        Function& F,
        StackFrameInfo& SFI,
        const std::vector<Value*> &Indices,
        RTBuilder::SWStackPtrVal *FrameAddr,
        RTBuilder::SWStackPtrVal *CurStackVal,
        RTBuilder::StackOffsetPtrVal *StackPtrLocPtr,
        RTBuilder::StackOffsetIntVal *FrameOffset,
        RTBuilder::SWHotZonePtrVal *SWHotZonePtr);
    void LowerProceduralHitAttributes(
        Function& F, StackFrameInfo& SFI,
        RTBuilder::SWStackPtrVal *FrameAddr);
    void LowerAllocas(
        Function& F,
        const StackFrameInfo &SFI,
        RTBuilder::SWStackPtrVal *FrameAddr);
    void LowerAllocaNumbers(
        Function& F, Function &RootFunction,
        const StackFrameInfo &SFI, RTBuilder::SWStackPtrVal *FrameAddr);
    void LowerSpillValues(
        Function& F, StackFrameInfo& SFI, RTBuilder::SWStackPtrVal *FrameAddr);
    void PatchSWStackPointers(Function& F, RTBuilder::SWStackPtrVal *Ptr);
    void patchMergeCalls(
        Function&F, RTBuilder::SWStackPtrVal *FrameAddr, Instruction* InsertPt);
    void LowerDispatchDimensions(Function& F);
    void LowerDispatchRayIndex(Function& F, std::vector<Value*> &Indices);
    std::vector<Value*> calcDispatchRayIndex(
        Function& F, const StackFrameInfo &SFI);
    void addOutOfBoundsCheck(
        Function& F,
        const StackFrameInfo& SFI,
        const std::vector<Value*>& Indices);
    void LowerRayInfo(Function& F, CallableShaderTypeMD CallableShaderType);
    void LowerHitKind(
        Function& F,
        RTArgs &SFI,
        RTBuilder::SWStackPtrVal* FrameAddr,
        RTBuilder::StackPointerVal *StackPtr);

    void LowerFillValues(
        Function& F, StackFrameInfo& SFI, RTBuilder::SWStackPtrVal *FrameAddr);
    void mergeContinuationShaders();
    void patchSignposts();
    void patchPayloads();
    void emitMergeCalls(
        Function *F,
        const StackFrameInfo &SFI,
        RTBuilder::StackOffsetIntVal *FrameOffset,
        RTBuilder::StackOffsetPtrVal *StackPtrLocPtr);
    std::tuple<RTBuilder::SWStackPtrVal*,
               RTBuilder::SWStackPtrVal*,
               RTBuilder::StackOffsetPtrVal*,
               RTBuilder::StackOffsetIntVal*,
               RTBuilder::StackPointerVal*,
               RTBuilder::SWHotZonePtrVal*>
        ComputeFrameAddr(Function& F, const StackFrameInfo& SFI);
    Instruction* LowerPayload(
        Function *F, StackFrameInfo &SFI, RTBuilder::SWStackPtrVal *FrameAddr);

    void padSpills(
        ContinuationHLIntrinsic* CHLI,
        const StackFrameInfo& SFI,
        RTBuilder::SWStackPtrVal* FrameAddr,
        RTBuilder &RTB);

    Module* m_module = nullptr;
    RayDispatchShaderContext *m_CGCtx = nullptr;

    // Maps a continuation ID to its corresponding continuation function.
    ContMap m_continuationMappings;
    SmallVector<ContinuationSignpostIntrinsic*, 4> m_Signposts;
    SmallVector<StoreInst*, 4> m_NextFrameStores;
    DenseMap<Function*, SmallSet<uint32_t, 4>> m_ContinuationToPayloadOffset;
    DenseMap<Function*, Function*> m_ContinuationToParent;
    const DataLayout* m_DL = nullptr;
    Function* m_UniqueCont = nullptr;
private:
    // match a basic block that ends:
    // call TraceRayHL(...) | CallShaderHL(...) | TraceRay(...)
    // ret ...
    // returns the 'ret' if there is no call that indicates this will later
    // invoke a continuation.
    ReturnInst* getRetNoContinuation(BasicBlock &BB) const
    {
        auto *RI = dyn_cast<ReturnInst>(BB.getTerminator());
        if (!RI)
            return nullptr;

        if (auto *Prev = RI->getPrevNode())
        {
            if (isa<ContinuationHLIntrinsic>(Prev) || isa<TraceRayIntrinsic>(Prev))
                return nullptr;
        }

        return RI;
    }
private:
    void invokeContinuationBTDStrategy(Value* ContAddr, RTBuilder& IRB);
    void invokeContinuationSwitchStrategy(Value* ContAddr, RTBuilder& IRB);

    bool recordContinuationPayloadOffset(Value* PayloadPtr, Function* Fn);

    static Function* getUniqueCont(const RayDispatchShaderContext* Ctx);
};

char RayTracingIntrinsicLoweringPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "raytracing-intrinsic-lowering"
#define PASS_DESCRIPTION "Lower intrinsics to RTStack operations"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingIntrinsicLoweringPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(RayTracingIntrinsicLoweringPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

Function* RayTracingIntrinsicLoweringPass::getUniqueCont(
    const RayDispatchShaderContext* Ctx)
{
    auto* MMD = Ctx->getModuleMetaData();
    if (!Ctx->tryPayloadSinking())
        return nullptr;

    if (MMD->rtInfo.NumContinuations != 1)
        return nullptr;

    if (!Ctx->canWholeProgramCompile())
        return nullptr;

    if (Ctx->requiresIndirectContinuationHandling() &&
        !Ctx->m_DriverInfo.supportsCallStackHandlerPatching())
    {
        return nullptr;
    }

    Function* ContFn = nullptr;
    for (auto& F : *Ctx->getModule())
    {
        if (F.isDeclaration())
            continue;
        if (RTBuilder::isContinuation(F))
        {
            ContFn = &F;
            break;
        }
    }

    IGC_ASSERT(ContFn);
    // The continuation address won't be patched if is was promoted to a shader
    // identifer.
    if (auto I = MMD->FuncMD.find(ContFn); I != MMD->FuncMD.end())
    {
        if (I->second.rtInfo.SlotNum)
            return nullptr;
    }

    return ContFn;
}

static uint32_t getFrameAlignment(const Value* Ptr, const DataLayout &DL)
{
    // Given a pointer offset from the base of the stack frame, compute the
    // alignment.
    uint64_t Offset = 0;
    auto Region = getRegionOffset(Ptr, &DL, &Offset);
    if (!Region || *Region != RTMemRegion::SWStack)
    {
        IGC_ASSERT_MESSAGE(0, "This shouldn't happen!");
        // be conservative
        return 1;
    }

    return gcd(StackFrameAlign, static_cast<uint32_t>(Offset));
}

// Calculate the byte offset from the base of the stack frame
// to the payload so we can recompute the payload offset in the
// inlined continuation.
bool RayTracingIntrinsicLoweringPass::recordContinuationPayloadOffset(
    Value* PayloadPtr, Function* Fn)
{
    if (!m_CGCtx->tryPayloadSinking())
        return true;

    uint64_t Offset = 0;
    if (auto Region = getRegionOffset(PayloadPtr, m_DL, &Offset))
    {
        if (*Region == RTMemRegion::SWStack)
        {
            m_ContinuationToPayloadOffset[Fn].insert(
                static_cast<uint32_t>(Offset));
            return true;
        }
    }

    return false;
}

// We inject this code at the top of the function to provide access to common
// memory locations (e.g., the async stack pointer) that can then be used by all
// the intrinsics we lower later in the function to avoid recomputing it
// every time (even though it may be CSE'd later on, there is probably some
// compile time savings in doing it this way).
std::tuple<RTBuilder::SWStackPtrVal*,
           RTBuilder::SWStackPtrVal*,
           RTBuilder::StackOffsetPtrVal*,
           RTBuilder::StackOffsetIntVal*,
           RTBuilder::StackPointerVal*,
           RTBuilder::SWHotZonePtrVal*>
RayTracingIntrinsicLoweringPass::ComputeFrameAddr(Function& F, const StackFrameInfo& SFI)
{
    RTBuilder RTB(&*F.getEntryBlock().getFirstInsertionPt(), SFI.Ctx);
    auto* SWHotZonePtr = RTB.getSWHotZonePointer();
    auto* StackPtr = RTB.getAsyncStackPointer();

    RTBuilder::SWStackPtrVal* CurStackVal = nullptr;
    RTBuilder::SWStackPtrVal* FrameAddr = nullptr;
    RTBuilder::StackOffsetIntVal* FrameOffset = nullptr;
    auto* Ptr = RTB.getAddrOfSWStackOffset(SWHotZonePtr);
    auto* CurStackOffset = RTB.getSWStackOffset(Ptr);

    int32_t FrameSize = SFI.getFrameSize();

    if (SFI.isRayGenRoot(&F))
    {
        FrameAddr = RTB.getSWStackPointer(None, FrameSize, false, CurStackVal);
        FrameOffset = RTB.getFirstFrameOffset();
    }
    else if (SFI.isRoot(&F))
    {
        FrameAddr = RTB.getSWStackPointer(
            CurStackOffset, FrameSize, false, CurStackVal);
        FrameOffset = CurStackOffset;
    }
    else if (SFI.isContinuation(&F))
    {
        FrameAddr = RTB.getSWStackPointer(
            CurStackOffset, FrameSize, true, CurStackVal);
        if (m_CGCtx->tryPayloadSinking())
        {
            // Mark the SWStack base with a "signpost" that makes it easy
            // to find it and the payload offset after running the inliner.
            auto* Offset = UndefValue::get(RTB.getInt32Ty());
            auto *Signpost = RTB.getContinuationSignpost(FrameAddr, Offset);
            m_Signposts.push_back(Signpost);
            FrameAddr =
                static_cast<RTBuilder::SWStackPtrVal*>(cast<Value>(Signpost));
        }
        FrameOffset = static_cast<RTBuilder::StackOffsetIntVal*>(
            RTB.CreateSub(
                CurStackOffset,
                RTB.getIntN(
                    CurStackOffset->getType()->getIntegerBitWidth(),
                    FrameSize),
                VALUE_NAME("ResetStackOffset")));
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "unhandled?");
    }

    return std::make_tuple(
        FrameAddr,
        CurStackVal,
        Ptr,
        FrameOffset,
        StackPtr,
        SWHotZonePtr);
}

void RayTracingIntrinsicLoweringPass::patchMergeCalls(
    Function &F, RTBuilder::SWStackPtrVal* FrameAddr, Instruction* InsertPt)
{
    auto& M = *F.getParent();
    auto& C = F.getContext();

    auto* MF = M.getFunction(RTBuilder::MergeFuncName);
    if (!MF)
        return;

    RTBuilder IRB(C, *m_CGCtx);

    auto getReturnIP = [&](Instruction* Pt)
    {
        IRB.SetInsertPoint(Pt);
        static_assert(TraceRayRTArgs::ReturnIPSlot == 0);
        // Payload doesn't really *have* to be here for functionality,
        // but you would be missing out on vectorization if these two aren't
        // contiguous.
        static_assert(TraceRayRTArgs::PayloadSlot == 1);
        if (m_UniqueCont)
        {
            if (m_CGCtx->requiresIndirectContinuationHandling())
            {
                return IRB.computeReturnIP(*m_CGCtx, *m_UniqueCont);
            }
            else
            {
                Value* ID = IRB.getInt64(m_continuationMappings.begin()->first);
                return ID;
            }
        }
        else
        {
            Value* Ptr = IRB.CreateBitCast(
                FrameAddr,
                Type::getInt64PtrTy(C,
                    FrameAddr->getType()->getPointerAddressSpace()));
            Value* ContinuationAddress = IRB.CreateLoad(
                Ptr->getType()->getPointerElementType(),
                Ptr,
                VALUE_NAME("ContinuationAddress"));
            return ContinuationAddress;
        }
    };

    Value* RetIP = nullptr;
    for (auto* U : MF->users())
    {
        IGC_ASSERT(isa<CallInst>(U));
        auto* CI = cast<CallInst>(U);
        if (CI->getFunction() != &F)
            continue;

        if (!RetIP && InsertPt)
            RetIP = getReturnIP(InsertPt);

        Value* CurRetIP = RetIP ? RetIP : getReturnIP(CI);
        CI->setArgOperand(0, CurRetIP);
    }
}

void RayTracingIntrinsicLoweringPass::PatchSWStackPointers(
    Function& F,
    RTBuilder::SWStackPtrVal *Ptr)
{
    for (auto II = inst_begin(&F), E = inst_end(&F); II != E; /* empty */)
    {
        auto* I = &*II++;
        if (auto* SWStackPtr = dyn_cast<SWStackPtrIntrinsic>(I))
        {
            if (auto* C = dyn_cast<Constant>(SWStackPtr->getAddr()))
            {
                if (C->isNullValue())
                {
                    SWStackPtr->replaceAllUsesWith(Ptr);
                    SWStackPtr->eraseFromParent();
                }
            }
        }
    }
}

//lowering for GenISA_DispatchDimensions - Lowers into a pointer to global buffer
// where the dispatch ray dimensions are stores. Dimension X, Y, Z
void RayTracingIntrinsicLoweringPass::LowerDispatchDimensions(Function& F)
{
    vector<GenIntrinsicInst*> dispatchDimensions;
    for (auto& I : instructions(F))
    {
        if (isa<GenIntrinsicInst>(&I, GenISAIntrinsic::GenISA_DispatchDimensions))
            dispatchDimensions.push_back(cast<GenIntrinsicInst>(&I));
    }

    if (dispatchDimensions.empty())
        return;

    RTBuilder builder(F.getContext(), *m_CGCtx);

    for (auto it : dispatchDimensions)
    {
        builder.SetInsertPoint(it);
        unsigned int index = static_cast<unsigned int>(
            cast<ConstantInt>(it->getOperand(0))->getZExtValue());
        Value* newVal = builder.getDispatchRayDimension(index);
        it->replaceAllUsesWith(newVal);
    }

    for (auto it : dispatchDimensions)
    {
        it->eraseFromParent();
    }
}

//Lowering for GenISA_DispatchRayIndex - lowers into a load from SWHotZone.
//where the Dispatch Ray index is stored.
void RayTracingIntrinsicLoweringPass::LowerDispatchRayIndex(
    Function& F,
    std::vector<Value*> &Indices)
{
    vector<GenIntrinsicInst*> rayIndexes;
    for (auto& I : instructions(F))
    {
        if (isa<GenIntrinsicInst>(&I, GenISAIntrinsic::GenISA_DispatchRayIndex))
            rayIndexes.push_back(cast<GenIntrinsicInst>(&I));
    }

    if (rayIndexes.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);
    auto* SWHotZonePtr = builder.getSWHotZonePointer();
    if (Indices.empty())
    {
        for (uint32_t i = 0; i < 3; i++)
        {
            Indices.push_back(
                builder.getDispatchRayIndex(SWHotZonePtr, i));
        }
    }

    if (m_CGCtx->LogMgr.isEnabled())
    {
        RTBuilder::InsertPointGuard Guard(builder);
        builder.SetInsertPoint(rayIndexes.back()->getNextNode());
        builder.printDispatchRayIndex(Indices);
    }

    for (auto it : rayIndexes)
    {
        unsigned int dim = static_cast<unsigned int>(
            cast<ConstantInt>(it->getOperand(0))->getZExtValue());
        IGC_ASSERT_MESSAGE((dim < 3), "out of range!");

        it->replaceAllUsesWith(Indices[dim]);
    }

    for (auto it : rayIndexes)
    {
        it->eraseFromParent();
    }
}

Value* RayTracingIntrinsicLoweringPass::GetRayInfo(
    DISPATCH_SHADER_RAY_INFO_TYPE infoKind,
    unsigned int dim,
    RTBuilder& builder,
    CallableShaderTypeMD CallableShaderType,
    Instruction* I)
{
    auto* perLaneStackPtr = builder.getAsyncStackPointer();
    Value* info = nullptr;

    switch (infoKind)
    {
    case WORLD_RAY_ORG:
        info = builder.getWorldRayOrig(perLaneStackPtr, dim);
        break;
    case WORLD_RAY_DIR:
        info = builder.getWorldRayDir(perLaneStackPtr, dim);
        break;
    case OBJ_RAY_ORG:
        info = builder.getObjRayOrig(perLaneStackPtr, dim, CallableShaderType);
        break;
    case OBJ_RAY_DIR:
        info = builder.getObjRayDir(perLaneStackPtr, dim, CallableShaderType);
        break;
    case RAY_T_MIN:
        info = builder.getRayTMin(perLaneStackPtr);
        break;
    case RAY_T_CURRENT:
        info = builder.getRayTCurrent(perLaneStackPtr, CallableShaderType);
        break;
    case INSTANCE_ID:
    case INSTANCE_INDEX:
        info = builder.getInstance(perLaneStackPtr, infoKind, CallableShaderType, nullptr, false);
        break;
    case PRIMITIVE_INDEX_TRIANGLE:
    case PRIMITIVE_INDEX_PROCEDURAL:
        {NodeType nodeType =
            infoKind == PRIMITIVE_INDEX_PROCEDURAL ?
            NODE_TYPE_PROCEDURAL :
            NODE_TYPE_QUAD;
        info = builder.getPrimitiveIndex(perLaneStackPtr, I, builder.getInt32(nodeType), CallableShaderType, false);
        break;
        }
    case RAY_FLAGS:
        info = builder.getRayFlags(perLaneStackPtr);
        info = builder.CreateZExt(info, builder.getInt32Ty());
        break;
    case OBJECT_TO_WORLD:
        info = builder.getObjToWorld(perLaneStackPtr, dim, CallableShaderType);
        break;
    case WORLD_TO_OBJECT:
        info = builder.getWorldToObj(perLaneStackPtr, dim, CallableShaderType);
        break;
    case GEOMETRY_INDEX_TRIANGLE:
    case GEOMETRY_INDEX_PROCEDURAL:
        {
            NodeType nodeType =
                infoKind == GEOMETRY_INDEX_PROCEDURAL ?
                NODE_TYPE_PROCEDURAL :
                NODE_TYPE_QUAD;
            info = builder.getGeometryIndex(perLaneStackPtr, I, builder.getInt32(nodeType), CallableShaderType, false);
            break;
        }
    default:
        IGC_ASSERT_MESSAGE(0, "Unsupported Ray Info");
        break;
    }

    return info;
}

//Lowering for GenISA_WorldRayInfo - lowers into a load from the RTStack header
//where the Info for current ray is stored
void RayTracingIntrinsicLoweringPass::LowerRayInfo(Function& F, CallableShaderTypeMD CallableShaderType)
{
    vector<RayInfoIntrinsic*> RayInfoList;
    for (auto &I : instructions(F))
    {
        if (auto *RIQ = dyn_cast<RayInfoIntrinsic>(&I))
            RayInfoList.push_back(RIQ);
    }

    RTBuilder builder(F.getContext(), *m_CGCtx);

    for (auto *it : RayInfoList)
    {
        builder.SetInsertPoint(it);

        auto infoKind = it->getInfoKind();
        unsigned int dim      = it->getDim();
        Value* rayInfo = GetRayInfo(infoKind, dim, builder, CallableShaderType, &*it);
        IGC_ASSERT_MESSAGE(rayInfo->getType()->getTypeID() == it->getType()->getTypeID(), "Inconsistent type?");
        it->replaceAllUsesWith(rayInfo);
    }

    for (auto it : RayInfoList)
    {
        it->eraseFromParent();
    }
}

void RayTracingIntrinsicLoweringPass::LowerHitKind(
    Function& F,
    RTArgs &Args,
    RTBuilder::SWStackPtrVal* FrameAddr,
    RTBuilder::StackPointerVal* StackPtr)
{
    vector<HitKindIntrinsic*> HKs;
    for (auto &I : instructions(F))
    {
        if (auto *HK = dyn_cast<HitKindIntrinsic>(&I))
            HKs.push_back(HK);
    }

    if (HKs.empty())
        return;

    RTBuilder builder(F.getContext(), *m_CGCtx);

    for (auto *HK : HKs)
    {
        builder.SetInsertPoint(HK);
        Value* Val = Args.isProcedural() ?
            builder.getProceduralHitKind(Args, FrameAddr) :
            builder.getTriangleHitKind(StackPtr, Args.FuncType);
        HK->replaceAllUsesWith(Val);
    }

    for (auto it : HKs)
    {
        it->eraseFromParent();
    }
}

// This injects bounds checking of the dispatch ray index at the top of raygen
// shaders to exit out for rays whose indices exceed the dispatch bounds.
// For example, say that have a HW thread with 8 active lanes.  If the
// application specified a dispatch of 14x1x1, we would fire off two HW threads
// to service those 14 rays.  The second HW thread would only be responsible
// for 6 lanes.  With this check, two of the lanes will exist early (i.e., they
// just do the out-of-bounds check then release their stack ID) and the other
// 6 will continue on as usual.
void RayTracingIntrinsicLoweringPass::addOutOfBoundsCheck(
    Function& F,
    const StackFrameInfo& SFI,
    const std::vector<Value*>& Indices)
{
    // Only need to check at the beginning of the raygen shader
    if (!SFI.isRayGenRoot(&F))
        return;

    auto& C = F.getContext();

    auto* SplitPt = RTBuilder::getEntryFirstInsertionPt(F, &Indices);
    auto& EntryBB = F.getEntryBlock();

    auto* ShaderEntryBB = EntryBB.splitBasicBlock(
        SplitPt, VALUE_NAME("ShaderEntry"));
    EntryBB.getTerminator()->eraseFromParent();

    auto* ExitBB = BasicBlock::Create(C, VALUE_NAME("ExitBB"), &F);

    RTBuilder RTB(C, *m_CGCtx);

    RTB.SetInsertPoint(&EntryBB);
    auto* DimX = RTB.getDispatchRayDimension(0);
    auto* DimY = RTB.getDispatchRayDimension(1);

    auto* BoundedX = RTB.CreateICmpULT(Indices[0], DimX);
    auto* BoundedY = RTB.CreateICmpULT(Indices[1], DimY);
    auto* InBounds = RTB.CreateAnd(BoundedX, BoundedY, VALUE_NAME("InBounds"));

    RTB.CreateCondBr(InBounds, ShaderEntryBB, ExitBB);

    RTB.SetInsertPoint(ExitBB);
    RTB.CreateStackIDRelease();
    RTB.CreateRetVoid();
}

//This function loops through all Kernel functions and adds code to compute the
// Ray Index for X, Y, and Z into the RT Stack. This value is then loaded as
// needed by other shaders launched via BTD.
std::vector<Value*> RayTracingIntrinsicLoweringPass::calcDispatchRayIndex(
    Function& F,
    const StackFrameInfo &SFI)
{
    if (!SFI.isRayGenRoot(&F))
        return {};

    // return these so we can use them in LowerDispatchRayIndex() to avoid
    // having to GVN the loads away later.
    std::vector<Value*> Indices(3);

    RTBuilder builder(&*F.getEntryBlock().getFirstInsertionPt(), *m_CGCtx);

    enum class DimName { X, Y, Z };

    const uint32_t TileXDim1D = m_CGCtx->opts().TileXDim1D;
    const uint32_t TileYDim1D = m_CGCtx->opts().TileYDim1D;
    const uint32_t TileXDim2D = m_CGCtx->opts().TileXDim2D;
    const uint32_t TileYDim2D = m_CGCtx->opts().TileYDim2D;

    uint32_t SubtileXDim2D = 0;
    uint32_t SubtileYDim2D = 0;
    if (IGC_IS_FLAG_ENABLED(EnableRayTracingCustomSubtile))
    {
        SubtileXDim2D = IGC_GET_FLAG_VALUE(RayTracingCustomSubtileXDim2D);
        SubtileYDim2D = IGC_GET_FLAG_VALUE(RayTracingCustomSubtileYDim2D);
    }

    // This should be in sync with selectRTTileLayout().
    static_assert(selectRTTileLayout(16, 1, 2) == RT_TILE_LAYOUT::_1D);
    auto* DispYDim = builder.getDispatchRayDimension(1);
    auto* Do1D = builder.CreateICmpEQ(
        DispYDim, builder.getInt32(1), VALUE_NAME("Do1D"));

    auto emitCode = [&](DimName D)
    {
        Value* GroupID = nullptr;
        Value* DimSize = nullptr;
        Value* LocalID = nullptr;
        bool CanShift = false;
        switch (D)
        {
        case DimName::X:
            GroupID = builder.createGroupId(0);
            {
                uint32_t _1DVal = TileXDim1D;
                uint32_t _2DVal = TileXDim2D;
                if (iSTD::IsPowerOfTwo(TileXDim1D) &&
                    iSTD::IsPowerOfTwo(TileXDim2D))
                {
                    _1DVal = llvm::countTrailingZeros(TileXDim1D);
                    _2DVal = llvm::countTrailingZeros(TileXDim2D);
                    CanShift = true;
                }
                DimSize = builder.CreateSelect(Do1D,
                    builder.getInt32(_1DVal),
                    builder.getInt32(_2DVal));
            }
            if (m_CGCtx->canEfficientTile())
            {
                auto* X1D = builder.createTileXOffset(TileXDim1D, 0, 0);
                auto* X2D = builder.createTileXOffset(
                    TileXDim2D, SubtileXDim2D, SubtileYDim2D);
                LocalID = builder.CreateSelect(Do1D, X1D, X2D);
            }
            else
            {
                LocalID = builder.createThreadLocalId(0);
            }
            LocalID = builder.CreateZExt(LocalID, builder.getInt32Ty());
            break;
        case DimName::Y:
            GroupID = builder.createGroupId(1);
            {
                uint32_t _1DVal = TileYDim1D;
                uint32_t _2DVal = TileYDim2D;
                if (iSTD::IsPowerOfTwo(TileYDim1D) &&
                    iSTD::IsPowerOfTwo(TileYDim2D))
                {
                    _1DVal = llvm::countTrailingZeros(TileYDim1D);
                    _2DVal = llvm::countTrailingZeros(TileYDim2D);
                    CanShift = true;
                }
                DimSize = builder.CreateSelect(Do1D,
                    builder.getInt32(_1DVal),
                    builder.getInt32(_2DVal));
            }
            if (m_CGCtx->canEfficientTile())
            {
                auto* Y1D = builder.createTileYOffset(TileXDim1D, 0, 0);
                auto* Y2D = builder.createTileYOffset(
                    TileXDim2D, SubtileXDim2D, SubtileYDim2D);
                LocalID = builder.CreateSelect(Do1D, Y1D, Y2D);
            }
            else
            {
                LocalID = builder.createThreadLocalId(1);
            }
            LocalID = builder.CreateZExt(LocalID, builder.getInt32Ty());
            break;
        case DimName::Z:
        {
            GroupID = builder.createGroupId(2);
            DimSize = builder.getInt32(1);
            LocalID = builder.getInt32(0);
            break;
        }
        default:
            IGC_ASSERT_MESSAGE(0, "unknown value?");
            break;
        }

        Value* Val = CanShift ?
            builder.CreateShl(GroupID, DimSize) :
            builder.CreateMul(GroupID, DimSize);
        Val = builder.CreateAdd(
            Val,
            LocalID,
            VALUE_NAME("DispatchRayIndex[" + Twine((uint32_t)D) + "]"));

        Indices[(uint32_t)D] = Val;
    };

    if (m_CGCtx->m_DriverInfo.supportsRaytracingTiling())
    {
        emitCode(DimName::X);
        emitCode(DimName::Y);
        emitCode(DimName::Z);
    }
    else
    {
        // This is the fallback path until we have the payload populated.

        // TODO: Optimizations to consider:
        // 1) when compiling RTPSO, analyze usage of DispatchRaysIndex()
        //    and only write out the values that are actually requested.
        // 2) Right now we lay out the group along the X-axis.  It may
        //    be better to tile it or go along a different dimension.
        //    For example, what if the app iterates over the y-axis
        //    and z-axis?

        //Store Ray Index X
        Value* XVal = builder.CreateMul(
            builder.createGroupId(0), builder.getSimdSize());
        XVal = builder.CreateAdd(
            XVal, builder.get32BitLaneID(), VALUE_NAME("DispatchRayIndex[0]"));
        Indices[0] = XVal;

        //Store Ray Index Y
        Value* YVal = builder.createGroupId(
            1, VALUE_NAME("DispatchRayIndex[1]"));
        Indices[1] = YVal;

        //Store Ray Index Z
        Value* ZVal = builder.createGroupId(
            2, VALUE_NAME("DispatchRayIndex[2]"));
        Indices[2] = ZVal;
    }

    return Indices;
}

//Lowering of GenISA_SpillValue intrisic. for every trace, it is most likely to have
//a continuation shader since the call to trace is asynchronous. values that are needed after the trace
// in the continuation shader are converted into a spill intrinsic. This intrisic is then lowered into a store
//in the RTStack, so that it can later be loaded from it as part of the continuation shader.
// Returns the spill size.
void RayTracingIntrinsicLoweringPass::LowerSpillValues(
    Function& F, StackFrameInfo &SFI, RTBuilder::SWStackPtrVal *FrameAddr)
{
    RTBuilder builder(F.getContext(), *m_CGCtx);

    for (auto &BB : F)
    {
        for (auto II = BB.begin(), IE = BB.end(); II != IE; /* empty */)
        {
            auto *I = &*II++;
            auto *Spill = dyn_cast<SpillValueIntrinsic>(I);
            if (!Spill)
                continue;

            builder.SetInsertPoint(Spill);

            Value* Ptr = SFI.getSpillPtr(
                builder,
                FrameAddr,
                Spill,
                VALUE_NAME("&spill." + Spill->getData()->getName()));

            IGCLLVM::Align Align(getFrameAlignment(Ptr, *m_DL));
            builder.CreateAlignedStore(Spill->getData(), Ptr, Align);

            Spill->eraseFromParent();
        }
    }
}

//Lowering of GenISA_FillValue intrisic. For every SpillValue, there is a matching
//FillValue. FillValue is lowered here into a load from the RTStack
void RayTracingIntrinsicLoweringPass::LowerFillValues(
    Function &F,  StackFrameInfo &SFI, RTBuilder::SWStackPtrVal *FrameAddr)
{
    RTBuilder builder(F.getContext(), *m_CGCtx);

    for (auto &BB : F)
    {
        for (auto II = BB.begin(), IE = BB.end(); II != IE; /* empty */)
        {
            auto *I = &*II++;
            auto *Fill = dyn_cast<FillValueIntrinsic>(I);
            if (!Fill)
                continue;

            builder.SetInsertPoint(Fill);

            Value* Ptr = SFI.getFillPtr(
                builder,
                FrameAddr,
                Fill,
                VALUE_NAME("&fill." + Fill->getName()));

            IGCLLVM::Align Align(getFrameAlignment(Ptr, *m_DL));
            Value* fillData = builder.CreateAlignedLoad(Ptr, Align);
            fillData->takeName(Fill);

            Fill->replaceAllUsesWith(fillData);
            Fill->eraseFromParent();
        }
    }
}

void RayTracingIntrinsicLoweringPass::padSpills(
    ContinuationHLIntrinsic* CHLI,
    const StackFrameInfo& SFI,
    RTBuilder::SWStackPtrVal* FrameAddr,
    RTBuilder &RTB)
{
    auto SpillSize = RTBuilder::getSpillSize(*CHLI);
    if (!SpillSize)
        return;

    uint32_t PadStartOffset = SFI.getSpillOffset() + *SpillSize;
    uint32_t Size = SFI.getFrameSize() - PadStartOffset;
    if (Size == 0 || Size % 4 != 0)
        return;
    uint32_t NumDWs = Size / 4;

    uint32_t Addrspace = FrameAddr->getType()->getPointerAddressSpace();
    auto *NewFrameAddr =
        RTB.CreateBitCast(FrameAddr, RTB.getInt8PtrTy(Addrspace));
    auto* Anchor = RTB.getSpillAnchor(UndefValue::get(RTB.getInt32Ty()));
    for (uint32_t i = 0; i < NumDWs; i++)
    {
        uint32_t CurOffset = PadStartOffset + 4 * i;
        auto* Addr = RTB.CreateGEP(NewFrameAddr, RTB.getInt32(CurOffset));
        Addr = RTB.CreateBitCast(Addr, RTB.getInt32PtrTy(Addrspace));
        RTB.CreateStore(Anchor, Addr);
    }
}

// Lowering of GenISA_TraceRayAsyncHL. This function does quite a bit of
// work which can be summarized by this step:
//   1.Copy all relevant information from TraceRay() into the RTStack Header
//       * Ray Info, BVH pointer, HitGroup, Miss group, Flags
//   2.Copies return IP into RTStack
//   3.Copies Payload into RTStack
//   4.Updates current stack pointer to point to the beginning of next function
void RayTracingIntrinsicLoweringPass::LowerTraces(
    Function& F, StackFrameInfo &SFI,
    const std::vector<Value*> &Indices,
    RTBuilder::SWStackPtrVal* FrameAddr,
    RTBuilder::SWStackPtrVal* CurStackVal,
    RTBuilder::StackOffsetPtrVal* StackPtrLocPtr,
    RTBuilder::StackOffsetIntVal* FrameOffset,
    RTBuilder::SWHotZonePtrVal* SWHotZonePtr)
{
    vector<TraceRayAsyncHLIntrinsic*> traceCalls;
    for (auto &I : instructions(F))
    {
        if (auto *II = dyn_cast<TraceRayAsyncHLIntrinsic>(&I))
            traceCalls.push_back(II);
    }

    for (auto *trace : traceCalls)
    {
        RTBuilder builder(trace, *m_CGCtx);
        padSpills(trace, SFI, FrameAddr, builder);

        builder.printTraceRay(trace);

        // Write the stack
        auto* const StackPointer = builder.getAsyncStackPointer();

        // 1. First, let's fill up the data in the MemRay struct

        // Store Ray info.  Will be vectorized later on.
        {
            Value* Vec = UndefValue::get(
                IGCLLVM::FixedVectorType::get(
                    builder.getFloatTy(), RTStackFormat::RayInfoSize));
            for (unsigned int i = 0; i < RTStackFormat::RayInfoSize; i++)
                Vec = builder.CreateInsertElement(Vec, trace->getRayInfo(i), i);

            auto* Ptr = builder.getRayInfoPtr(StackPointer, 0, TOP_LEVEL_BVH);
            uint32_t Addrspace = Ptr->getType()->getPointerAddressSpace();
            Ptr = builder.CreateBitCast(
                Ptr, Vec->getType()->getPointerTo(Addrspace));

            builder.CreateStore(Vec, Ptr);
        }

        // rootNodeptr + rayflags
        {
            Value* rootNodePtr = builder.getRootNodePtr(trace->getBVH());
            // uint64_t rootNodePtr : 48;  // root node to start traversal at
            // uint64_t rayFlags    : 16;  // ray flags (see RayFlag structure)
            //Need to "or" the TraceRay flags with Pipeline flags passed on PSO input
            Value* pipelineFlagsPlusRayFlags = builder.CreateOr(
                trace->getFlag(),
                builder.getInt32(m_CGCtx->pipelineConfig.pipelineFlags));
            pipelineFlagsPlusRayFlags = builder.CreateAnd(
                pipelineFlagsPlusRayFlags,
                builder.getInt32(RTStackFormat::RayFlagsMask));
            Value* rootNodePtrPlusFlags = builder.CreateOr(
                builder.CreateShl(
                    builder.CreateZExt(pipelineFlagsPlusRayFlags, builder.getInt64Ty()),
                    builder.getInt64((uint32_t)MemRay::Bits::rootNodePtr)),
                rootNodePtr,
                VALUE_NAME("rootNodePtrAndRayFlags"));
            builder.setNodePtrAndFlags(StackPointer, rootNodePtrPlusFlags);
        }

        // hitgroupPtr + hitgroupStride
        {
            Value* hitGroupBasePtr = builder.getpHitGroupBasePtr();
            Value* hitGroupStride = builder.getpHitGroupStride();
            // For RayContribToHitGroupIndex only 4 least-significant bits are used
            Value* rayContribToHitGroupIndex = builder.CreateAnd(
                trace->getRayContributionToHitGroupIndex(), builder.getInt32(0xF));

            // ray[0].hitGroupSRBasePtr = RTDispatchGlobals.hitGroupBasePtr +
            //     RTDispatchGlobals.hitGroupStride *
            //     RayContribToHitGroupIndex; // from TraceRay
            // ray[0].hitGroupSRStride = RTDispatchGlobals.hitGroupStride;
            // ray[0].shaderIndexMultiplier =
            //     MultiplierForGeometryContributionToHitGroupIndex; // from TraceRay
            hitGroupBasePtr = builder.CreateAdd(
                hitGroupBasePtr,
                builder.CreateZExt(
                    builder.CreateMul(
                        hitGroupStride,
                        rayContribToHitGroupIndex),
                    builder.getInt64Ty()), VALUE_NAME("hitGroupBasePtr"));

            // base of hit group shader record array (8-bytes alignment)
            // uint64_t hitGroupSRBasePtr : 48;
            // stride of hit group shader record array (8-bytes alignment)
            // uint64_t hitGroupSRStride : 16;
            constexpr uint64_t Mask =
                QWBITMASK((uint32_t)MemRay::Bits::hitGroupSRBasePtr);
            Value* hitgroupInfo = builder.CreateOr(
                builder.CreateShl(
                    builder.CreateZExt(hitGroupStride, builder.getInt64Ty()),
                    builder.getInt64((uint32_t)MemRay::Bits::hitGroupSRBasePtr)),
                builder.CreateAnd(hitGroupBasePtr, builder.getInt64(Mask)));
            builder.setHitGroupPtrAndStride(StackPointer, hitgroupInfo);
        }

        // missSRPtr
        {
            Value* MissShaderPtr = builder.getpMissShaderBasePtr();
            Value* missShaderStride = builder.getpMissShaderStride();
            // For miss shader index only 16 least-significant bits are used
            Value* missShaderIndex = builder.CreateAnd(
                trace->getMissShaderIndex(), builder.getInt32(0xFFFF));
            // For multiplier only 4 least-significant bits are used
            Value* multiplierForGeometryContributionToHitGroupIndex = builder.CreateAnd(
                trace->getMultiplierForGeometryContributionToHitGroupIndex(),
                builder.getInt32(0xF));

            // pointer to miss shader record to invoke on a miss (8-bytes alignment)
            // uint64_t missSRPtr : 48;
            // uint64_t pad : 1;
            // uint64_t shaderIndexMultiplier : 8; // shader index multiplier
            Value* missShaderOffset = builder.CreateMul(
                missShaderIndex,
                missShaderStride,
                VALUE_NAME("missShaderOffset"));
            MissShaderPtr = builder.CreateAdd(
                MissShaderPtr,
                builder.CreateZExt(missShaderOffset, builder.getInt64Ty()),
                VALUE_NAME("missShaderPtr"));
            // set the multipler as the upper 8 bits
            constexpr uint64_t Mask =
                QWBITMASK((uint32_t)MemRay::Bits::missSRPtr);
            MissShaderPtr = builder.CreateOr(
                builder.CreateAnd(MissShaderPtr, builder.getInt64(Mask)),
                builder.CreateShl(
                    builder.CreateZExt(
                        multiplierForGeometryContributionToHitGroupIndex,
                        builder.getInt64Ty()),
                    builder.getInt64((uint32_t)MemRay::Offset::shaderIndexMultiplier),
                    VALUE_NAME("shaderIndexMultiplier")));

            builder.setMissShaderPtr(StackPointer, MissShaderPtr);
        }

        //RayMask
        // the pointer to instance leaf in case we traverse an
        // instance (64-bytes alignment)
        // uint64_t instLeafPtr : 48; // rayFlags go here for bvhLevel = 0
        // uint64_t rayMask : 8; // ray mask used for ray masking
        {
            auto* Flags = builder.CreateAnd(
                trace->getFlag(), builder.getInt32(RTStackFormat::RayFlagsMask));
            Value* maskPlusInstLeafPtr = builder.CreateOr(
                builder.CreateShl(
                    builder.CreateZExt(Flags, builder.getInt64Ty()),
                    builder.getInt64((uint32_t)MemRay::Offset::rayFlagsCopy)),
                builder.CreateShl(
                    builder.CreateZExt(trace->getMask(), builder.getInt64Ty()),
                    builder.getInt64((uint32_t)MemRay::Offset::rayMask)),
                VALUE_NAME("maskPlusIntLeafPtr"));
            builder.setInstLeafPtrAndRayMask(StackPointer, maskPlusInstLeafPtr);
        }

        // the only stores of the dispatch ray indices happen in the raygen
        // root.  Do it at the last moment right before a TraceRay().
        if (SFI.isRayGenRoot(&F))
            builder.setDispatchRayIndices(SWHotZonePtr, Indices);

        // 2. Setup inputs for callee (i.e., the continuation)

        // If the TraceRay() is in a continuation, the value in memory for the
        // stack pointer is already pointing to the next frame so we don't need
        // to push it down.  In a root function, it is pointing to the top of
        // the frame so we have to push it down.
        auto *NewStackPtrVal = SFI.isContinuation(&F) ? CurStackVal :
            builder.bumpStackPtr(
                FrameAddr,
                SFI.getFrameSize(),
                FrameOffset,
                StackPtrLocPtr);

        // Now write the return IP and payload into the start of the next
        // stack frame.

        // If we're in a continuation that writes itself as the return
        // continuation, we know that same value must already be in that stack
        // slot because the only way to have reached this continuation in the
        // first place was for another shader to read that stack slot to get
        // here.
        bool WriteAddr = IGC_IS_FLAG_ENABLED(DisableRTStackOpts) ||
            (!SFI.isContinuation(&F) || trace->getContinuationFn() != &F);

        if (!m_UniqueCont && WriteAddr)
        {
            // Store the continuation ID/address associated with this trace call.
            builder.storeContinuationAddress(
                SFI, trace->getPayload()->getType(), trace, NewStackPtrVal);
        }

        bool Ok = recordContinuationPayloadOffset(
            trace->getPayload(), trace->getContinuationFn());
        IGC_ASSERT(Ok); // This is a performance assert

        auto Stores =
            builder.storePayload(SFI, trace->getPayload(), NewStackPtrVal);
        m_NextFrameStores.append(Stores.begin(), Stores.end());

        //Initial BVH level is zero
        builder.createASyncTraceRay(
            TOP_LEVEL_BVH,
            TraceRayMessage::TRACE_RAY_INITIAL,
            VALUE_NAME("trace_ray_init_payload"));
    }

    for (auto it : traceCalls)
    {
        it->eraseFromParent();
    }
}


void RayTracingIntrinsicLoweringPass::LowerProceduralHitAttributes(
    Function& F,
    StackFrameInfo& SFI,
    RTBuilder::SWStackPtrVal* FrameAddr)
{
    switch (SFI.FuncType)
    {
    case ClosestHit:
    case AnyHit:
    {
        if (SFI.isProcedural())
            RTBuilder::loadCustomHitAttribsFromStack(F, SFI, FrameAddr);
        break;
    }
    default:
        break;
    }
}

// Conceptually similar to TraceRay() lowering, we also update the frame ptr
// and args but emit a send.btd rather than a send.rta.
void RayTracingIntrinsicLoweringPass::LowerCallShaders(
    Function& F,
    StackFrameInfo& SFI,
    const std::vector<Value*>& Indices,
    RTBuilder::SWStackPtrVal* FrameAddr,
    RTBuilder::SWStackPtrVal* CurStackVal,
    RTBuilder::StackOffsetPtrVal* StackPtrLocPtr,
    RTBuilder::StackOffsetIntVal* FrameOffset,
    RTBuilder::SWHotZonePtrVal* SWHotZonePtr)
{
    vector<CallShaderHLIntrinsic*> callShaderCalls;
    for (auto& I : instructions(F))
    {
        if (auto * II = dyn_cast<CallShaderHLIntrinsic>(&I))
            callShaderCalls.push_back(II);
    }

    for (auto call : callShaderCalls)
    {
        RTBuilder builder(call, *m_CGCtx);
        padSpills(call, SFI, FrameAddr, builder);

        // the only stores of the dispatch ray indices happen in the raygen
        // root.  Do it at the last moment right before a TraceRay().
        if (SFI.isRayGenRoot(&F))
            builder.setDispatchRayIndices(SWHotZonePtr, Indices);

        //Bump the stack pointer to the next stack frame
        auto *NewStackPtrVal = SFI.isContinuation(&F) ? CurStackVal :
            builder.bumpStackPtr(
                FrameAddr,
                SFI.getFrameSize(),
                FrameOffset,
                StackPtrLocPtr);

        if (!m_UniqueCont)
        {
            //Store continuation address/ID. Serves as a return IP
            builder.storeContinuationAddress(
                SFI, call->getParameter()->getType(), call, NewStackPtrVal);
        }

        bool Ok = recordContinuationPayloadOffset(
            call->getParameter(), call->getContinuationFn());
        IGC_ASSERT(Ok); // This is a performance assert

        auto Stores =
            builder.storePayload(SFI, call->getParameter(), NewStackPtrVal);
        m_NextFrameStores.append(Stores.begin(), Stores.end());

        //Get Address to Shader being invoked
        Value* callableShaderPtr = builder.getpCallableShaderBasePtr();
        Value* callableShaderStride = builder.getpCallableShaderStride();

        Value* callableShaderOffset = builder.CreateMul(
            call->getShaderIndex(),
            callableShaderStride);
        Value* callableShaderAddress = builder.CreateAdd(
            callableShaderPtr,
            builder.CreateZExt(callableShaderOffset, builder.getInt64Ty()));

        builder.CreateBTDCall(callableShaderAddress);
    }

    for (auto call : callShaderCalls)
    {
        call->eraseFromParent();
    }
}

//This function lowers allocas into pointers to the RTStack
void RayTracingIntrinsicLoweringPass::LowerAllocas(
    Function& F, const StackFrameInfo &SFI, RTBuilder::SWStackPtrVal *FrameAddr)
{
    vector<AllocaInst*> allocas;
    BasicBlock* entryBlock = &F.getEntryBlock();

    for (auto& II : *entryBlock)
    {
        if (auto* Alloca = dyn_cast<AllocaInst>(&II))
        {
            IGC_ASSERT_MESSAGE(SFI.isRoot(&F), "alloca in continuation?");
            // We place allocas with non-default address space on the RTStack.
            // The rest will go to scratch.
            if (RTBuilder::isNonLocalAlloca(Alloca))
                allocas.push_back(Alloca);
        }
    }

    if (allocas.empty())
        return;

    RTBuilder builder(F.getContext(), *m_CGCtx);

    for (auto *AI : allocas)
    {
        builder.SetInsertPoint(AI);

        auto* Ptr = SFI.getAllocaPtr(
            builder,
            FrameAddr,
            AI,
            VALUE_NAME("&alloca." + AI->getName()));

        // Replace all uses of original alloca with the RTStack buffer
        AI->replaceAllUsesWith(Ptr);
    }

    for (auto it : allocas)
    {
        it->eraseFromParent();
    }
}

// Lower these temporary intrinsics create from LateRemat to their corresponding
// frame addresses.
void RayTracingIntrinsicLoweringPass::LowerAllocaNumbers(
    Function& F, Function &RootFunction,
    const StackFrameInfo& SFI, RTBuilder::SWStackPtrVal* FrameAddr)
{
    SmallVector<AllocaNumberIntrinsic*, 4> AllocaNumbers;
    BasicBlock* entryBlock = &F.getEntryBlock();

    for (auto& II : *entryBlock)
    {
        if (auto* AllocaNumber = dyn_cast<AllocaNumberIntrinsic>(&II))
            AllocaNumbers.push_back(AllocaNumber);
    }

    if (AllocaNumbers.empty())
        return;

    auto NumberMap = RTBuilder::getNumberAllocaMap(RootFunction);

    RTBuilder builder(AllocaNumbers[0], *m_CGCtx);

    for (auto *AN : AllocaNumbers)
    {
        builder.SetInsertPoint(AN);

        auto I = NumberMap.find((uint32_t)AN->getNumber());
        IGC_ASSERT_MESSAGE((I != NumberMap.end()), "missing association?");
        auto* AI = I->second;

        Value* Ptr = SFI.getAllocaPtr(
            builder,
            FrameAddr,
            AI,
            VALUE_NAME("&alloca." + AI->getName()));

        // Replace all uses of original alloca number with the RTStack buffer
        AN->replaceAllUsesWith(Ptr);
    }

    for (auto it : AllocaNumbers)
    {
        it->eraseFromParent();
    }
}

Instruction* RayTracingIntrinsicLoweringPass::LowerPayload(
    Function* F, StackFrameInfo& SFI, RTBuilder::SWStackPtrVal* FrameAddr)
{
    auto *PayloadPtr = RTBuilder::LowerPayload(F, SFI, FrameAddr);
    if (!PayloadPtr)
        return nullptr;

    // we mark the payload pointer so it is easier to do payload sinking later
    // on. We should only do this for root functions.

    if (!m_CGCtx->tryPayloadSinking() || !SFI.isRoot(F))
        return PayloadPtr;

    RTBuilder RTB(PayloadPtr->getNextNode(), *m_CGCtx);

    auto* II = RTB.getPayloadPtrIntrinsic(
        UndefValue::get(PayloadPtr->getType()),
        FrameAddr);
    PayloadPtr->replaceAllUsesWith(II);
    II->setPayloadPtr(PayloadPtr);
    return II;
}

// Now that we've computed all payload offsets in their stack frames, we need
// to update all of the signposts we created with their offsets. If at least
// one offset could not be determined, we don't sink.
void RayTracingIntrinsicLoweringPass::patchSignposts()
{
    bool Ok = true;
    for (auto* Post : m_Signposts)
    {
        auto I = m_ContinuationToPayloadOffset.find(Post->getFunction());
        if (I == m_ContinuationToPayloadOffset.end() || I->second.size() != 1)
        {
            Ok = false;
            break;
        }

        Post->setOffset(*I->second.begin());
    }

    if (!Ok)
    {
        for (auto* Post : m_Signposts)
        {
            Post->replaceAllUsesWith(Post->getFrameAddr());
            Post->eraseFromParent();
        }
    }
}

void RayTracingIntrinsicLoweringPass::patchPayloads()
{
    if (!m_UniqueCont)
        return;

    auto I = m_ContinuationToPayloadOffset.find(m_UniqueCont);
    if (I == m_ContinuationToPayloadOffset.end())
        return;
    if (I->second.size() != 1)
        return;

    const uint32_t Offset = *I->second.begin();

    Function* Root = m_ContinuationToParent.find(m_UniqueCont)->second;
    auto* MMD = m_CGCtx->getModuleMetaData();
    uint32_t StackSize = MMD->FuncMD.find(Root)->second.rtInfo.ShaderStackSize;

    IRBuilder<> IRB(Root->getContext());

    // Address = FrameAddr - StackSize + Offset
    //         = FrameAddr - (StackSize - Offset)

    IGC_ASSERT(StackSize > Offset);
    const uint32_t Dist = StackSize - Offset;

    visitGenIntrinsic(*Root->getParent(), GenISAIntrinsic::GenISA_PayloadPtr,
    [&](GenIntrinsicInst* GII) {
        auto* Payload = cast<PayloadPtrIntrinsic>(GII);
        IRB.SetInsertPoint(Payload);
        auto* FrameAddr = Payload->getFrameAddr();
        uint32_t Addrspace = FrameAddr->getType()->getPointerAddressSpace();
        FrameAddr = IRB.CreateBitCast(FrameAddr, IRB.getInt8PtrTy(Addrspace));
        auto* NewLoc = IRB.CreateGEP(
            IRB.getInt8Ty(), FrameAddr, IRB.getInt32(-int(Dist)));
        NewLoc = IRB.CreateBitCast(NewLoc, Payload->getPayloadPtr()->getType());
        Payload->setPayloadPtr(NewLoc);
    });

    for (auto* SI : m_NextFrameStores)
        SI->eraseFromParent();
}

// Given a retrieved continuation address, BTD there.
void RayTracingIntrinsicLoweringPass::invokeContinuationBTDStrategy(
    Value* ContAddr,
    RTBuilder& IRB)
{
    IRB.CreateBTDCall(ContAddr);
    IRB.CreateRetVoid();
}

// Given a retrieved continuation address, construct a switch that will call
// the given continuation function.
void RayTracingIntrinsicLoweringPass::invokeContinuationSwitchStrategy(
    Value* ContAddr,
    RTBuilder& IRB)
{
    auto* mergedContinuationFunc = IRB.GetInsertBlock()->getParent();
    const uint32_t NumContinuationShaders = m_continuationMappings.size();

    if (NumContinuationShaders == 0)
    {
        IRB.CreateRetVoid();
        return;
    }

    auto& C = IRB.getContext();

    BasicBlock* defaultCase = BasicBlock::Create(
        C, VALUE_NAME("default"), mergedContinuationFunc);
    // It should not be possible to hit the default case of the switch
    // because there should be a valid continuation ID pointing to a
    // continuation function.
    (void) new UnreachableInst(C, defaultCase);

    SwitchInst* contSwitch = IRB.CreateSwitch(
        ContAddr, defaultCase, NumContinuationShaders);

    for (auto& CSI : m_continuationMappings)
    {
        uint32_t Idx = CSI.first;
        Function* F  = CSI.second;

        SmallVector<Value*, 4> Args;

        for (auto& Arg : F->args())
            Args.push_back(UndefValue::get(Arg.getType()));

        BasicBlock* caseBlock = BasicBlock::Create(
            C,
            VALUE_NAME("case" + Twine(Idx)),
            mergedContinuationFunc);
        Instruction* retInst = ReturnInst::Create(C, caseBlock);
        IRB.SetInsertPoint(retInst);
        IRB.CreateCall(F, Args);
        contSwitch->addCase(IRB.getInt64(Idx), caseBlock);
    }
}

//This function creates a new function called __mergeContinuation
//in this new function, there is a switch that will evaluate which
//continuation shader should be called based on the Return IP portion
//of the current function in the RTStack
void RayTracingIntrinsicLoweringPass::mergeContinuationShaders()
{
    Function *mergedContinuationFunc = m_module->getFunction(MergeFuncName);

    if (!mergedContinuationFunc)
        return;

    mergedContinuationFunc->addFnAttr(llvm::Attribute::AttrKind::NoUnwind);

    auto &C = m_module->getContext();
    RTBuilder builder(C, *m_CGCtx);

    BasicBlock* EntryBlock = BasicBlock::Create(
        C, VALUE_NAME("entry"), mergedContinuationFunc);
    builder.SetInsertPoint(EntryBlock);

    IGC_ASSERT_MESSAGE(mergedContinuationFunc->arg_size() == 1, "added?");
    auto* ContinuationAddress = mergedContinuationFunc->arg_begin();
    ContinuationAddress->setName(VALUE_NAME("ContAddr"));

    if (m_CGCtx->requiresIndirectContinuationHandling())
        invokeContinuationBTDStrategy(ContinuationAddress, builder);
    else
        invokeContinuationSwitchStrategy(ContinuationAddress, builder);
}

void RayTracingIntrinsicLoweringPass::emitMergeCalls(
    Function *F,
    const StackFrameInfo &SFI,
    RTBuilder::StackOffsetIntVal *FrameOffset,
    RTBuilder::StackOffsetPtrVal *StackPtrLocPtr)
{
    if (!shaderReturnsToContinuation(SFI.FuncType))
        return;

    RTBuilder builder(F->getContext(), *m_CGCtx);

    // returns that have a TraceRayHL prior will do a BTD.  Other returns
    // actually need to exit and invoke the next continuation.
    for (auto& BB : *F)
    {
        auto* RI = getRetNoContinuation(BB);
        if (!RI)
            continue;

        builder.SetInsertPoint(RI);
        // If we're in a continuation, we haven't yet written the stack pointer
        // back out so it points to the top of this stack frame.  Do it now so
        // the correct continuation is called.
        if (SFI.isContinuation(F))
        {
            builder.writeNewStackOffsetVal(FrameOffset, StackPtrLocPtr);
        }
        builder.createMergeCall();
    }
}

bool RayTracingIntrinsicLoweringPass::runOnModule(Module &M)
{
    m_module = &M;
    m_CGCtx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
    m_continuationMappings.clear();
    m_Signposts.clear();
    m_NextFrameStores.clear();
    m_ContinuationToPayloadOffset.clear();
    m_ContinuationToParent.clear();
    m_DL = &M.getDataLayout();
    m_UniqueCont = getUniqueCont(m_CGCtx);
    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto &FuncMD = modMD->FuncMD;

    vector<Function*> RootFunctions = getRootFunctions(m_CGCtx, M);

    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;
        for (auto& I : instructions(F))
        {
            if (auto* CHLI = dyn_cast<ContinuationHLIntrinsic>(&I))
            {
                m_continuationMappings.insert(std::make_pair(
                    CHLI->getContinuationID(), CHLI->getContinuationFn()));
            }
        }
    }

    for (auto *RootFunction : RootFunctions)
    {
        ContMap Entries = getFuncGroup(RootFunction);

        SmallVector<Function*, 4> FuncGroup;
        for (auto& Entry : Entries)
        {
            Function* ContFn = Entry.second;
            FuncGroup.push_back(ContFn);
            m_ContinuationToParent[ContFn] = RootFunction;
        }

        // Process the root last so their allocas can be scanned by the
        // continuations.
        FuncGroup.push_back(RootFunction);

        auto MD = FuncMD.find(RootFunction);
        IGC_ASSERT_MESSAGE((MD != FuncMD.end()), "Missing metadata?");
        auto CallableShaderType = MD->second.rtInfo.callableShaderType;

        Optional<HIT_GROUP_TYPE> HitGroupTy =
            m_CGCtx->getHitGroupType(RootFunction->getName().str());

        StackFrameInfo SFI(
            RootFunction, CallableShaderType,
            HitGroupTy,
            m_CGCtx, MD->second, MD->second.rtInfo.Types, true);

        for (auto* F : FuncGroup)
        {
            SFI.addFunction(F);

            if (auto I = FuncMD.find(F); I != FuncMD.end())
            {
                auto& rtInfo = I->second.rtInfo;
                if (rtInfo.isContinuation)
                    rtInfo.ParentName = RootFunction->getName().str();
            }
        }

        SFI.finalize();

        for (auto *F : FuncGroup)
        {
            auto [FrameAddr, CurStackVal, StackPtrLocPtr,
                  FrameOffset, StackPtr, SWHotZonePtr] =
                ComputeFrameAddr(*F, SFI);
            PatchSWStackPointers(*F, FrameAddr);
            LowerDispatchDimensions(*F);
            LowerAllocaNumbers(*F, *RootFunction, SFI, FrameAddr);
            LowerAllocas(*F, SFI, FrameAddr);
            LowerSpillValues(*F, SFI, FrameAddr);
            LowerFillValues(*F, SFI, FrameAddr);
            emitMergeCalls(F, SFI, FrameOffset, StackPtrLocPtr);
            auto Indices = calcDispatchRayIndex(*F, SFI);
            LowerTraces(
                *F, SFI, Indices,
                FrameAddr, CurStackVal, StackPtrLocPtr, FrameOffset, SWHotZonePtr);
            LowerCallShaders(
                *F, SFI, Indices,
                FrameAddr, CurStackVal, StackPtrLocPtr, FrameOffset, SWHotZonePtr);
            auto *PayloadPt = LowerPayload(F, SFI, FrameAddr);
            LowerProceduralHitAttributes(*F, SFI, FrameAddr);
            LowerDispatchRayIndex(*F, Indices);
            // These calls may modify the CFG
            LowerRayInfo(*F, SFI.FuncType);
            LowerHitKind(*F, SFI, FrameAddr, StackPtr);
            addOutOfBoundsCheck(*F, SFI, Indices);
            patchMergeCalls(*F, FrameAddr, PayloadPt);
        }

        // Set the stack size for the roots.  Since continuations share the
        // same stack frame with their root, the convention here is to just
        // have the stack size of continuations left as 0.
        RayTraceShaderInfo &Info = MD->second.rtInfo;
        Info.ShaderStackSize = SFI.getFrameSize();
    }

    mergeContinuationShaders();
    patchSignposts();
    patchPayloads();

    DumpLLVMIR(m_CGCtx, "RayTracingIntrinsicLoweringPass");
    return true;
}

namespace IGC
{

Pass* createRayTracingIntrinsicLoweringPass(void)
{
    return new RayTracingIntrinsicLoweringPass();
}

} // namespace IGC
