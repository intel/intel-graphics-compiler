/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass does lowering for intersection and any-hit shader specific
/// intrinsics.  In addition, it also creates the callStackHandler shader.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "RTArgs.h"
#include "RTStackFormat.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/Utils/SSAUpdater.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/InstIterator.h>
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "llvmWrapper/Transforms/Utils/ValueMapper.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include "Utils.h"

using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

class LowerIntersectionAnyHit : public ModulePass
{
public:
    LowerIntersectionAnyHit() : ModulePass(ID)
    {
        initializeLowerIntersectionAnyHitPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "LowerIntersectionAnyHit";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;
private:
    RayDispatchShaderContext *m_CGCtx = nullptr;
    const UnifiedBits* RayFlagBits = nullptr;
    using AnyHitMapTy = std::unordered_map<std::string, Function*>;
private:
    // returns a mapping from the name of an any-hit shader to the
    // corresponding function that can be inlined into an intersection shader.
    AnyHitMapTy getInlineableAnyHitMapping(Module &M);
    Function* createAnyHitFn(Function &F, FunctionMetaData &MD);
    void markAsCallableShader(Function *F, CallableShaderTypeMD Ty) const
    {
        ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
        IGC::FunctionMetaData funcMd;
        funcMd.functionType = FunctionTypeMD::CallableShader;
        funcMd.rtInfo.callableShaderType = Ty;
        modMD->FuncMD.insert(std::make_pair(F, funcMd));

        IGC::IGCMD::IGCMetaDataHelper::addFunction(
            *m_CGCtx->getMetaDataUtils(),
            F,
            FunctionTypeMD::KernelFunction);
    }
    void createCallStackHandler(Module &M);
    void handleIntersectionAnyHitShaders(Module &M);
    void addTraceRayAtEnd(Function &F, Value *TraceRayCtrlLoc);
    void addAnyHitExitCode(Function &F);
    // Generate branching code implementing the ReportHit() intrinsic.
    // Return the call to the any-hit shader that needs to be inlined
    // (nullptr if no such call).
    CallInst* codeGenReportHit(
        RTArgs &RTArgs,
        ReportHitHLIntrinsic *RHI,
        Function *AnyHitShader,
        Value *TraceRayCtrlLoc,
        RTBuilder::SWStackPtrVal* FrameAddr);
    template <typename FnType>
    void invokeClosestHitShader(
        RTBuilder &IRB,
        Value *ShaderRecordPointer,
        Value *RayFlags,
        BasicBlock *BTDBB,
        FnType InitForCHS,
        BasicBlock *SkipBB);
    void codeGenIgnoreHit(GenIntrinsicInst* IHI);
    void codeGenAcceptHitAndEndSearch(GenIntrinsicInst* AHI);
    void initializeCommittedHit(
        RTBuilder& IRB,
        RTBuilder::StackPointerVal* StackPointer,
        Value* THit,
        Value* PotentialPrimVal,
        Value* PotentialInstVal);
    void injectExitCode(
        RTBuilder& IRB,
        RTBuilder::StackPointerVal* StackPointer,
        Value* RayFlags);
    Value* checkFlagBitSet(
        RTBuilder& IRB,
        Value* RayFlags,
        RTStackFormat::RayFlags Flag,
        const Twine &Name = "");
    Function* getAnyHitShader(Function& IntersectionFn, const AnyHitMapTy &Map);
    void lowerTriangleAnyHitIntrinsics(Function& F);
    Value* isClosestHitNull(RTBuilder& IRB, Value* ShaderRecordPointer) const;
    // Examine the hitgroup usages of `F`. If all hitgroups have the same
    // closest-hit shader, return it. Otherwise, if there are differences or
    // no closest-hit shader at all, return None.
    Optional<std::string> getUniqueClosestHit(const Function& F) const;
    void LowerPayload(
        Function *F, RTArgs &Args, RTBuilder::SWStackPtrVal* FrameAddr);
};

enum class ANYHIT_RETURN_CODES
{
    IGNORE_HIT,
    ACCEPT_HIT,
    FALLTHROUGH_EXIT,
};

char LowerIntersectionAnyHit::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "lower-intersection-anyhit"
#define PASS_DESCRIPTION "Creates callStackHandler and lowers intrinsics for intersection and anyhit shaders"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerIntersectionAnyHit, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowerIntersectionAnyHit, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool LowerIntersectionAnyHit::runOnModule(Module& M)
{
    m_CGCtx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());

    auto FlagBits = examineRayFlags(*m_CGCtx);
    RayFlagBits = &FlagBits;

    if (m_CGCtx->isRTPSO())
        createCallStackHandler(M);

    handleIntersectionAnyHitShaders(M);

    return true;
}

// ReportHit() and AcceptHitAndEndSearch() can do an invocation of the
// closest-hit shader by directly doing a BTD to it.  The closest-hit shader
// expects to have stack->committedHit populated.  When doing a TRACE_RAY_COMMIT,
// the potentialHit will be copied over to committedHit.  Since we're skipping
// that step, we need to explicitly populate the fields that could be read
// in the closest-hit shader.
void LowerIntersectionAnyHit::initializeCommittedHit(
    RTBuilder &IRB,
    RTBuilder::StackPointerVal* StackPointer,
    Value* THit,
    Value* PotentialPrimVal,
    Value* PotentialInstVal)
{
    //stack->committedHit.t = THit;
    IRB.setCommittedHitT(StackPointer, THit);
    //stack->committedHit.primLeafPtr = stack->potentialHit.primLeafPtr;
    //stack->committedHit.hitGroupRecPtr0 = stack->potentialHit.hitGroupRecPtr0;
    IRB.setCommittedHitTopPrimLeafPtr(StackPointer, PotentialPrimVal);
    //stack->committedHit.instLeafPtr = stack->potentialHit.instLeafPtr;
    //stack->committedHit.hitGroupRecPtr1 = stack->potentialHit.hitGroupRecPtr1;
    IRB.setCommittedHitTopInstLeafPtr(StackPointer, PotentialInstVal);
}

Value* LowerIntersectionAnyHit::isClosestHitNull(
    RTBuilder& IRB,
    Value* ShaderRecordPointer) const
{
    Value* NullClosestHit = nullptr;
    std::string FnName = IRB.GetInsertBlock()->getParent()->getName().str();
    if (auto* Refs = m_CGCtx->hitgroupRefs(FnName))
    {
        if (llvm::all_of(*Refs, [](HitGroupInfo* H) { return H->ClosestHit; }))
            NullClosestHit = IRB.getFalse();
        else if (llvm::none_of(*Refs, [](HitGroupInfo* H) { return H->ClosestHit; }))
            NullClosestHit = IRB.getTrue();
    }

    if (!NullClosestHit)
    {
        auto* ShaderRecordPtr = IRB.CreateIntToPtr(
            ShaderRecordPointer,
            PointerType::get(IRB.getInt64Ty(), ADDRESS_SPACE_GLOBAL),
            VALUE_NAME("&ShaderRecord[0]"));
        static_assert(offsetof(ShaderIdentifier, ClosestHit) == 0, "KSP moved?");
        static_assert(offsetof(ShaderRecord, ID) == 0, "layout change?");
        auto* ClosestHitKSP = IRB.CreateLoad(
            ShaderRecordPtr, VALUE_NAME("ClosestHitKSP"));
        NullClosestHit = IRB.CreateICmpEQ(
            ClosestHitKSP,
            IRB.getInt64(KSP::NullValue));
    }

    return NullClosestHit;
}

Optional<std::string> LowerIntersectionAnyHit::getUniqueClosestHit(
    const Function& F) const
{
    Optional<std::string> CHS_FName;
    if (auto* Refs = m_CGCtx->hitgroupRefs(F.getName().str()))
    {
        auto LeaderCHS = (*Refs)[0]->ClosestHit;
        if (llvm::all_of(*Refs, [&](HitGroupInfo* H) { return LeaderCHS == H->ClosestHit; }))
            CHS_FName = LeaderCHS;
    }

    return CHS_FName;
}

// A Shader Identifier is composed of zero or more kernel shader
// pointers (KSPs):
// <---8 bytes---><---8 bytes-->
// +-------------+-------------+-------------+-------------+
// | closest-hit | IS/AS/IS+AS |   Unused    |   Unused    |
// +-------------+-------------+-------------+-------------+
//
// Where:
//
// IS = intersection shader
// AS = any-hit shader
//
// the IS+AS case occurs when a procedural geometry hitgroup has both an
// intersection and any-hit shader.  We don't have HW to invoke them
// individually so we fuse them into one shader.  It is also legal to have a
// null KSP in each slot (null == 0).
//
// We intend to use the two unused slots for continuation KSPs.
// See 'struct KSP' in RTStackFormat.h for description of the bitfields of a
// KSP.
template <typename FnType>
void LowerIntersectionAnyHit::invokeClosestHitShader(
    RTBuilder& IRB,
    // The value from HitGroupRecPtr0/1.  Points to the base of a shader record.
    Value* ShaderRecordPointer,
    Value* RayFlags,
    // Pass an empty block.  This will call btd.
    BasicBlock* BTDBB,
    // This will be invoked to setup RTStack that the closest-hit needs to read
    // once invoked.
    FnType InitForCHS,
    // Pass an empty block.  This will go to the next continuation.
    // TODO: Won't need this once we have new BTD encoding.
    BasicBlock* SkipBB)
{
    // if ((flags & SKIP_CLOSEST_HIT_SHADER) | ClosestHitKSP == null)
    //   <run the continuation>
    // else
    //   BTD(ClosestHit)

    IGC_ASSERT_MESSAGE((IRB.GetInsertBlock()->getTerminator() == nullptr),
        "Expected that terminator was already removed!");
    {
        auto* SkipClosestHit = checkFlagBitSet(
            IRB,
            RayFlags,
            RTStackFormat::RayFlags::SKIP_CLOSEST_HIT_SHADER,
            VALUE_NAME("skip_closest_hit_flag_bit_set"));
        Value* NullClosestHit = isClosestHitNull(IRB, ShaderRecordPointer);
        SkipClosestHit = IRB.CreateOr(SkipClosestHit, NullClosestHit,
            VALUE_NAME("skip_closest_hit"));
        IRB.CreateCondBr(SkipClosestHit, SkipBB, BTDBB);
    }

    RTBuilder::InsertPointGuard Guard(IRB);

    //////// btd closest hit bb ////////
    IRB.SetInsertPoint(BTDBB);
    InitForCHS(IRB);
    CallInst* btdCall = IRB.CreateBTDCall(ShaderRecordPointer);
    auto CHS_FName = getUniqueClosestHit(*btdCall->getFunction());
    if (CHS_FName)
    {
        MDNode* node = MDNode::get(
            btdCall->getContext(),
            MDString::get(btdCall->getContext(), *CHS_FName));
        btdCall->setMetadata(RTBuilder::BTDTarget, node);
    }
    IRB.CreateRetVoid();

    //////// skip closest hit bb ////////
    IRB.SetInsertPoint(SkipBB);

    IRB.createMergeCall();
    IRB.CreateRetVoid();
}

Value* LowerIntersectionAnyHit::checkFlagBitSet(
    RTBuilder &IRB,
    Value* RayFlags,
    RTStackFormat::RayFlags Flag,
    const Twine &Name)
{
    IGC_ASSERT(RayFlagBits);

    uint32_t BitPos = llvm::countTrailingZeros((uint32_t)Flag);
    if (auto Val = (*RayFlagBits)[BitPos])
        return (*Val ? IRB.getTrue() : IRB.getFalse());

    auto *Set = IRB.CreateAnd(
        RayFlags,
        IRB.getInt16((uint16_t)Flag));
    Set = IRB.CreateICmpNE(Set, IRB.getInt16(0), Name);

    return Set;
}

void LowerIntersectionAnyHit::injectExitCode(
    RTBuilder& IRB,
    RTBuilder::StackPointerVal* StackPointer,
    Value* RayFlags)
{
    auto* BB = IRB.GetInsertBlock();
    auto* F = BB->getParent();
    auto& C = BB->getContext();

    BasicBlock* BTDClosestHitBB = BasicBlock::Create(
        C, VALUE_NAME("BTDClosestHitBB"), F, BB->getNextNode());
    BasicBlock* SkipClosestHitBB = BasicBlock::Create(
        C, VALUE_NAME("SkipClosestHit"), F, BTDClosestHitBB->getNextNode());

    // RayTFar = stack->potentialHit.t;
    Value* RayTFar = IRB.getPotentialHitT(StackPointer);

    Value* PotentialPrimVal = IRB.getHitTopOfPrimLeafPtr(
        StackPointer, CallableShaderTypeMD::AnyHit);
    Value* PotentialInstVal = IRB.getPotentialHitTopInstLeafPtr(StackPointer);

    auto InitForCHS = [=](RTBuilder& IRB) {
        initializeCommittedHit(
            IRB,
            StackPointer,
            RayTFar,
            PotentialPrimVal,
            PotentialInstVal);
    };
    // need to do BTD to the closest hit shader
    Value* ClosestHitAddress = IRB.getHitGroupRecPtrFromPrimAndInstVals(
        PotentialPrimVal, PotentialInstVal);

    invokeClosestHitShader(
        IRB,
        ClosestHitAddress,
        RayFlags,
        BTDClosestHitBB,
        InitForCHS,
        SkipClosestHitBB);
}

CallInst* LowerIntersectionAnyHit::codeGenReportHit(
    RTArgs &RTArgs,
    ReportHitHLIntrinsic *RHI,
    Function *AnyHitShader,
    Value *TraceRayCtrlLoc,
    RTBuilder::SWStackPtrVal* FrameAddr)
{
    auto &C = RHI->getContext();
    auto *F = RHI->getFunction();
    auto *RHBlock = RHI->getParent();
    auto *EndBlock = RHBlock->splitBasicBlock(
        RHI->getNextNode(), VALUE_NAME("ReportHitEndBlock"));

    CallInst* AnyHitCall = nullptr;


    BasicBlock* ClosestHitTrueBB = BasicBlock::Create(
        C, VALUE_NAME("ClosestHitTrue"), F, EndBlock);

    BasicBlock* OpaqueTrueBB = BasicBlock::Create(
        C, VALUE_NAME("OpaqueTrue"), F, EndBlock);

    BasicBlock* OpaqueFalseBB = BasicBlock::Create(
        C, VALUE_NAME("OpaqueFalse"), F, EndBlock);

    BasicBlock* TermTrueBB = BasicBlock::Create(
        C, VALUE_NAME("TermTrue"), F, EndBlock);

    BasicBlock* TermFalseBB = BasicBlock::Create(
        C, VALUE_NAME("TermFalse"), F, EndBlock);

    BasicBlock* SkipClosestHitBB = BasicBlock::Create(
        C, VALUE_NAME("SkipClosestHit"), F, EndBlock);

    BasicBlock* BTDClosestHitBB = BasicBlock::Create(
        C, VALUE_NAME("BTDClosestHitBB"), F, EndBlock);

    RTBuilder IRB(RHI, *m_CGCtx);
    auto *StackPointer = IRB.getAsyncStackPointer();

    // As per the DXR spec:

    // Ray-triangle intersection can only occur if the intersection t-value
    // satisfies TMin < t < TMax.

    // Ray-procedural-primitive intersection can only occur if the intersection
    // t-value satisfies TMin <= t <= TMax.

    // closest hit so far?
    // if (THit >= ray_tnear && THit <= ray_tfar)
    {
        Value* RayTMin = IRB.getRayTMin(StackPointer);
        Value* RayTFar = IRB.getPotentialHitT(StackPointer);
        Value* GreaterThanTMin = IRB.CreateFCmpOGE(RHI->getTHit(), RayTMin);
        Value* LessThanTFar = IRB.CreateFCmpOLE(RHI->getTHit(), RayTFar);
        Value* ClosestSoFar = IRB.CreateAnd(GreaterThanTMin, LessThanTFar,
            VALUE_NAME("ClosestSoFar"));
        RHBlock->getTerminator()->eraseFromParent();
        IRB.SetInsertPoint(RHBlock);
        IRB.CreateCondBr(ClosestSoFar, ClosestHitTrueBB, EndBlock);
    }

    // check opaque
    //////// closest hit true bb ////////
    IRB.SetInsertPoint(ClosestHitTrueBB);

    // Copy the custom hit attributes to the stack to be read later by
    // a any-hit and closest-hit shaders.
    //
    // Also place the custom HitKind for the any-hit and closest-hit shaders to read.
    {
        Value* Attrs = RHI->getAttributes();
        if (!isa<ConstantPointerNull>(Attrs->stripPointerCasts()))
        {
            auto* AttrTy = Attrs->getType();
            Value* CustomHitAttrPtr = RTArgs.getCustomHitAttribPtr(
                IRB, FrameAddr, AttrTy->getPointerElementType());

            auto& DL = F->getParent()->getDataLayout();
            IRB.CreateMemCpy(
                CustomHitAttrPtr,
                4,
                Attrs,
                std::min(4U, (unsigned)DL.getABITypeAlignment(AttrTy->getPointerElementType())),
                IRB.getInt64(DL.getTypeAllocSize(AttrTy->getPointerElementType())));
        }

        IRB.setProceduralHitKind(RTArgs, FrameAddr, RHI->getHitKind());
    }

    if (!AnyHitShader)
    {
        // If there is no any-hit shader, the geometry is assumed to be opaque.
        IRB.CreateBr(OpaqueTrueBB);
    }
    else
    {
        // frontFace: whether we hit the front-facing side of a triangle
        // (also used to pass opaque flag when calling intersection shaders)
        Value* IsOpaque = IRB.getIsFrontFace(StackPointer, CallableShaderTypeMD::AnyHit);
        IsOpaque->setName(VALUE_NAME("is_opaque"));
        IRB.CreateCondBr(IsOpaque, OpaqueTrueBB, OpaqueFalseBB);
    }

    //////// opaque false bb /////////
    IRB.SetInsertPoint(OpaqueFalseBB);
    Value* AnyHitAcceptHitAndEndSearch = nullptr;
    if (!AnyHitShader)
    {
        // Without an any-hit shader this is just a dead block so just
        // mark it with 'unreachable' and it will be cleaned up later.
        AnyHitAcceptHitAndEndSearch = IRB.getFalse();
        IRB.CreateUnreachable();
    }
    else
    {
        SmallVector<Value*, 4> Args;
        // We have already lowered the arguments in this function.  Just pass
        // in undef values to satisfy the function signature (the arguments
        // won't be read).  We do, though, need to pass in the implicit
        // args for HitKind and THit (see createAnyHitFn()).
        size_t NumArgs = AnyHitShader->arg_size();
        IGC_ASSERT_MESSAGE((NumArgs >= 2), "missing args?");
        size_t NumExplicitArgs = NumArgs - 2;
        auto ArgI = AnyHitShader->arg_begin();
        for (size_t CurArg = 0; CurArg < NumExplicitArgs; CurArg++, ArgI++)
            Args.push_back(UndefValue::get(ArgI->getType()));
        Args.push_back(RHI->getHitKind());
        Args.push_back(RHI->getTHit());
        auto* CI = IRB.CreateCall(
            AnyHitShader, Args, VALUE_NAME("any_hit_result"));
        AnyHitAcceptHitAndEndSearch = IRB.CreateICmpEQ(
            CI, IRB.getInt32((uint32_t)ANYHIT_RETURN_CODES::ACCEPT_HIT),
            VALUE_NAME("is_accept_ret"));
        Value* Ignore = IRB.CreateICmpEQ(
            CI, IRB.getInt32((uint32_t)ANYHIT_RETURN_CODES::IGNORE_HIT),
            VALUE_NAME("is_ignore_ret"));
        IRB.CreateCondBr(Ignore, EndBlock, OpaqueTrueBB);

        AnyHitCall = CI;
    }

    //////// opaque true bb ////////
    IRB.SetInsertPoint(OpaqueTrueBB);
    auto* AcceptHitAndEndSearch = IRB.CreatePHI(
        IRB.getInt1Ty(), 2, VALUE_NAME("accept_hit_and_end_search"));
    AcceptHitAndEndSearch->addIncoming(IRB.getFalse(), ClosestHitTrueBB);
    AcceptHitAndEndSearch->addIncoming(
        AnyHitAcceptHitAndEndSearch, OpaqueFalseBB);

    // Once we reach this block, we know that ReportHit() will return true.
    // We have a hit, need to issue a TRACE_RAY_COMMIT
    {
        IRB.CreateStore(
            IRB.getInt32(TraceRayMessage::TRACE_RAY_COMMIT),
            TraceRayCtrlLoc);
    }

    // term = RayFlag & ACCEPT_FIRST_HIT_AND_END_SEARCH | AcceptHitAndEndSearch()
    // term?
    auto *RayFlags = IRB.getRayFlags(StackPointer);
    auto* DoTerm = checkFlagBitSet(
        IRB, RayFlags,
        RTStackFormat::RayFlags::ACCEPT_FIRST_HIT_AND_END_SEARCH,
        VALUE_NAME("rayflag_term"));
    DoTerm = IRB.CreateOr(DoTerm, AcceptHitAndEndSearch,
        VALUE_NAME("term?"));
    IRB.CreateCondBr(DoTerm, TermTrueBB, TermFalseBB);

    //////// term false bb /////////
    IRB.SetInsertPoint(TermFalseBB);
    //stack->potentialHit.t = THit;
    IRB.setPotentialHitT(StackPointer, RHI->getTHit());
    //stack->potentialHit.valid = true;
    IRB.setHitValid(StackPointer, CallableShaderTypeMD::AnyHit);
    IRB.CreateBr(EndBlock);

    //////// term true bb ////////
    IRB.SetInsertPoint(TermTrueBB);
    Value* PotentialPrimVal = IRB.getHitTopOfPrimLeafPtr(
        StackPointer, CallableShaderTypeMD::AnyHit);
    Value* PotentialInstVal = IRB.getPotentialHitTopInstLeafPtr(StackPointer);
    auto InitForCHS = [=](RTBuilder& IRB) {
        initializeCommittedHit(
            IRB,
            StackPointer,
            RHI->getTHit(),
            PotentialPrimVal,
            PotentialInstVal);
    };
    // need to do BTD to the closest hit shader
    Value* ClosestHitAddress = IRB.getHitGroupRecPtrFromPrimAndInstVals(
        PotentialPrimVal, PotentialInstVal);

    invokeClosestHitShader(
        IRB,
        ClosestHitAddress,
        RayFlags,
        BTDClosestHitBB,
        InitForCHS,
        SkipClosestHitBB);

    // ReportHit() returns true if we commit the hit.
    if (!RHI->use_empty())
    {
        SSAUpdater Updater;
        Updater.Initialize(IRB.getInt1Ty(), "report_hit_val");
        Updater.AddAvailableValue(RHBlock, IRB.getFalse());
        Updater.AddAvailableValue(OpaqueTrueBB, IRB.getTrue());
        Value* ReportRetVal = Updater.GetValueAtEndOfBlock(EndBlock);
        RHI->replaceAllUsesWith(ReportRetVal);
    }

    RHI->eraseFromParent();
    return AnyHitCall;
}

void LowerIntersectionAnyHit::codeGenIgnoreHit(GenIntrinsicInst* IHI)
{
    auto* BB = IHI->getParent();
    BB->splitBasicBlock(IHI, VALUE_NAME("DeadBB"));
    BB->getTerminator()->eraseFromParent();
    RTBuilder IRB(BB, *m_CGCtx);
    IRB.createASyncTraceRay(
        IRB.getInt32(BOTTOM_LEVEL_BVH),
        TraceRayMessage::TRACE_RAY_CONTINUE,
        VALUE_NAME("ignore_hit_continue"));
    IRB.CreateRetVoid();
    IHI->eraseFromParent();
}

void LowerIntersectionAnyHit::codeGenAcceptHitAndEndSearch(GenIntrinsicInst* AHI)
{
    auto* BB = AHI->getParent();

    BB->splitBasicBlock(AHI, VALUE_NAME("DeadBB"));
    BB->getTerminator()->eraseFromParent();

    RTBuilder IRB(BB, *m_CGCtx);
    IRB.SetCurrentDebugLocation(AHI->getDebugLoc());
    auto *StackPointer = IRB.getAsyncStackPointer();
    auto *RayFlags = IRB.getRayFlags(StackPointer);

    injectExitCode(IRB, StackPointer, RayFlags);

    AHI->eraseFromParent();
}

void LowerIntersectionAnyHit::addTraceRayAtEnd(Function &F, Value *TraceRayCtrlLoc)
{
    // Visit all 'ret' instructions and insert continues
    RTBuilder IRB(F.getContext(), *m_CGCtx);
    for (auto &BB : F)
    {
        auto *RI = dyn_cast<ReturnInst>(BB.getTerminator());
        if (!RI)
            continue;

        IRB.SetInsertPoint(RI);
        Value *CtrlVal = IRB.CreateLoad(TraceRayCtrlLoc);
        IRB.createASyncTraceRay(
            IRB.getInt32(BOTTOM_LEVEL_BVH),
            CtrlVal,
            VALUE_NAME("trace_ray_continue_payload"));
    }
}

void LowerIntersectionAnyHit::addAnyHitExitCode(Function &F)
{
    // Visit all 'ret' instructions and insert code to determine what to
    // execute next.
    RTBuilder IRB(F.getContext(), *m_CGCtx);

    SmallVector<BasicBlock*, 4> BBs;
    for (auto& BB : F)
    {
        if (isa<ReturnInst>(BB.getTerminator()))
            BBs.push_back(&BB);
    }

    for (auto *BB : BBs)
    {
        auto *RI = cast<ReturnInst>(BB->getTerminator());
        IRB.SetCurrentDebugLocation(RI->getDebugLoc());

        // if (flags & RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH)
        //   // BTD(ClosestHit) or continuation
        // else
        //    TraceRayCommit()

        // The fallthrough behavior of any-hit shaders is to commit unless
        // IgnoreHit() or AcceptHitAndEndSearch() is called.
        RI->eraseFromParent();

        IRB.SetInsertPoint(BB);
        auto* StackPointer = IRB.getAsyncStackPointer();
        auto* RayFlags = IRB.getRayFlags(StackPointer);

        auto* DoBTD = checkFlagBitSet(
            IRB,
            RayFlags,
            RTStackFormat::RayFlags::ACCEPT_FIRST_HIT_AND_END_SEARCH,
            VALUE_NAME("end_search?"));

        auto* BTDBB = BasicBlock::Create(
            F.getContext(), VALUE_NAME("BTDBB"), &F, BB->getNextNode());
        auto* CommitBB = BasicBlock::Create(
            F.getContext(), VALUE_NAME("CommitBB"), &F, BTDBB->getNextNode());

        IRB.CreateCondBr(DoBTD, BTDBB, CommitBB);

        IRB.SetInsertPoint(BTDBB);
        injectExitCode(IRB, StackPointer, RayFlags);

        IRB.SetInsertPoint(CommitBB);
        IRB.createASyncTraceRay(
            IRB.getInt32(BOTTOM_LEVEL_BVH),
            TraceRayMessage::TRACE_RAY_COMMIT,
            VALUE_NAME("trace_ray_commit_payload"));
        IRB.CreateRetVoid();
    }
}

// This is the shader that will be placed into the callStackHandlerPtr slot
// in RayDispatchGlobalData.  This shader will be invoked by the RTUnit
// when it needs to continue tracing a ray but there isn't a shader to call.
// For example, a shader record could have a null closest-hit shader.  When a
// ray triangle intersection is found, the RTUnit would normally BTD to the
// closest-hit shader (or perhaps any-hit if available).  The
// RTUnit detects that the shader is null and instead invokes this shader.
void LowerIntersectionAnyHit::createCallStackHandler(Module &M)
{
    // Code later in this pass will populate it with code that invokes
    // the next continuation.
    // If the shader type is 'procedural' (i.e., the anyhit+intersection),
    // we emit a trace ray continue rather than popping the continuation
    // off the stack.
    auto& C = M.getContext();
    auto* FTy = FunctionType::get(Type::getVoidTy(C), false);
    auto *F = Function::Create(
        FTy,
        GlobalValue::ExternalLinkage,
        "__callStackHandler",
        &M);

    auto *Entry = BasicBlock::Create(C, VALUE_NAME("entry"), F);
    auto* TraceRayBB = BasicBlock::Create(C, VALUE_NAME("TraceRay"), F);
    auto *ContinuationBB = BasicBlock::Create(C, VALUE_NAME("Continuation"), F);
    RTBuilder IRB(Entry, *m_CGCtx);
    Value* BTDShaderType = IRB.CreateShaderType();

    Value* IsProcedural = IRB.CreateICmpEQ(
        BTDShaderType,
        IRB.getInt32((uint32_t)RayTracingShaderType::INTERSECTION),
        VALUE_NAME("is_procedural"));

    Value* IsAnyHit = IRB.CreateICmpEQ(
        BTDShaderType,
        IRB.getInt32((uint32_t)RayTracingShaderType::ANY_HIT),
        VALUE_NAME("is_anyhit"));

    Value* ShouldTrace = IRB.CreateOr(IsProcedural, IsAnyHit);

    auto* StackPointer = IRB.getAsyncStackPointer();
    Value* BvhLevel = IRB.getInt32(BOTTOM_LEVEL_BVH);
    IRB.CreateCondBr(ShouldTrace, TraceRayBB, ContinuationBB);

    IRB.SetInsertPoint(TraceRayBB);
    auto* TraceRayCtrl = IRB.CreateSelect(
        IsProcedural,
        IRB.getInt32(TraceRayMessage::TRACE_RAY_CONTINUE),
        IRB.getInt32(TraceRayMessage::TRACE_RAY_COMMIT),
        VALUE_NAME("traceRayCtrl"));
    IRB.createASyncTraceRay(
        BvhLevel,
        TraceRayCtrl,
        VALUE_NAME("trace_ray"));
    IRB.CreateRetVoid();

    IRB.SetInsertPoint(ContinuationBB);
    IRB.createMergeCall();
    IRB.CreateRetVoid();

    markAsCallableShader(F, CallStackHandler);
}

void LowerIntersectionAnyHit::LowerPayload(
    Function* F, RTArgs& Args, RTBuilder::SWStackPtrVal* FrameAddr)
{
    auto *PayloadPtr = RTBuilder::LowerPayload(F, Args, FrameAddr);
    if (!PayloadPtr || !m_CGCtx->tryPayloadSinking())
        return;

    RTBuilder RTB(PayloadPtr->getNextNode(), *m_CGCtx);

    auto* II = RTB.getPayloadPtrIntrinsic(
        UndefValue::get(PayloadPtr->getType()),
        FrameAddr);
    PayloadPtr->replaceAllUsesWith(II);
    II->setPayloadPtr(PayloadPtr);
}

// For each procedural any-hit shader, we will:
// 1. create a new function with an i32 return type.
// 2. splice the code from the original to the new.
// 3. Lower arguments and lower IgnoreHit() and AcceptHitAndEndSearch() to
//    return values.
Function* LowerIntersectionAnyHit::createAnyHitFn(
    Function &F, FunctionMetaData &MD)
{
    IRBuilder<> IRB(F.getContext());

    auto* FTy = F.getFunctionType();
    SmallVector<Type*, 2> Tys;
    for (auto *Ty : FTy->params())
        Tys.push_back(Ty);

    // Also add two implicit args for the HitKind and the THit values which
    // will both be provided by the ReportHit() from the Intersection shader.
    Tys.push_back(IRB.getInt32Ty()); // HitKind
    Tys.push_back(IRB.getFloatTy()); // THit

    auto* NewFuncTy = FunctionType::get(IRB.getInt32Ty(), Tys, false);

    auto* NewFunc = Function::Create(
        NewFuncTy, F.getLinkage(), F.getName(), F.getParent());

    ValueToValueMapTy VM;
    VM[&F] = NewFunc;
    Function::arg_iterator DestI = NewFunc->arg_begin();
    for (const Argument& I : F.args())
    {
        DestI->setName(I.getName());
        VM[&I] = &*DestI++;
    }

    Argument* HitKindArg = &*DestI++;
    HitKindArg->setName("hitkind");
    Argument* THitArg    = &*DestI++;
    THitArg->setName("thit");

    NewFunc->getBasicBlockList().splice(NewFunc->end(), F.getBasicBlockList());
    RemapFunction(*NewFunc, VM, RF_IgnoreMissingLocals | RF_ReuseAndMutateDistinctMDs);

    RTBuilder RTB(&*NewFunc->getEntryBlock().getFirstInsertionPt(), *m_CGCtx);
    auto* FrameAddr = RTB.getSWStackPointer();

    RTArgs Args(NewFunc, AnyHit,
        HIT_GROUP_TYPE::PROCEDURAL_PRIMITIVE,
        m_CGCtx, MD, MD.rtInfo.Types);

    LowerPayload(NewFunc, Args, FrameAddr);
    RTBuilder::loadCustomHitAttribsFromStack(*NewFunc, Args, FrameAddr);

    SmallVector<GenIntrinsicInst*, 4> IgnoreHits;
    SmallVector<GenIntrinsicInst*, 4> AcceptHits;
    SmallVector<GenIntrinsicInst*, 4> HitKinds;
    SmallVector<GenIntrinsicInst*, 4> RayTCurrents;
    // Need to update old 'ret void' instructions
    SmallVector<ReturnInst*, 4> Rets;
    for (auto& I : instructions(*NewFunc))
    {
        if (auto * GII = dyn_cast<GenIntrinsicInst>(&I))
        {
            switch (GII->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_IgnoreHitHL:
                IgnoreHits.push_back(GII);
                break;
            case GenISAIntrinsic::GenISA_AcceptHitAndEndSearchHL:
                AcceptHits.push_back(GII);
                break;
            case GenISAIntrinsic::GenISA_HitKind:
                HitKinds.push_back(GII);
                break;
            case GenISAIntrinsic::GenISA_RayTCurrent:
                RayTCurrents.push_back(GII);
                break;
            default:
                break;
            }
        }
        else if (auto * RI = dyn_cast<ReturnInst>(&I))
        {
            Rets.push_back(RI);
        }
    }

    // Replace returns and intrinsics with return values to indicate to the
    // caller (the intersection shader) what happened inside the function.
    auto procInst = [](Instruction* I, uint32_t RetVal)
    {
        auto* BB = I->getParent();
        BB->splitBasicBlock(I);
        BB->getTerminator()->eraseFromParent();
        IRBuilder<> IRB(BB);
        IRB.CreateRet(IRB.getInt32(RetVal));
        I->eraseFromParent();
    };

    for (auto* HK : HitKinds)
    {
        HK->replaceAllUsesWith(HitKindArg);
        HK->eraseFromParent();
    }

    for (auto* RTC : RayTCurrents)
    {
        RTC->replaceAllUsesWith(THitArg);
        RTC->eraseFromParent();
    }

    for (auto* Ret : Rets)
        procInst(Ret, (uint32_t)ANYHIT_RETURN_CODES::FALLTHROUGH_EXIT);

    for (auto* IHI : IgnoreHits)
        procInst(IHI, (uint32_t)ANYHIT_RETURN_CODES::IGNORE_HIT);

    for (auto* AHI : AcceptHits)
        procInst(AHI, (uint32_t)ANYHIT_RETURN_CODES::ACCEPT_HIT);

    return NewFunc;
}

LowerIntersectionAnyHit::AnyHitMapTy
LowerIntersectionAnyHit::getInlineableAnyHitMapping(Module& M)
{
    AnyHitMapTy Mapping;

    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto &FuncMD = modMD->FuncMD;

    // createAnyHitFn() below will inject new temporary functions into the
    // module.  Gather up the current funcs here so we can safely iterate
    // over them instead of the module functions.
    SmallVector<Function*, 4> Funcs;
    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;

        Funcs.push_back(&F);
    }

    for (auto* Func : Funcs)
    {
        auto& F = *Func;
        if (F.isDeclaration())
            continue;

        auto MD = FuncMD.find(&F);
        IGC_ASSERT_MESSAGE((MD != FuncMD.end()), "Missing metadata?");
        auto CallableShaderType = MD->second.rtInfo.callableShaderType;

        if (CallableShaderType == AnyHit)
        {
            if (m_CGCtx->getHitGroupType(F.getName().str()) !=
                HIT_GROUP_TYPE::PROCEDURAL_PRIMITIVE)
            {
                continue;
            }

            Function* AnyHitFn = createAnyHitFn(F, MD->second);
            Mapping.insert(std::make_pair(F.getName(), AnyHitFn));
        }
    }

    return Mapping;
}

// returns the corresponding any-hit shader for this intersection shader
// or null if no such shader exists.
Function* LowerIntersectionAnyHit::getAnyHitShader(
    Function& IntersectionFn,
    const AnyHitMapTy &Map)
{
    auto AnyHitName = m_CGCtx->getIntersectionAnyHit(IntersectionFn.getName().str());
    if (!AnyHitName)
        return nullptr;

    Function* AnyHitFn = nullptr;
    auto AHI = Map.find(*AnyHitName);
    if (AHI != Map.end())
        AnyHitFn = AHI->second;

    return AnyHitFn;
}

void LowerIntersectionAnyHit::lowerTriangleAnyHitIntrinsics(Function& F)
{
    addAnyHitExitCode(F);

    // Now that we've added TRACE_RAY_COMMIT in the fallthrough paths,
    // handle the intrinics.
    SmallVector<GenIntrinsicInst*, 4> IgnoreHits;
    SmallVector<GenIntrinsicInst*, 4> AcceptHits;
    for (auto& I : instructions(F))
    {
        if (auto * GII = dyn_cast<GenIntrinsicInst>(&I))
        {
            switch (GII->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_IgnoreHitHL:
                IgnoreHits.push_back(GII);
                break;
            case GenISAIntrinsic::GenISA_AcceptHitAndEndSearchHL:
                AcceptHits.push_back(GII);
                break;
            default:
                break;
            }
        }
    }

    for (auto* IHI : IgnoreHits)
        codeGenIgnoreHit(IHI);

    for (auto* AHI : AcceptHits)
        codeGenAcceptHitAndEndSearch(AHI);
}

void LowerIntersectionAnyHit::handleIntersectionAnyHitShaders(Module &M)
{
    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto &FuncMD = modMD->FuncMD;

    auto& DL = M.getDataLayout();

    uint32_t SWStackAddrSpace = RTBuilder::getSWStackAddrSpace(*modMD);

    // First, populate max custom hit attr sizes prior to doing anything else
    for (auto &F : M)
    {
        if (F.isDeclaration())
            continue;

        auto MD = FuncMD.find(&F);
        IGC_ASSERT_MESSAGE((MD != FuncMD.end()), "Missing metadata?");
        auto CallableShaderType = MD->second.rtInfo.callableShaderType;
        auto& rtInfo = MD->second.rtInfo;

        if (CallableShaderType == Intersection)
        {
            uint64_t MaxCustomHitAttrSize = 0;

            for (auto &I : instructions(F))
            {
                if (auto * RHI = dyn_cast<ReportHitHLIntrinsic>(&I))
                {
                    Value* Attrs = RHI->getAttributes();
                    if (isa<ConstantPointerNull>(Attrs->stripPointerCasts()))
                        continue;

                    MaxCustomHitAttrSize = std::max(
                        MaxCustomHitAttrSize,
                        (uint64_t)DL.getTypeAllocSize(
                            Attrs->getType()->getPointerElementType()));
                }
            }

            rtInfo.CustomHitAttrSizeInBytes = (uint32_t)MaxCustomHitAttrSize;
        }
        else if (CallableShaderType == ClosestHit || CallableShaderType == AnyHit)
        {
            if (m_CGCtx->getHitGroupType(F.getName().str()) ==
                HIT_GROUP_TYPE::PROCEDURAL_PRIMITIVE)
            {
                ArgQuery Q{ CallableShaderType, MD->second };
                if (auto *Arg = Q.getHitAttribArg(&F))
                {
                    if (!Arg->use_empty())
                    {
                        uint64_t Size = DL.getTypeAllocSize(
                            Arg->getType()->getPointerElementType());
                        rtInfo.CustomHitAttrSizeInBytes = (uint32_t)Size;
                    }
                }
            }
        }
    }

    SmallVector<Function*, 4> Funcs;
    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;

        Funcs.push_back(&F);
    }

    auto AnyHitMap = getInlineableAnyHitMapping(M);

    for (auto *Func : Funcs)
    {
        auto& F = *Func;
        if (F.isDeclaration())
            continue;

        auto MD = FuncMD.find(&F);
        IGC_ASSERT_MESSAGE((MD != FuncMD.end()), "Missing metadata?");
        auto CallableShaderType = MD->second.rtInfo.callableShaderType;

        if (CallableShaderType == Intersection)
        {
            RTArgs Args(
                &F, CallableShaderType,
                HIT_GROUP_TYPE::PROCEDURAL_PRIMITIVE,
                m_CGCtx, MD->second, MD->second.rtInfo.Types);

            Function* AnyHitFn = getAnyHitShader(F, AnyHitMap);
            // keep track of the trace ray type
            RTBuilder IRB(&*F.getEntryBlock().getFirstInsertionPt(), *m_CGCtx);
            auto* FrameAddr = IRB.getSWStackPointer();
            AllocaInst *TraceRayCtrl = IRB.CreateAlloca(
                IRB.getInt32Ty(),
                nullptr, "",
                SWStackAddrSpace);

            // behavior is to exit the intersection shader with
            // TRACE_RAY_CONTINUE unless a commit happens (via ReportHit()).
            IRB.CreateStore(
                IRB.getInt32(TraceRayMessage::TRACE_RAY_CONTINUE),
                TraceRayCtrl);

            addTraceRayAtEnd(F, TraceRayCtrl);
            SmallVector<ReportHitHLIntrinsic*, 4> ReportHits;
            for (auto &I : instructions(F))
            {
                if (auto *ReportHit = dyn_cast<ReportHitHLIntrinsic>(&I))
                    ReportHits.push_back(ReportHit);
            }

            for (auto* RHI : ReportHits)
            {
                auto *AnyHitCall = codeGenReportHit(
                    Args, RHI, AnyHitFn, TraceRayCtrl, FrameAddr);
                if (AnyHitCall)
                {
                    InlineFunctionInfo IFI;
                    bool CanInline = IGCLLVM::InlineFunction(AnyHitCall, IFI);
                    CanInline = CanInline;
                    IGC_ASSERT_MESSAGE(CanInline, "failed to inline?");
                }
            }
        }
        else if (CallableShaderType == AnyHit)
        {
            if (m_CGCtx->getHitGroupType(F.getName().str()) ==
                HIT_GROUP_TYPE::PROCEDURAL_PRIMITIVE)
            {
                // There's nothing to process here because a copy of this
                // will be inlined in the intersection shader.
                // The stack size will be 0 for procedural any-hit shaders
                // because the whole stack reservation will be contained within
                // the intersection shader.
                continue;
            }

            lowerTriangleAnyHitIntrinsics(F);
        }
    }

    // Clean up the temporary any-hit functions that should, by now, be all
    // inlined into their respective intersection shaders.
    // Also, remove the original shaders so they don't show up in the output.
    for (auto [K, F] : AnyHitMap)
    {
        IGC_ASSERT_MESSAGE(F->use_empty(), "uses left uninlined?");
        F->eraseFromParent();

        Function* OrigShader = M.getFunction(K);

        if (!OrigShader)
            continue;

        IGCMD::IGCMetaDataHelper::removeFunction(
            *m_CGCtx->getMetaDataUtils(),
            *m_CGCtx->getModuleMetaData(),
            OrigShader);

        IGC_ASSERT_MESSAGE(OrigShader->use_empty(), "any-hit shader used?");
        OrigShader->eraseFromParent();
    }
}

namespace IGC
{

Pass* createLowerIntersectionAnyHitPass(void)
{
    return new LowerIntersectionAnyHit();
}

} // namespace IGC
