/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/WGFuncs/WGFuncResolution.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-wg-resolution"
#define PASS_DESCRIPTION "Resolve WG built-in"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WGFuncResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(WGFuncResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WGFuncResolution::ID = 0;

WGFuncResolution::WGFuncResolution() : ModulePass(ID)
{
    initializeWGFuncResolutionPass(*PassRegistry::getPassRegistry());
}

bool WGFuncResolution::runOnModule(Module& M)
{
    m_changed = false;
    m_pModule = &M;

    visit(M);

    return m_changed;
}

void WGFuncResolution::visitCallInst(CallInst& callInst)
{
    Function* pCalledFunc = callInst.getCalledFunction();
    if (!pCalledFunc)
    {
        // Indirect call
        return;
    }
    StringRef funcName = pCalledFunc->getName();
    if (funcName.startswith("__builtin_IB_work_group_any"))
    {
        SmallVector<Value*, 1> args;

        args.push_back(callInst.getOperand(0));

        Function* isaIntrinFunc = GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_WorkGroupAny);
        CallInst* isaIntrinCall = CallInst::Create(isaIntrinFunc, args, callInst.getName(), &callInst);

        isaIntrinCall->setDebugLoc(callInst.getDebugLoc());

        callInst.replaceAllUsesWith(isaIntrinCall);
        callInst.eraseFromParent();
        m_changed = true;
    }
}
