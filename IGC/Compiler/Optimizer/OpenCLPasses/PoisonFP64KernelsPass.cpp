/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/PoisonFP64KernelsPass.h"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
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

static Constant *createPoisonMessage(Module *M, Function *Kernel) {
    std::string message = "[CRITICAL ERROR] Kernel '" + Kernel->getName().str()
                        + "' removed due to usage of FP64 instructions unsupported by the targeted hardware. "
                        + "Running this kernel may result in unexpected results.\n";
    std::string varName = "poison.message." + Kernel->getName().str();

    LLVMContext &Ctx = M->getContext();
    Type *StringType = ArrayType::get(Type::getInt8Ty(Ctx), message.size() + 1);

    Constant *StringValue = ConstantDataArray::getString(Ctx, message, true);
    GlobalVariable *GV = new GlobalVariable(
        *M, StringType, true, GlobalValue::InternalLinkage, StringValue,
        varName, nullptr, GlobalValue::ThreadLocalMode::NotThreadLocal, 2);
    GV->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
    return GV;
}

static void poisonKernel(Function *Kernel) {
    Module *M = Kernel->getParent();
    LLVMContext &Ctx = M->getContext();

    Kernel->removeFnAttr("uses-fp64-math");
    Kernel->deleteBody();
    BasicBlock* BB = BasicBlock::Create(Ctx, "entry", Kernel);

    Function *F = M->getFunction("printf");
    if (F == nullptr) {
        FunctionType *FT = FunctionType::get(
            Type::getInt32Ty(Ctx), { Type::getInt8PtrTy(Ctx, 2) }, true);
        F = Function::Create(FT, GlobalValue::ExternalLinkage, "printf", M);
        F->setCallingConv(CallingConv::SPIR_FUNC);
        F->addFnAttr(llvm::Attribute::NoUnwind);
    }

    Constant *PoisonMessage = createPoisonMessage(M, Kernel);
    Type *MessageType = static_cast<PointerType *>(PoisonMessage->getType())->getPointerElementType();

    std::vector<Value *> Indices = {
        ConstantInt::getSigned(Type::getInt32Ty(Ctx), 0),
        ConstantInt::getSigned(Type::getInt32Ty(Ctx), 0)
    };
    GetElementPtrInst *GEP = GetElementPtrInst::Create(MessageType, PoisonMessage, Indices, "posion.message.gep", BB);
    GEP->setIsInBounds(true);

    CallInst::Create(F, { GEP }, "printf.result", BB);
    ReturnInst::Create(Ctx, BB);
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
