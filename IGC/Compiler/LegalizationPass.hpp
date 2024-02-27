/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CodeGenContext;
}

namespace IGC
{
    class Legalization : public llvm::FunctionPass, public llvm::InstVisitor<Legalization>
    {
        bool m_preserveNan;

        // With option "-finite-math-only", IGC ignores all nans but keeps isnan
        // checks. That is, in the fast mode, isnan will be honored. We also
        // *assume* all isnan checks are lowered into the following forms:
        //   %b = fcmp uno float %x, 0.000000e+00 or %b = fcmp une float %x, %x
        // All other forms will be optimized away. This is a less ideal workaround
        // to limit the scope. A proper fix is to keep isnan check as an intrinsic
        // call, but implementing this requires to rewrite ocl builtins.
        bool m_preserveNanCheck;

        const llvm::DataLayout* m_DL;
        std::vector<llvm::Instruction*> m_instructionsToRemove;
        llvm::IRBuilder<>* m_builder = nullptr;
        IGC::CodeGenContext* m_ctx = nullptr;
    public:

        static char ID;

        Legalization(bool preserveNan = false);

        ~Legalization() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "Legalization Pass";
        }

        void visitInstruction(llvm::Instruction& I);
        void visitCallInst(llvm::CallInst& I);
        void visitSelectInst(llvm::SelectInst& I);
        void visitPHINode(llvm::PHINode& I);
        void visitICmpInst(llvm::ICmpInst& IC);
        void visitFCmpInst(llvm::FCmpInst& FC);
        void visitFCmpInstUndorderedPredicate(llvm::FCmpInst& FC);
        void visitFCmpInstUndorderedFlushNan(llvm::FCmpInst& FC);
        void visitInsertElementInst(llvm::InsertElementInst& I);
        void visitShuffleVectorInst(llvm::ShuffleVectorInst& I);
        void visitStoreInst(llvm::StoreInst& I);
        void visitLoadInst(llvm::LoadInst& I);
        void visitAlloca(llvm::AllocaInst& I);
        void visitIntrinsicInst(llvm::IntrinsicInst& I);
        void visitBitCastInst(llvm::BitCastInst& I);
        void visitBasicBlock(llvm::BasicBlock& BB);
        void visitTruncInst(llvm::TruncInst&);
        void visitUnaryInstruction(llvm::UnaryInstruction &I);
        void visitBinaryOperator(llvm::BinaryOperator& I);
        void visitAddrSpaceCastInst(llvm::AddrSpaceCastInst&);

    protected:
        llvm::Value* addFCmpWithORD(llvm::FCmpInst& FC);
        llvm::Value* addFCmpWithUNO(llvm::FCmpInst& FC);
        llvm::Value* findInsert(llvm::Value* vector, unsigned int index);
        llvm::Type* LegalAllocaType(llvm::Type* type) const;
        llvm::Type* LegalStructAllocaType(llvm::Type* type) const;

        void RecursivelyChangePointerType(llvm::Instruction* oldPtr, llvm::Type* Ty, llvm::Instruction* newPtr);
        void PromoteFp16ToFp32OnGenSampleCall(llvm::CallInst& I);
        void PromoteInsertElement(llvm::Value* I, llvm::Value* newVec);

        /// \brief Ensure a function have a unique return instruction.
        void unifyReturnInsts(llvm::Function& F);

    private:
        llvm::DenseMap<llvm::Value*, llvm::Value*>fpMap;
    };

    // Legalize IR out of LLVM optimization passes (such as GVN).
    llvm::FunctionPass* createGenOptLegalizer();

    // Emulate FDIV instructions.
    llvm::FunctionPass* createGenFDIVEmulation();

} // namespace IGC
