/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/IndirectCallOptimization.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/IGCIRBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

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

        if (func->arg_size() != call->getNumArgOperands())
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
        SmallVector<Function*, 8> CallableFuncs;
        bool needFallbackToIndirect = false;

        // Compiler provides a hint for which group of functions can be called by this instruction,
        // stored as a string of function names separated by "," in the "function_groups" metadata.
        if (MDNode * funcgroups = CI.getMetadata("function_group"))
        {
            StringRef funcStr = cast<MDString>(funcgroups->getOperand(0))->getString();
            SmallVector<StringRef, 8> funcNames;
            funcStr.split(funcNames, ',');

            if (funcNames.size() == 0)
            {
                return false;
            }
            else if (funcNames.size() == 1)
            {
                // Trivial case, replace with the direct call
                Function* F = pModule->getFunction(funcNames.front());
                if (F && F->hasFnAttribute("IndirectlyCalled") && CompareCallFuncSignature(&CI, F))
                {
                    SmallVector<Value*, 8> callArgs(CI.arg_begin(), CI.arg_end());
                    IGCIRBuilder<> IRB(pModule->getContext());
                    IRB.SetInsertPoint(&CI);
                    CallInst* dirCall = IRB.CreateCall(F, callArgs);
                    CI.replaceAllUsesWith(dirCall);
                    return true;
                }
                return false;
            }
            else
            {
                for (auto fName : funcNames)
                {
                    Function* F = pModule->getFunction(fName);
                    if (F && F->hasFnAttribute("IndirectlyCalled") && CompareCallFuncSignature(&CI, F))
                    {
                        CallableFuncs.push_back(F);
                    }
                    else
                    {
                        // If hint has a function not in current module, we should still fall back to indirect case
                        needFallbackToIndirect = true;
                    }
                }
            }
        }
        else
        {
            // Find all functions in module that match the call signature
            for (auto& FI : *pModule)
            {
                if (FI.hasFnAttribute("IndirectlyCalled") && CompareCallFuncSignature(&CI, &FI))
                {
                    CallableFuncs.push_back(&FI);
                }
            }
            // Always provide fallback option if hint not provided
            needFallbackToIndirect = true;
        }

        if (CallableFuncs.empty())
        {
            // Nothing to do
            return false;
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

        SmallVector<Instruction*, 8> expandedCalls;
        SmallVector<Value*, 8> callArgs(CI.arg_begin(), CI.arg_end());
        IGCIRBuilder<> IRB(pModule->getContext());
        IRB.SetInsertPoint(&CI);

        Value* calledAddr = IRB.CreatePtrToInt(CI.getCalledValue(), IRB.getInt64Ty());

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
        unsigned numFuncs = CallableFuncs.size();
        for (unsigned i = 0; i < numFuncs; i++)
        {
            Function* pFunc = CallableFuncs[i];
            Value* funcAddr = IRB.CreatePtrToInt(pFunc, IRB.getInt64Ty());
            Value* cnd = IRB.CreateICmpEQ(calledAddr, funcAddr);
            auto brPair = funcToBBPairMap[pFunc];
            IRB.CreateCondBr(cnd, brPair.first, brPair.second);

            IRB.SetInsertPoint(brPair.first);
            CallInst* directCall = IRB.CreateCall(pFunc, callArgs);
            expandedCalls.push_back(directCall);
            IRB.CreateBr(endBlock);
            IRB.SetInsertPoint(brPair.second);

            if (i == (numFuncs - 1))
            {
                if (!needFallbackToIndirect)
                {
                    // Should never reach this block. Dummy branch for code correctness
                    IRB.CreateBr(endBlock);
                }
                else
                {
                    // In the final "else" block, if none of the function address match, then we have to fallback to the indirect call
                    CallInst* indCall = IRB.CreateCall(CI.getCalledValue(), callArgs);
                    expandedCalls.push_back(indCall);
                    IRB.CreateBr(endBlock);
                }
            }
        }

        // Return values have to be selected based on which block the function was called
        if (expandedCalls.size() > 1 && CI.getType() != IRB.getVoidTy())
        {
            IRB.SetInsertPoint(&*(endBlock->begin()));
            PHINode* phi = IRB.CreatePHI(CI.getType(), expandedCalls.size());
            for (auto call : expandedCalls)
            {
                phi->addIncoming(call, call->getParent());
            }
            CI.replaceAllUsesWith(phi);
        }
        return true;
    }
}