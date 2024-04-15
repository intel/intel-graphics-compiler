/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/PoisonFP64KernelsPass/PoisonFP64KernelsPass.h"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"


using namespace llvm;
using namespace IGC;

char PoisonFP64Kernels::ID = 0;
const char *PoisonFP64Kernels::attributeName = "invalid_kernel(\"uses-fp64-math\")";
// Register pass to igc-opt
#define PASS_FLAG "igc-poison-fp64-kernels"
#define PASS_DESCRIPTION "Poison kernels using FP64 on unsupported platforms."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PoisonFP64Kernels, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PoisonFP64Kernels, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

PoisonFP64Kernels::PoisonFP64Kernels(void) : CallGraphSCCPass(ID)
{
    initializePoisonFP64KernelsPass(*PassRegistry::getPassRegistry());
}

void PoisonFP64Kernels::markForRemoval(Function *F) {
    if (fp64Functions.count(F) > 0)
       return;

    fp64FunctionsOrder.push_back(F);
    fp64Functions.insert(F);
}

bool PoisonFP64Kernels::doInitialization(CallGraph &CG) {
    fp64Functions.clear();
    fp64FunctionsOrder.clear();
    return false;
}

bool PoisonFP64Kernels::runOnSCC(CallGraphSCC &SCC) {
    auto ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    // If DP emu is enabled we don't need poison fp64 pass
    if (ctx->m_hasDPEmu)
    {
        return false;
    }

    bool modified = false;
    for (CallGraphNode *Node : SCC) {
        Function *F = Node->getFunction();
        if (F == nullptr || F->isDeclaration())
            continue;

        bool markedCallee = false;
        for (BasicBlock &BB : *F) {
            for (Instruction &I : BB) {
                auto *CB = dyn_cast<CallBase>(&I);
                if (CB == nullptr)
                    continue;

                Function *Callee = CB->getCalledFunction();
                if (Callee && Callee->hasFnAttribute("uses-fp64-math")) {
                    F->addFnAttr("uses-fp64-math");
                    modified = true;

                    markForRemoval(Callee);
                    markedCallee = true;
                }
            }
        }

        if (markedCallee || F->hasFnAttribute("uses-fp64-math")) {
            markForRemoval(F);
        }
    }
    return modified;
}

static void poisonKernel(Function *Kernel) {
    Module *M = Kernel->getParent();
    LLVMContext &Ctx = M->getContext();

    Kernel->removeFnAttr("uses-fp64-math");
    Kernel->addFnAttr(PoisonFP64Kernels::attributeName);
    Kernel->deleteBody();

    BasicBlock* EntryBB = BasicBlock::Create(Ctx, "entry", Kernel);
    ReturnInst::Create(Ctx, EntryBB);
}

bool PoisonFP64Kernels::doFinalization(CallGraph &CG) {
    CodeGenContext *CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    IGCMD::MetaDataUtils *MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    bool modified = false;
    for (Function *F : reverse(fp64FunctionsOrder)) {
        std::string msg = "Removing unsupported FP64 function: '" + F->getName().str() + "'";
        CGC->EmitWarning(msg.c_str());

        if (isEntryFunc(MDUtils, F)) {
            poisonKernel(F);
        } else {
            F->eraseFromParent();
        }
        modified = true;
    }

    return modified;
}
