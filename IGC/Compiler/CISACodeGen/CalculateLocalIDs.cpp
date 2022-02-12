/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Constants.h>
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"
#include "CalculateLocalIDs.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/IGCIRBuilder.h"
#include "iStdLib/utility.h"
#include "Probe/Assertion.h"


namespace IGC
{
using namespace llvm;

// On XeHP_SDV+ the compiler can use HW generate IDs. The condition is that at least
// two workgroup size dimensions must be a power of 2. When 2 or 3 dimensions
// are not a power of 2 the pass replaces THREAD_ID_IN_GROUP_X,
// THREAD_ID_IN_GROUP_Y, THREAD_ID_IN_GROUP_Z SGV loads with calculated local
// IDs.
class CalculateLocalIDs : public FunctionPass, public InstVisitor<CalculateLocalIDs>
{
public:
    CalculateLocalIDs() :
        m_WorkgroupSize{},
        FunctionPass(ID)
    { }

    virtual bool runOnFunction(Function& F);

    virtual void getAnalysisUsage(AnalysisUsage& AU) const
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    virtual StringRef getPassName() const { return "CalculateLocalIDs"; }

    void visitCallInst(CallInst& callInst);
    static char ID;

private:
    void GetWorkgroupSize(Function& F);
    void CalculateThreadID(GenIntrinsicInst* write);
    Value* CreateURem(
        IGCIRBuilder<>& builder,
        Value* dividend,
        uint divisor,
        const char* name);
    Value* CreateUDiv(
        IGCIRBuilder<>& builder,
        Value* dividend,
        uint divisor,
        const char* name);

    uint m_WorkgroupSize[3];
    SmallVector<GenIntrinsicInst*, 32> m_ThreadIDs;
};

// Register pass to igc-opt
#define PASS_FLAG "igc-calculate-local-ids"
#define PASS_DESCRIPTION "Calculate local Id's"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CalculateLocalIDs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CalculateLocalIDs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char CalculateLocalIDs::ID = 0;

bool CalculateLocalIDs::runOnFunction(Function& F)
{
    GetWorkgroupSize(F);
    IGC_ASSERT(m_WorkgroupSize[0] > 0);
    IGC_ASSERT(m_WorkgroupSize[1] > 0);
    IGC_ASSERT(m_WorkgroupSize[2] > 0);

    bool isPow2[3] = {
        iSTD::IsPowerOfTwo(m_WorkgroupSize[0]),
        iSTD::IsPowerOfTwo(m_WorkgroupSize[1]),
        iSTD::IsPowerOfTwo(m_WorkgroupSize[2]) };

    // HW generated IDs can only be used if at least 2 dimensions are a power
    // of 2.
    bool useHWGeneratedIDs =
        (isPow2[0] && isPow2[1]) ||
        (isPow2[0] && isPow2[2]) ||
        (isPow2[1] && isPow2[2]);

    if (useHWGeneratedIDs)
    {
        return false;
    }

    m_ThreadIDs.clear();
    visit(F);
    bool modified = false;

    for (GenIntrinsicInst* write : m_ThreadIDs)
    {
        CalculateThreadID(write);
        modified = true;
    }

    return modified;
}

SGVUsage GetUsage(GenIntrinsicInst* inst)
{
    const GenISAIntrinsic::ID IID = inst->getIntrinsicID();
    IGC_ASSERT(IID == GenISAIntrinsic::GenISA_DCL_SystemValue);
    IGC_ASSERT(isa<ConstantInt>(inst->getOperand(0)));
    ConstantInt* op0 = cast<ConstantInt>(inst->getOperand(0));
    SGVUsage usage = static_cast<SGVUsage>(op0->getZExtValue());
    return usage;
}

void CalculateLocalIDs::GetWorkgroupSize(Function& F)
{
    Module* module = F.getParent();

    auto GetGlobalInitializer = [&module]
    (const char* name)->uint
    {
        GlobalVariable* glob = module->getGlobalVariable(name);
        IGC_ASSERT(glob);
        IGC_ASSERT(glob->getInitializer());
        IGC_ASSERT(isa<ConstantInt>(glob->getInitializer()));

        uint ret = int_cast<uint>(
            cast<ConstantInt>(glob->getInitializer())->getZExtValue());
        return ret;
    };

    m_WorkgroupSize[0] = GetGlobalInitializer("ThreadGroupSize_X");
    m_WorkgroupSize[1] = GetGlobalInitializer("ThreadGroupSize_Y");
    m_WorkgroupSize[2] = GetGlobalInitializer("ThreadGroupSize_Z");
}

void CalculateLocalIDs::visitCallInst(CallInst& callInst)
{
    if (GenIntrinsicInst* intrinsic = dyn_cast<GenIntrinsicInst>(&callInst))
    {
        const GenISAIntrinsic::ID IID = intrinsic->getIntrinsicID();
        if (IID == GenISAIntrinsic::GenISA_DCL_SystemValue)
        {
            SGVUsage usage = GetUsage(intrinsic);
            if (THREAD_ID_IN_GROUP_X == usage ||
                THREAD_ID_IN_GROUP_Y == usage ||
                THREAD_ID_IN_GROUP_Z == usage)
            {
                m_ThreadIDs.push_back(intrinsic);
            }
        }
    }
}

// This pass is enabled on platforms that do not support integer division.
// Local workgroup size is smaller than 1024, fp32 division can be used to
// emulate division.
static const bool useExactIntegerDiv = false; // fallback for debug
Value* CalculateLocalIDs::CreateURem(
    IGCIRBuilder<>& builder,
    Value* dividend,
    uint divisor,
    const char* name)
{
    IGC_ASSERT(dividend->getType()->isIntegerTy(16));
    if (useExactIntegerDiv)
    {
        return builder.CreateURem(
            dividend,
            builder.getInt16(divisor),
            VALUE_NAME(name));
    }
    Value* quotient = CreateUDiv(builder, dividend, divisor, "");
    return builder.CreateSub(
        dividend,
        builder.CreateMul(quotient, builder.getInt16(divisor)),
        VALUE_NAME(name));
}
Value* CalculateLocalIDs::CreateUDiv(
    IGCIRBuilder<>& builder,
    Value* dividend,
    uint divisor,
    const char* name)
{
    IGC_ASSERT(dividend->getType()->isIntegerTy(16));
    if (useExactIntegerDiv)
    {
        return builder.CreateUDiv(
            dividend,
            builder.getInt16(divisor),
            VALUE_NAME(name));
    }

    // Convert integer division q = a / b; to:
    //   float af = (float)a;
    //   float bf = (float)b;
    //   q = (int)(af/bf);

    // All integer values smaller than 16777217 can be represented exactly in
    // IEEE fp32.
    Value* af = builder.CreateUIToFP(dividend, builder.getFloatTy());
    Value* bf = ConstantFP::get(builder.getFloatTy(), (float)(divisor));
    Value* qf = builder.CreateFDiv(af, bf);
    Value* q = builder.CreateFPToUI(qf, dividend->getType());
    return q;
}

void CalculateLocalIDs::CalculateThreadID(GenIntrinsicInst* intr)
{
    IGC_ASSERT(nullptr != intr);
    IGCIRBuilder<> builder(intr);

    Module* module = intr->getParent()->getParent()->getParent();
    Function* systemValueIntrinsic = GenISAIntrinsic::getDeclaration(
        module,
        GenISAIntrinsic::GenISA_DCL_SystemValue,
        builder.getInt16Ty());

    Value* subgroupId = builder.CreateCall(
        systemValueIntrinsic,
        builder.getInt32(THREAD_ID_WITHIN_THREAD_GROUP));

    Function* simdSizeIntrinsic = GenISAIntrinsic::getDeclaration(
        module,
        GenISAIntrinsic::GenISA_simdSize);

    Value* simdSize = builder.CreateZExtOrTrunc(
        builder.CreateCall(simdSizeIntrinsic),
        builder.getInt16Ty());

    Function* simdLaneIdIntrinsic = GenISAIntrinsic::getDeclaration(
        module,
        GenISAIntrinsic::GenISA_simdLaneId);

    Value* subgroupLocalInvocationId = builder.CreateZExtOrTrunc(
        builder.CreateCall(simdLaneIdIntrinsic),
        builder.getInt16Ty());

    Value* flatId = builder.CreateAdd(
        builder.CreateMul(subgroupId, simdSize),
        subgroupLocalInvocationId,
        VALUE_NAME("gl_LocalInvocationIndex"));

    Value* calculatedId = builder.getInt16(0);
    switch (GetUsage(intr))
    {
    case THREAD_ID_IN_GROUP_X:
        if (m_WorkgroupSize[0] > 1)
        {
            calculatedId = CreateURem(
                builder,
                flatId,
                m_WorkgroupSize[0],
                VALUE_NAME("gl_LocalInvocationID[0]"));
        }
        break;
    case THREAD_ID_IN_GROUP_Y:
        if (m_WorkgroupSize[1] > 1)
        {
            calculatedId = CreateURem(
                builder,
                CreateUDiv(builder, flatId, m_WorkgroupSize[0], ""),
                m_WorkgroupSize[1],
                VALUE_NAME("gl_LocalInvocationID[1]"));
        }
        break;
    case THREAD_ID_IN_GROUP_Z:
        if (m_WorkgroupSize[2] > 1)
        {
            calculatedId = CreateUDiv(
                builder,
                flatId,
                m_WorkgroupSize[0] * m_WorkgroupSize[1],
                VALUE_NAME("gl_LocalInvocationID[2]"));
        }
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "Unexpected system value type");
    }
    IGC_ASSERT(intr->getType()->isIntegerTy(16) ||
        intr->getType()->isIntegerTy(32));
    calculatedId = builder.CreateZExt(calculatedId, intr->getType());
    intr->replaceAllUsesWith(calculatedId);
    intr->eraseFromParent();
}

FunctionPass* createCalculateLocalIDsPass()
{
    return new CalculateLocalIDs();
}

} //namespace IGC
