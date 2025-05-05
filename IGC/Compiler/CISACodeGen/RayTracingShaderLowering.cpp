/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// The current purpose of this pass is to inject fences at TraceRay() and BTD
/// calls to ensure that subsequently spawned threads have access to fresh data.
///
/// For now, we inject fences at all calls to ensure correctness.  We will later
/// add further analysis that will examine the locations of writes to determine
/// if we can elided the use of fences in some situations.
///
//===----------------------------------------------------------------------===//
#include "RayTracingShaderLowering.hpp"
#include "AdaptorCommon/RayTracing/RTBuilder.h"
#include "IGCPassSupport.h"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class RayTracingShaderLowering : public ModulePass
{
public:
    RayTracingShaderLowering() : ModulePass(ID) {}
    bool runOnModule(Module& M) override;
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<DominatorTreeWrapperPass>();
        AU.addRequired<PostDominatorTreeWrapperPass>();
    }

    StringRef getPassName() const override
    {
        return "RayTracingShaderLowering";
    }

    static char ID;
private:
    void injectFence(RTBuilder& RTB, GenIntrinsicInst* GII, bool rtFenceWAofBkMode, bool extraTGM) const;
    void simplifyCast(CastInst &CI);
    CodeGenContext* CGCtx = nullptr;
};

char RayTracingShaderLowering::ID = 0;

// From InstCombineCasts.cpp
static Instruction::CastOps isEliminableCastPair(
    const CastInst* CI1,
    const CastInst* CI2)
{
    auto& DL = CI1->getModule()->getDataLayout();

    Type* SrcTy = CI1->getSrcTy();
    Type* MidTy = CI1->getDestTy();
    Type* DstTy = CI2->getDestTy();

    Instruction::CastOps firstOp = CI1->getOpcode();
    Instruction::CastOps secondOp = CI2->getOpcode();
    Type* SrcIntPtrTy =
        SrcTy->isPtrOrPtrVectorTy() ? DL.getIntPtrType(SrcTy) : nullptr;
    Type* MidIntPtrTy =
        MidTy->isPtrOrPtrVectorTy() ? DL.getIntPtrType(MidTy) : nullptr;
    Type* DstIntPtrTy =
        DstTy->isPtrOrPtrVectorTy() ? DL.getIntPtrType(DstTy) : nullptr;
    unsigned Res = CastInst::isEliminableCastPair(firstOp, secondOp, SrcTy, MidTy,
        DstTy, SrcIntPtrTy, MidIntPtrTy,
        DstIntPtrTy);

    // We don't want to form an inttoptr or ptrtoint that converts to an integer
    // type that differs from the pointer size.
    if ((Res == Instruction::IntToPtr && SrcTy != DstIntPtrTy) ||
        (Res == Instruction::PtrToInt && DstTy != SrcIntPtrTy))
        Res = 0;

    return Instruction::CastOps(Res);
}

static Instruction* commonCastTransforms(CastInst& CI)
{
    Value* Src = CI.getOperand(0);

    // Try to eliminate a cast of a cast.
    if (auto * CSrc = dyn_cast<CastInst>(Src))
    {   // A->B->C cast
        if (Instruction::CastOps NewOpc = isEliminableCastPair(CSrc, &CI))
        {
            // The first cast (CSrc) is eliminable so we need to fix up or replace
            // the second cast (CI). CSrc will then have a good chance of being dead.
            auto* Ty = CI.getType();
            auto* Res = CastInst::Create(NewOpc, CSrc->getOperand(0), Ty);
            return Res;
        }
    }

    return nullptr;
}

// The memory intrinsics are lowered late such that there are no instcombine
// runs past this point.  Quickly look to see if we can do some simplification.
void RayTracingShaderLowering::simplifyCast(CastInst& CI)
{
    for (auto* U : CI.users())
    {
        if (auto* UserCI = dyn_cast<CastInst>(U))
        {
            if (CI.getOperand(0)->getType() == UserCI->getType())
            {
                // %p = inttoptr i64 %x to T*
                // %q = ptrtoint %p to i64
                // ===>
                // %x
                UserCI->replaceAllUsesWith(CI.getOperand(0));
            }
            else if (auto* NewCast = commonCastTransforms(*UserCI))
            {
                const DebugLoc& DL = UserCI->getDebugLoc();
                if (Instruction* NewCastInst = dyn_cast<Instruction>(NewCast))
                    NewCastInst->setDebugLoc(DL);
                NewCast->insertAfter(UserCI);
                UserCI->replaceAllUsesWith(NewCast);
            }
        }
    }
}

void RayTracingShaderLowering::injectFence(
    RTBuilder& RTB, GenIntrinsicInst* GII, bool rtFenceWAofBkMode, bool extraTGM) const
{
    LSC_FENCE_OP FenceOp = LSC_FENCE_OP_NONE;
    LSC_SCOPE    Scope   = LSC_SCOPE_LOCAL;
    RTB.SetInsertPoint(GII);
    if (rtFenceWAofBkMode)
    {
        RTB.CreateLSCFence(LSC_TGM, Scope, FenceOp);
    }
    else
    {
        RTB.CreateLSCFence(LSC_UGM, Scope, FenceOp);
        if (extraTGM)
            RTB.CreateLSCFence(LSC_TGM, Scope, FenceOp);
    }
}

bool RayTracingShaderLowering::runOnModule(Module& M)
{
    CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    RTBuilder RTB(M.getContext(), *CGCtx);
    const bool ForcePreemptionDisable =
        CGCtx->type == ShaderType::RAYTRACING_SHADER &&
        CGCtx->platform.canSupportWMTP();

    bool Changed = false;
    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;
        Value* EntryPreemptionVal = nullptr;
        auto checkPreemption = [&](Function &F) {
            if (EntryPreemptionVal)
                return EntryPreemptionVal;
            RTBuilder::InsertPointGuard Guard(RTB);
            RTB.SetInsertPoint(&F.getEntryBlock().front());
            Function* cr0 = llvm::GenISAIntrinsic::getDeclaration(
                F.getParent(),
                GenISAIntrinsic::GenISA_movcr);
            Value* Val = RTB.CreateCall(cr0, RTB.getInt32(0)); // cr0.0
            Val = RTB.CreateAnd(
                Val, EmitPass::getEncoderPreemptionMode(PREEMPTION_ENABLED));
            EntryPreemptionVal = RTB.CreateICmpNE(Val, RTB.getInt32(0));
            return EntryPreemptionVal;
        };
        const bool isFunc = !isEntryFunc(CGCtx->getMetaDataUtils(), &F);
        if (ForcePreemptionDisable)
        {
            RTB.SetInsertPoint(&F.getEntryBlock().front());
            RTB.CreatePreemptionDisableIntrinsic();
        }

        SmallVector<Instruction*, 8> DeadInsts;

        BasicBlock* disablePreemptionBB = nullptr;
        BasicBlock* enablePreemptionBB = nullptr;

        DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
        PostDominatorTree& PDT = getAnalysis<PostDominatorTreeWrapperPass>(F).getPostDomTree();

        for (auto II = inst_begin(&F), IE = inst_end(&F); II != IE; /* empty */)
        {
            Instruction& I = *II++;
            if (auto* II = dyn_cast<IntrinsicInst>(&I))
            {
                switch (II->getIntrinsicID())
                {
                case Intrinsic::lifetime_start:
                case Intrinsic::lifetime_end:
                {
                    auto* Ptr = II->getOperand(1);
                    uint32_t Addrspace =
                        Ptr->getType()->getPointerAddressSpace();
                    if (Addrspace != ADDRESS_SPACE_PRIVATE)
                        II->eraseFromParent();
                    break;
                }
                default:
                    break;
                }
            }
            auto* GII = dyn_cast<GenIntrinsicInst>(&I);
            if (!GII)
                continue;

            switch (GII->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_TraceRayAsync:
                injectFence(RTB, GII, CGCtx->platform.RTFenceWAforBkModeEnabled(), IGC_IS_FLAG_ENABLED(EnableRayTracingTGMFence));
                Changed = true;
                break;
            case GenISAIntrinsic::GenISA_BindlessThreadDispatch:
                injectFence(RTB, GII, CGCtx->platform.RTFenceWAforBkModeEnabled(), false);
                Changed = true;
                break;
            case GenISAIntrinsic::GenISA_SWHotZonePtr:
            case GenISAIntrinsic::GenISA_AsyncStackPtr:
            case GenISAIntrinsic::GenISA_SyncStackPtr:
            case GenISAIntrinsic::GenISA_SWStackPtr:
            {
                DeadInsts.push_back(GII);
                RTB.SetInsertPoint(GII);
                Value* Op = GII->getArgOperand(0);
                Value* NewVal = RTB.CreateBitOrPointerCast(Op, GII->getType());
                GII->replaceAllUsesWith(NewVal);
                if (auto *CI = dyn_cast<CastInst>(NewVal))
                    simplifyCast(*CI);
                Changed = true;
                break;
            }
            case GenISAIntrinsic::GenISA_RayQueryCheck:
                if (!ForcePreemptionDisable)
                {
                    disablePreemptionBB =
                        disablePreemptionBB ?
                        DT.findNearestCommonDominator(disablePreemptionBB, GII->getParent()) :
                        GII->getParent();
                }
                break;
            case GenISAIntrinsic::GenISA_RayQueryRelease:
                if (!ForcePreemptionDisable)
                {
                    enablePreemptionBB =
                        enablePreemptionBB ?
                        PDT.findNearestCommonDominator(enablePreemptionBB, GII->getParent()) :
                        GII->getParent();
                }
                break;
            default:
                break;
            }
        }

        if (disablePreemptionBB)
        {
            IGC_ASSERT(enablePreemptionBB);

            RTB.SetInsertPoint(disablePreemptionBB->getFirstNonPHI());
            RTB.CreatePreemptionDisableIntrinsic();
        }

        if (enablePreemptionBB)
        {
            IGC_ASSERT(disablePreemptionBB);

            Value* Flag = isFunc ? checkPreemption(F) : nullptr;

            RTB.SetInsertPoint(enablePreemptionBB->getTerminator());
            RTB.CreatePreemptionEnableIntrinsic(Flag);
        }

        for (auto* I : DeadInsts)
            I->eraseFromParent();
    }

    return Changed;
}

namespace IGC {

#define PASS_FLAG "igc-raytracing-shader-lowering"
#define PASS_DESCRIPTION "Do final fixup in raytracing shaders"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(RayTracingShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

ModulePass* CreateRayTracingShaderLowering()
{
    return new RayTracingShaderLowering();
}

} // namespace IGC
