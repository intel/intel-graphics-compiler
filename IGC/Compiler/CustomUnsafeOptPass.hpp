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
#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
llvm::FunctionPass* CreateEarlyOutPatternsPass();
llvm::FunctionPass* CreateLowerFmaPass();
llvm::FunctionPass* CreateHoistFMulInLoopPass();

class CustomUnsafeOptPass : public llvm::FunctionPass, public llvm::InstVisitor<CustomUnsafeOptPass>
{
public:
    static char ID;

    CustomUnsafeOptPass();

    ~CustomUnsafeOptPass() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    virtual bool runOnFunction(llvm::Function &F) override;

    virtual llvm::StringRef getPassName() const override
    {
        return "Custom Unsafe Optimization Pass";
    }

    void visitInstruction(llvm::Instruction &I);
    void visitBinaryOperator(llvm::BinaryOperator &I);
    void visitFCmpInst(llvm::FCmpInst &FC);
    void visitSelectInst(llvm::SelectInst &I);
    void visitCallInst(llvm::CallInst &I);
    void visitFPToSIInst(llvm::FPToSIInst &I);

    bool visitBinaryOperatorPropNegate(llvm::BinaryOperator &I);
    bool visitBinaryOperatorNegateMultiply(llvm::BinaryOperator &I);
    bool visitBinaryOperatorTwoConstants(llvm::BinaryOperator &I);
    bool visitBinaryOperatorDivDivOp(llvm::BinaryOperator &I);
    bool visitBinaryOperatorDivRsq(llvm::BinaryOperator &I);
    bool visitBinaryOperatorAddDiv(llvm::BinaryOperator &I);
    bool visitBinaryOperatorFmulFaddPropagation(llvm::BinaryOperator &I);
    bool visitBinaryOperatorExtractCommonMultiplier(llvm::BinaryOperator &I);
    bool removeCommonMultiplier(llvm::Value *I, llvm::Value *commonMultiplier);
    bool visitBinaryOperatorFmulToFmad(llvm::BinaryOperator &I);
    bool visitBinaryOperatorToFmad(llvm::BinaryOperator &I);
    bool visitBinaryOperatorAddSubOp(llvm::BinaryOperator &I);
    bool visitBinaryOperatorDivAddDiv(llvm::BinaryOperator &I);
    bool isFDiv(llvm::Value *I, llvm::Value *&numerator, llvm::Value *&denominator);
    bool possibleForFmadOpt(llvm::Instruction *inst);
    bool visitFCmpInstFCmpFAddOp(llvm::FCmpInst &FC);
    bool visitFCmpInstFCmpSelOp(llvm::FCmpInst &FC);
    bool visitFMulFCmpOp(llvm::FCmpInst &FC);
    bool visitExchangeCB(llvm::BinaryOperator &I);

    bool m_isChanged;
    bool m_disableReorderingOpt;
    IGC::CodeGenContext *m_ctx;
    IGCMD::MetaDataUtils *m_pMdUtils;

private:
    inline llvm::BinaryOperator* copyIRFlags(
        llvm::BinaryOperator* newOp,
        llvm::Value* oldOp)
    {
        newOp->copyIRFlags(oldOp);

        llvm::DebugLoc dbg = ((llvm::Instruction*)oldOp)->getDebugLoc();
        newOp->setDebugLoc(dbg);

        return newOp;
    }

    void reassociateMulAdd(llvm::Function &F);

    void strengthReducePow(llvm::IntrinsicInst* intrin,
        llvm::Value* exponent);
};

} // namespace IGC
