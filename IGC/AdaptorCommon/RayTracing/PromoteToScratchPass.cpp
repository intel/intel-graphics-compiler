/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Prior to this pass, all allocas will be alloated in the global address
/// space.  For example:
///
/// %1 = alloca i32, addrspace(1)
///
/// The intrinsic lowering pass (which runs after this pass) will examine all
/// addrspace(1) allocas and reserve space for them in the RTStack.  This is
/// useful for ray payload data that would be updated by another shader.  For
/// example:
///
/// struct RayPayload
/// {
///   float4 color;
/// };
///
/// [shader("raygeneration")]
/// void MyRaygenShader()
/// {
///   RayPayload payload = { float4(0, 0, 0, 0) };
///   TraceRay(..., payload);
/// }
///
/// [shader("miss")]
/// void MyMissShader(inout RayPayload payload)
/// {
///   payload.color = float4(0, 0, 0, 1);
/// }
///
/// The payload parameter passed to MyMissShader is by reference so appears
/// as a pointer in the IR.  We spill this pointer onto the RTStack in the
/// raygen shader so it can be read in the miss shader.  The 16 bytes
/// representing the RayPayload will also be spilled on the stack so that
/// memory is still live when the miss shader writes to it.  That must stay
/// on the RTStack.
///
/// In contrast, an allocation used privately within a shader doesn't require
/// its lifetime to extend to the execution of other shaders so the above alloca
/// would become:
///
/// %1 = alloca i32
///
/// Intrinsic lowering will skip this and allow PrivateMemoryResolution to do
/// its thing.
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "AllocaTracking.h"
#include "Compiler/IGCPassSupport.h"
#include "iStdLib/utility.h"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Support/DebugCounter.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace AllocaTracking;

#define DEBUG_TYPE "promote-to-scratch"

STATISTIC(NumAllocaPromoted, "Number of allocas promoted");

DEBUG_COUNTER(AllocaPromoteCounter, "promote-to-scratch-promote",
    "Controls number of promoted allocas");

class PromoteToScratchPass : public FunctionPass, public InstVisitor<PromoteToScratchPass>
{
public:
    PromoteToScratchPass() : FunctionPass(ID)
    {
        initializePromoteToScratchPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "PromoteToScratchPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
    }

    void visitAllocaInst(AllocaInst& AI);

    static char ID;
private:
    bool Changed;
};

char PromoteToScratchPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "promote-to-scratch"
#define PASS_DESCRIPTION "Convert global allocas into private allocas"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromoteToScratchPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PromoteToScratchPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool PromoteToScratchPass::runOnFunction(Function &F)
{
    Changed = false;
    visit(F);
    return Changed;
}

void PromoteToScratchPass::visitAllocaInst(AllocaInst& AI)
{
    if (!DebugCounter::shouldExecute(AllocaPromoteCounter))
        return;

    // For now, we use a simple filtering approach which can be expanded if
    // we see missing optimization opportunities.  If all users are supported,
    // we can just mutate the type in place for each of the values.
    IGC_ASSERT_MESSAGE(RTBuilder::isNonLocalAlloca(&AI), "Should still be global!");

    SmallVector<Instruction*, 4> Insts;
    DenseSet<CallInst*> DeferredInsts;
    if (processAlloca(&AI, false, Insts, DeferredInsts))
    {
        Changed = true;
        NumAllocaPromoted++;
        rewriteTypes(ADDRESS_SPACE_PRIVATE, Insts, DeferredInsts);
    }
}

namespace IGC
{

Pass* createPromoteToScratchPass(void)
{
    return new PromoteToScratchPass();
}

} // namespace IGC
