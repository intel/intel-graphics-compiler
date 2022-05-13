/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Any transformations required for functional correctness prior to splitting
/// shaders into continuations.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "RTArgs.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include "Utils.h"

using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

class SplitPreparePass : public ModulePass
{
public:
    SplitPreparePass() : ModulePass(ID)
    {
        initializeSplitPreparePassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "SplitPreparePass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;
private:
    void hoistValues(Function &F, CallableShaderTypeMD Ty) const;
    bool processShader(Function& F, FunctionMetaData &FMD) const;
private:
    RayDispatchShaderContext *m_CGCtx = nullptr;
    const UnifiedBits* RayFlagBits = nullptr;
};

char SplitPreparePass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "split-prepare-pass"
#define PASS_DESCRIPTION "transformations required before continuation splitting to ensure correctness"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SplitPreparePass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SplitPreparePass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool SplitPreparePass::runOnModule(Module &M)
{
    m_CGCtx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    auto &FuncMD = modMD->FuncMD;

    auto FlagBits = examineRayFlags(*m_CGCtx);
    RayFlagBits = &FlagBits;

    bool Changed = false;

    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;

        auto MD = FuncMD.find(&F);
        IGC_ASSERT_MESSAGE((MD != FuncMD.end()), "Missing metadata?");

        Changed |= processShader(F, MD->second);
    }

    return Changed;
}

bool SplitPreparePass::processShader(
    Function& F, FunctionMetaData &FMD) const
{
    auto ShaderTy = FMD.rtInfo.callableShaderType;
    hoistValues(F, ShaderTy);

    switch (ShaderTy)
    {
    case ClosestHit:
    case AnyHit:
    {
        // Lower triangle intersection attributes here so we can spill the
        // barycentrics if necessary.  The procedural intersection attributes
        // are allocated in a stack frame so don't need to be spilled.  They
        // will be processed later.
        auto HitTy = m_CGCtx->getHitGroupType(F.getName().str());
        if (HitTy == HIT_GROUP_TYPE::TRIANGLES)
        {
            RTArgs Args(&F, ShaderTy,
                HitTy,
                m_CGCtx, FMD, FMD.rtInfo.Types);
            RTBuilder RTB(&*F.getEntryBlock().getFirstInsertionPt(), *m_CGCtx);
            auto* StackPointer = RTB.getAsyncStackPointer();
            RTBuilder::lowerIntersectionAttributeFromMemHit(
                F, Args, StackPointer);
        }
        break;
    }
    default:
        break;
    }

    return true;
}

// For example, say that WorldRayOrigin() is invoked after a TraceRay() call:
// [shader("closesthit")]
// void ClosestHitShader(...)
// {
//   TraceRay(...)
//   ... = WorldRayOrigin()
// }

// in a closest-hit shader.  Given that we set up the RTStack MemRay::org value
// when doing a TraceRay(), if we try to read it in the continuation after the
// TraceRay() we'll read the value from the ray passed to the TraceRay() rather
// than the ray that we're still processing.
//
// Here, we hoist system values to the entry block (at least for now) so that
// they will spill and we can use the correct value in the continuation.
//
// In addition, local pointers aren't valid in continuations since the private
// shader table just contains shaders.  We spill the pointer so it can be
// refilled in the continuation if needed.
void SplitPreparePass::hoistValues(
    Function &F,
    CallableShaderTypeMD Ty) const
{
    IGC_ASSERT(RayFlagBits);

    bool HoistSysVal = (Ty == ClosestHit || Ty == Miss);

    IRBuilder<> IRB(F.getContext());

    auto *IP = &*F.getEntryBlock().getFirstInsertionPt();

    for (auto II = inst_begin(F), IE = inst_end(F); II != IE; /* empty */)
    {
        auto* I = &*II++;
        if (auto *RIQ = dyn_cast<RayInfoIntrinsic>(I))
        {
            if (RIQ->getInfoKind() == IGC::RAY_FLAGS)
            {
                if (auto V = RayFlagBits->getConstant())
                {
                    auto* C = IRB.getInt32((uint32_t)V->getZExtValue());
                    RIQ->replaceAllUsesWith(C);
                    RIQ->eraseFromParent();
                    continue;
                }
            }

            if (HoistSysVal)
                RIQ->moveBefore(IP);
        }
        else if (auto *HK = dyn_cast<HitKindIntrinsic>(I))
        {
            if (HoistSysVal)
                HK->moveBefore(IP);
        }
        else if (auto *LocalPtr = dyn_cast<LocalBufferPointerIntrinsic>(I))
        {
            LocalPtr->moveBefore(IP);
        }
    }
}

namespace IGC
{

Pass* createSplitPreparePass(void)
{
    return new SplitPreparePass();
}

} // namespace IGC
