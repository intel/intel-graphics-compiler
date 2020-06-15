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

#include <vector>

#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-break-const-expr"
#define PASS_DESCRIPTION "Break constant expressions into instruction sequences"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BreakConstantExpr, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(BreakConstantExpr, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BreakConstantExpr::ID = 0;

BreakConstantExpr::BreakConstantExpr() : FunctionPass(ID)
{
    initializeBreakConstantExprPass(*PassRegistry::getPassRegistry());
}

bool BreakConstantExpr::runOnFunction(Function& F)
{
    bool changed = false;
    // Go over all the instructions in the function
    for (inst_iterator it = inst_begin(F), e = inst_end(F); it != e; ++it)
    {
        Instruction* pInst = &*it;
        if (DbgDeclareInst * DbgDclInst = dyn_cast<DbgDeclareInst>(pInst)) {
            // For DbgDeclareInst, the operand is a metadata that might
            // contain a constant expression.
            Value* op = DbgDclInst->getAddress();
            // If the debug adress is a constant expression, recursively break it up.
            if (ConstantExpr * expr = dyn_cast_or_null<ConstantExpr>(op))
            {
                breakExpressions(expr, 0, pInst);
                changed = true;
            }
            continue;
        }
#if 0
        //Disable handling of llvm.dbg.value instruction as it needs
        //proper handling of metadata.
        if (DbgValueInst * DbgValInst = dyn_cast<DbgValueInst>(pInst)) {
            // For DbgValueInst, the operand is a metadata that might
            // contain a constant expression.
            Value* op = DbgValInst->getValue();
            // If the debug value operand is a constant expression, recursively break it up.
            if (ConstantExpr * expr = dyn_cast_or_null<ConstantExpr>(op))
            {
                breakExpressions(expr, 0, pInst);
                changed = true;
            }
            continue;
        }
#endif

        // And all the operands of each instruction
        int numOperands = it->getNumOperands();
        for (int i = 0; i < numOperands; ++i)
        {
            Value* op = it->getOperand(i);

            // If the operand is a constant expression, recursively break it up.
            if (ConstantExpr * expr = dyn_cast<ConstantExpr>(op))
            {
                breakExpressions(expr, i, pInst);
                changed = true;
            }
            else if (ConstantVector * cvec = dyn_cast<ConstantVector>(op))
            {
                changed |= breakExpressionsInVector(cvec, i, pInst);
            }
            else if (ConstantStruct *CS = dyn_cast<ConstantStruct>(op))
            {
                breakConstantStruct(CS, i, pInst);
                changed = true;
            }
        }
    }
    return changed;
}

void BreakConstantExpr::replaceConstantWith(llvm::Constant* exprOrVec, llvm::Instruction* newInst, int operandIndex, llvm::Instruction* user)
{
    if (PHINode * phi = dyn_cast<PHINode>(user))
    {
        newInst->insertBefore(phi->getIncomingBlock(operandIndex)->getTerminator());
        user->setOperand(operandIndex, newInst);
    }
    else if (dyn_cast<DbgInfoIntrinsic>(user))
    {
        newInst->insertBefore(user);
        // For debug info intrinsic, the operand is a metadata that
        // contains the constant expression.
#if 1
        // llvm 3.6.0 transition
        user->setOperand(operandIndex, newInst);
#else
        IGC_ASSERT(0);
#endif
    }
    else
    {
        newInst->insertBefore(user);
        user->replaceUsesOfWith(exprOrVec, newInst);
    }
    if (exprOrVec->use_empty())
    {
        exprOrVec->destroyConstant();
    }
}

void BreakConstantExpr::breakExpressions(llvm::ConstantExpr* expr, int operandIndex, llvm::Instruction* user)
{
    // Create a new instruction, and insert it at the appropriate point.
    Instruction* newInst = expr->getAsInstruction();
    newInst->setDebugLoc(user->getDebugLoc());

    replaceConstantWith(expr, newInst, operandIndex, user);

    // Thew new instruction may itself reference constant expressions.
    // So, recursively process all of its arguments.
    int numOperands = newInst->getNumOperands();
    for (int i = 0; i < numOperands; ++i)
    {
        Value* op = newInst->getOperand(i);
        ConstantExpr* innerExpr = dyn_cast<ConstantExpr>(op);
        if (innerExpr)
        {
            breakExpressions(innerExpr, i, newInst);
        }
        else if (ConstantVector * cvec = dyn_cast<ConstantVector>(op))
        {
            breakExpressionsInVector(cvec, i, newInst);
        }
    }
}

bool BreakConstantExpr::breakExpressionsInVector(llvm::ConstantVector* cvec, int operandIndex, llvm::Instruction* user)
{
    bool hasConstantExpression = false;
    for (unsigned elemIdx = 0; elemIdx < cvec->getNumOperands(); ++elemIdx)
    {
        if (isa<ConstantExpr>(cvec->getOperand(elemIdx)))
        {
            hasConstantExpression = true;
            break;
        }
    }

    if (hasConstantExpression)
    {
        Value* currVec = UndefValue::get(cvec->getType());
        const unsigned vecSize = cvec->getNumOperands();

        for (unsigned elemIdx = 0; elemIdx < vecSize; ++elemIdx)
        {
            Value* op = cvec->getOperand(elemIdx);
            Instruction* newInst = InsertElementInst::Create(
                currVec,
                op,
                ConstantInt::get(Type::getInt32Ty(user->getContext()), elemIdx));

            if (elemIdx < cvec->getNumOperands() - 1)
            {
                if (PHINode * phi = dyn_cast<PHINode>(user))
                {
                    newInst->insertBefore(phi->getIncomingBlock(operandIndex)->getTerminator());
                }
                else
                {
                    newInst->insertBefore(user);
                }
            }
            else
            {
                // cvec can be destroyed inside!
                replaceConstantWith(cvec, newInst, operandIndex, user);
            }

            if (ConstantExpr * expr = dyn_cast<ConstantExpr>(op))
            {
                breakExpressions(expr, 1, newInst);
            }

            currVec = newInst;
        }
    }

    return hasConstantExpression;
}

void BreakConstantExpr::breakConstantStruct(ConstantStruct* cs,
                                            int operandIndex,
                                            Instruction *user)
{
    StructType *structType = cs->getType();
    IRBuilder<> B(user);
    Value *newStruct = UndefValue::get(structType);

    // Create a structure from scratch, replace every constant operand by an
    // instruction.
    for (unsigned i = 0; i < cs->getNumOperands(); ++i)
    {
        Value *constStructOp = cs->getOperand(i);
        Value *newStructOp = constStructOp;

        // TODO: Handle more cases, such as ConstantStruct, ConstantVector, etc.
        if (ConstantExpr *c = dyn_cast<ConstantExpr>(constStructOp))
        {
            Instruction *newInst = c->getAsInstruction();
            newInst->setDebugLoc(user->getDebugLoc());
            newStructOp = newInst;

            if (PHINode * phi = dyn_cast<PHINode>(user))
            {
                newInst->insertBefore(phi->getIncomingBlock(operandIndex)->
                    getTerminator());
            }
            else
            {
                newInst->insertBefore(user);
            }
        }

        newStruct = B.CreateInsertValue(newStruct, newStructOp, i);
    }

    user->replaceUsesOfWith(cs, newStruct);

    if (cs->use_empty())
    {
        cs->destroyConstant();
    }
}
