/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


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

bool BreakConstantExpr::hasConstantExpr(ConstantVector* cvec) const
{
    const uint32_t VecSize = cvec->getNumOperands();
    for (unsigned elemIdx = 0; elemIdx < VecSize; ++elemIdx)
    {
        if (isa<ConstantExpr>(cvec->getOperand(elemIdx))) {
            return true;
        }
    }
    return false;
}

bool BreakConstantExpr::hasConstantExpr(ConstantStruct* cstruct) const
{
    const uint32_t Size0 = cstruct->getNumOperands();
    for (uint32_t i = 0; i < Size0; ++i)
    {
        Value* Ci = cstruct->getOperand(i);
        if (ConstantStruct* aCS = dyn_cast<ConstantStruct>(Ci))
        {
            // Allow 2-level struct (layout struct by LdStCombine is 2 levels)
            uint32_t Size1 = aCS->getNumOperands();
            for (uint32_t j = 0; j < Size1; ++j) {
                Value* Cij = aCS->getOperand(j);
                if (ConstantExpr* c = dyn_cast<ConstantExpr>(Cij)) {
                    return true;
                }
                if (ConstantVector* CVec = dyn_cast<ConstantVector>(Cij)) {
                    if (hasConstantExpr(CVec)) {
                        return true;
                    }
                }
            }
        }
        else if (ConstantExpr* CE = dyn_cast<ConstantExpr>(Ci)) {
            return true;
        }
        if (ConstantVector* CVec = dyn_cast<ConstantVector>(Ci)) {
            if (hasConstantExpr(CVec)) {
                return true;
            }
        }
    }
    return false;
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
                changed |= breakConstantStruct(CS, i, pInst);
            }
        }
    }
    return changed;
}

void BreakConstantExpr::replaceConstantWith(llvm::Constant* exprOrVec, llvm::Instruction* newInst, int operandIndex, llvm::Instruction* user)
{
    if (PHINode* phi = dyn_cast<PHINode>(user))
    {
        newInst->insertBefore(phi->getIncomingBlock(operandIndex)->getTerminator());
        user->setOperand(operandIndex, newInst);
    }
    else if (dyn_cast<DbgInfoIntrinsic>(user))
    {
        newInst->insertBefore(user);
        // For debug info intrinsic, the operand is a metadata that
        // contains the constant expression.
        if (auto* DDI = dyn_cast<DbgDeclareInst>(user))
        {
            MetadataAsValue* MAV = MetadataAsValue::get(user->getContext(), ValueAsMetadata::get(newInst));
            user->setOperand(operandIndex, MAV);
        }
        else
        {
            user->setOperand(operandIndex, newInst);
        }
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
    if (!hasConstantExpr(cvec))
        return false;

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
            if (PHINode* phi = dyn_cast<PHINode>(user))
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

        if (ConstantExpr* expr = dyn_cast<ConstantExpr>(op))
        {
            breakExpressions(expr, 1, newInst);
        }

        currVec = newInst;
    }
    return true;
}

bool BreakConstantExpr::breakConstantStruct(ConstantStruct* cs,
                                            int operandIndex,
                                            Instruction *user)
{
    if (!hasConstantExpr(cs))
        return false;

    // [TODO] Here, localize the changes inside breakConstantStruct to minimize
    //        the impact. It may be worth refactoring breakConstantExpr
    auto insertBefore = [=](Instruction* newInst, Instruction* InsertPos) {
        if (PHINode* phi = dyn_cast<PHINode>(InsertPos)) {
            newInst->insertBefore(phi->getIncomingBlock(operandIndex)->
                getTerminator());
        }
        else {
            newInst->insertBefore(InsertPos);
        }
    };

    auto breakInst = [this](Instruction* I) {
        bool changed = false;
        const int numOperands = (int)I->getNumOperands();
        for (int i = 0; i < numOperands; ++i) {
            Value* op = I->getOperand(i);

            if (ConstantExpr* expr = dyn_cast<ConstantExpr>(op)) {
                breakExpressions(expr, i, I);
                changed = true;
            }
            else if (ConstantVector* cvec = dyn_cast<ConstantVector>(op))
            {   // can this happen ?
                changed |= breakExpressionsInVector(cvec, i, I);
            }
            else if (ConstantStruct* cs = dyn_cast<ConstantStruct>(op))
            {   // can this happen ?
                changed |= breakConstantStruct(cs, i, I);
            }
        }
        return changed;
    };

    StructType *structType = cs->getType();
    IRBuilder<> B(user);
    Value *newStruct = UndefValue::get(structType);

    // Create a structure from scratch, replace every constant operand by an
    // instruction.
    for (unsigned i = 0; i < cs->getNumOperands(); ++i)
    {
        Value* Ci = cs->getOperand(i);

        if (ConstantStruct* aCS = dyn_cast<ConstantStruct>(Ci))
        {
            // Allow 2-level struct (layout struct by LdStCombine is 2 levels)
            for (unsigned j = 0; j < aCS->getNumOperands(); ++j) {
                Value* Cij = aCS->getOperand(j);
                uint32_t idx[2] = { i, j };

                ConstantVector* CE = dyn_cast<ConstantVector>(Cij);
                if (CE && hasConstantExpr(CE)) {
                    Instruction* newInst = InsertValueInst::Create(newStruct, CE, idx);
                    newInst->setDebugLoc(user->getDebugLoc());
                    insertBefore(newInst, user);

                    breakExpressionsInVector(CE, 1, newInst);
                    newStruct = newInst;
                    continue;
                }

                Value* newCij= Cij;
                if (ConstantExpr* c = dyn_cast<ConstantExpr>(Cij))
                {
                    Instruction* newInst = c->getAsInstruction();
                    newInst->setDebugLoc(user->getDebugLoc());
                    newCij = newInst;

                    insertBefore(newInst, user);
                    (void)breakInst(newInst);
                }
                newStruct = B.CreateInsertValue(newStruct, newCij, idx);
            }
            continue;
        }

        ConstantVector* CE = dyn_cast<ConstantVector>(Ci);
        if (CE && hasConstantExpr(CE)) {
            Instruction* newInst = InsertValueInst::Create(newStruct, CE, i);
            newInst->setDebugLoc(user->getDebugLoc());
            insertBefore(newInst, user);

            breakExpressionsInVector(CE, 1, newInst);
            newStruct = newInst;
            continue;
        }

        Value* newCi = Ci;
        if (ConstantExpr* c = dyn_cast<ConstantExpr>(Ci))
        {
            Instruction* newInst = c->getAsInstruction();
            newInst->setDebugLoc(user->getDebugLoc());
            newCi = newInst;

            insertBefore(newInst, user);
            (void)breakInst(newInst);
        }
        newStruct = B.CreateInsertValue(newStruct, newCi, i);
    }

    user->replaceUsesOfWith(cs, newStruct);

    if (cs->use_empty())
    {
        cs->destroyConstant();
    }
    return true;
}
