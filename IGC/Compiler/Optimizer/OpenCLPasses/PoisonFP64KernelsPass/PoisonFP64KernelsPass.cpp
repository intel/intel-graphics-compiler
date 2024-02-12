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

static GlobalVariable *createPoisonMessage(Module *M, Function *Kernel) {
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

static Function *getOrDeclareFunction(Module *M, const char *name, FunctionType *type) {
    Function *f = M->getFunction(name);
    if (f == nullptr) {
        f = Function::Create(type, GlobalValue::ExternalLinkage, name, M);
        f->setCallingConv(CallingConv::SPIR_FUNC);
        f->addFnAttr(llvm::Attribute::NoUnwind);
    }
    return f;
}

static void poisonKernel(Function *Kernel) {
    Module *M = Kernel->getParent();
    LLVMContext &Ctx = M->getContext();

    Kernel->removeFnAttr("uses-fp64-math");
    Kernel->addFnAttr(PoisonFP64Kernels::attributeName);
    Kernel->deleteBody();

    BasicBlock* EntryBB = BasicBlock::Create(Ctx, "entry", Kernel);
    BasicBlock* ThenBB = BasicBlock::Create(Ctx, "then", Kernel);
    BasicBlock* ElseBB = BasicBlock::Create(Ctx, "else", Kernel);

    /* Entry block : */

    Type *Int32Ty = Type::getInt32Ty(Ctx);
    /* Get and merge all local and global IDs to make sure that error message is only printed once. */
    Function *GetGlobalID = getOrDeclareFunction(M, "__builtin_IB_get_group_id", FunctionType::get(Int32Ty, { Int32Ty }, true));
    Function *GetLocalIDX = getOrDeclareFunction(M, "__builtin_IB_get_local_id_x", FunctionType::get(Int32Ty, { }, true));
    Function *GetLocalIDY = getOrDeclareFunction(M, "__builtin_IB_get_local_id_y", FunctionType::get(Int32Ty, { }, true));
    Function *GetLocalIDZ = getOrDeclareFunction(M, "__builtin_IB_get_local_id_z", FunctionType::get(Int32Ty, { }, true));

    auto getInt32Const = [&](int v) { return ConstantInt::get(Type::getInt32Ty(Ctx), v);};
    /* Collect all IDs: */
    Value *Ids[6] = {
        CallInst::Create(GetGlobalID, { getInt32Const(0) }, "gid0", EntryBB),
        CallInst::Create(GetGlobalID, { getInt32Const(1) }, "gid1", EntryBB),
        CallInst::Create(GetGlobalID, { getInt32Const(2) }, "gid2", EntryBB),
        CallInst::Create(GetLocalIDX, { }, "lid0", EntryBB),
        CallInst::Create(GetLocalIDY, { }, "lid1", EntryBB),
        CallInst::Create(GetLocalIDZ, { }, "lid2", EntryBB),
    };
    /* Merge IDs into a single value: */
    Value *GlobalID = Ids[0];
    for (int i = 1; i < 6; i++) {
        GlobalID = BinaryOperator::CreateOr(GlobalID, Ids[i], "id.merge", EntryBB);
    }

    Value *Zero = ConstantInt::get(GlobalID->getType(), 0);
    Value *Cond = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_EQ, GlobalID, Zero, "id.is.zero", EntryBB);
    BranchInst::Create(ThenBB, ElseBB, Cond, EntryBB);

    /* Then block : */

    Function *Printf = getOrDeclareFunction(M, "printf", FunctionType::get(Int32Ty, { Type::getInt8PtrTy(Ctx, 2) }, true));

    GlobalVariable* PoisonMessage = createPoisonMessage(M, Kernel);
    Type* MessageType = PoisonMessage->getValueType();

    std::vector<Value *> Indices = {
        ConstantInt::getSigned(Type::getInt32Ty(Ctx), 0),
        ConstantInt::getSigned(Type::getInt32Ty(Ctx), 0)
    };
    GetElementPtrInst *GEP = GetElementPtrInst::Create(MessageType, PoisonMessage, Indices, "posion.message.gep", ThenBB);
    GEP->setIsInBounds(true);

    CallInst::Create(Printf, { GEP }, "printf.result", ThenBB);
    ReturnInst::Create(Ctx, ThenBB);

    /* Else block : */

    ReturnInst::Create(Ctx, ElseBB);
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
