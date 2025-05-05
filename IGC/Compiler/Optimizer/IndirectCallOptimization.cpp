/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/IndirectCallOptimization.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/IGCIRBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"
#include <map>
#include "Probe/Assertion.h"

using namespace llvm;

#define PASS_FLAG "indirect-call-optimization"
#define PASS_DESCRIPTION "Changes indirect calls to direct calls if possible"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

namespace IGC
{
    // Register pass to igc-opt
    IGC_INITIALIZE_PASS_BEGIN(IndirectCallOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_END(IndirectCallOptimization, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

    char IndirectCallOptimization::ID = 0;

    IndirectCallOptimization::IndirectCallOptimization() : FunctionPass(ID)
    {
        initializeIndirectCallOptimizationPass(*PassRegistry::getPassRegistry());
    }

    bool IndirectCallOptimization::runOnFunction(Function& F)
    {
        bool modified = false;
        auto BI = F.begin(), BE = F.end();
        for (; BI != BE; BI++)
        {
            auto II = BI->begin(), IE = BI->end();
            while (II != IE)
            {
                if (CallInst* call = dyn_cast<CallInst>(&*II))
                {
                    if (visitCallInst(*call))
                    {
                        // Iterators have been invalidated, so we need to update them
                        II = call->getNextNode()->getIterator();
                        IE = call->getParent()->getTerminator()->getIterator();
                        BI = call->getParent()->getIterator();
                        BE = F.end();
                        call->eraseFromParent();
                        modified = true;
                        continue;
                    }
                }
                II++;
            }
        }

        return modified;
    }

    inline bool CompareCallFuncSignature(CallInst* call, Function* func)
    {
        if (func->getReturnType() != call->getType())
            return false;

        if (func->arg_size() != IGCLLVM::getNumArgOperands(call))
            return false;

        unsigned index = 0;
        for (auto ai = func->arg_begin(), ei = func->arg_end(); ai != ei; ai++)
        {
            if (ai->getType() != call->getArgOperand(index++)->getType())
            {
                return false;
            }
        }
        return true;
    }

    bool IndirectCallOptimization::visitCallInst(CallInst &CI)
    {
        // Not an indirect call
        if (CI.getCalledFunction() || CI.isInlineAsm()) return false;

        Function* currFunc = CI.getParent()->getParent();
        Module* pModule = currFunc->getParent();
        SmallSet<Function*, 8> CallableFuncs;

        // function groups are stored in the !callees metadata
        if (MDNode* callmd = CI.getMetadata("callees"))
        {
            for (auto& op : callmd->operands())
            {
                if (Function* F = mdconst::dyn_extract<Function>(op))
                {
                    if (F->hasFnAttribute("referenced-indirectly") && CompareCallFuncSignature(&CI, F))
                    {
                        CallableFuncs.insert(F);
                        continue;
                    }
                }
                IGC_ASSERT_MESSAGE(0, "Invalid function in function group!");
                return false;
            }
        }

        if (CallableFuncs.empty())
        {
            // Nothing to do
            return false;
        }
        else if (CallableFuncs.size() == 1)
        {
            // Trivial case, replace with the direct call
            Function* F = *CallableFuncs.begin();
            SmallVector<Value*, 8> callArgs(CI.arg_begin(), CI.arg_end());
            IGCIRBuilder<> IRB(pModule->getContext());
            IRB.SetInsertPoint(&CI);
            CallInst* dirCall = IRB.CreateCall(F, callArgs);
            CI.replaceAllUsesWith(dirCall);
            return true;
        }


        // Add some checks to determine if inlining is profitable

        // Limit the number of branches
        if (CallableFuncs.size() > 4)
            return false;

        // Limit max number of instructions added after inlining all functions
        unsigned maxInlinedInsts = 0;
        for (auto pFunc : CallableFuncs)
        {
            maxInlinedInsts += pFunc->getInstructionCount();
            // Use the OCLInlineThreshold for now
            if (maxInlinedInsts > IGC_GET_FLAG_VALUE(OCLInlineThreshold))
            {
                return false;
            }
        }

        SmallVector<std::pair<Value*, BasicBlock*>, 8> callToBBPair;
        SmallVector<Value*, 8> callArgs(CI.arg_begin(), CI.arg_end());
        IGCIRBuilder<> IRB(pModule->getContext());
        IRB.SetInsertPoint(&CI);

        Value* calledAddr = IRB.CreatePtrToInt(IGCLLVM::getCalledValue(CI), IRB.getInt64Ty());

        BasicBlock* beginBlock = CI.getParent();
        BasicBlock* endBlock = beginBlock->splitBasicBlock(&CI, "endIndirectCallBB");

        // Remove the original terminator created during the splitBasicBlock. We will replace it with a conditional branch
        beginBlock->getTerminator()->eraseFromParent();

        std::map<Function*, std::pair<BasicBlock*, BasicBlock*>> funcToBBPairMap;
        for (auto pFunc : CallableFuncs)
        {
            BasicBlock* callBlock = BasicBlock::Create(pModule->getContext(), "funcThen", currFunc, endBlock);
            BasicBlock* condBlock = BasicBlock::Create(pModule->getContext(), "funcElse", currFunc, endBlock);
            funcToBBPairMap[pFunc] = std::make_pair(callBlock, condBlock);
        }

        IRB.SetInsertPoint(beginBlock);
        for (auto it = CallableFuncs.begin(), ie = CallableFuncs.end(); it != ie; it++)
        {
            Function* pFunc = *it;
            Value* funcAddr = IRB.CreatePtrToInt(pFunc, IRB.getInt64Ty());
            Value* cnd = IRB.CreateICmpEQ(calledAddr, funcAddr);
            auto brPair = funcToBBPairMap[pFunc];
            IRB.CreateCondBr(cnd, brPair.first, brPair.second);

            IRB.SetInsertPoint(brPair.first);
            CallInst* directCall = IRB.CreateCall(pFunc, callArgs);
            callToBBPair.push_back(std::make_pair(directCall, brPair.first));
            IRB.CreateBr(endBlock);
            IRB.SetInsertPoint(brPair.second);

            if (std::distance(it, ie) == 1)
            {
                // In the final "else" block, if none of the addresses match, do nothing and create undef value
                callToBBPair.push_back(std::make_pair(UndefValue::get(CI.getType()), brPair.second));
                IRB.CreateBr(endBlock);
            }
        }

        // Return values have to be selected based on which block the function was called
        if (callToBBPair.size() > 0 && CI.getType() != IRB.getVoidTy())
        {
            IRB.SetInsertPoint(&*(endBlock->begin()));
            PHINode* phi = IRB.CreatePHI(CI.getType(), callToBBPair.size());
            for (const auto& it : callToBBPair)
            {
                phi->addIncoming(it.first, it.second);
            }
            CI.replaceAllUsesWith(phi);
        }
        return true;
    }
}
