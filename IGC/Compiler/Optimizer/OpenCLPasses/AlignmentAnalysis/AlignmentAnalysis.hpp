/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"

namespace IGC
{

    /// @brief  This pass correctly sets the alignment of all loads
    ///         and stores according to OpenCL rules.
    ///         It perform a data-flow analysis to keep track of the highest power
    ///         of 2 that divides a value.
    ///         This tracking is performed for integers and for pointers, and is
    ///         based on the assumption that global variables and kernel arguments
    ///         are always aligned on their data type.
    ///         The result is an underapproximation of the actual alignment, so it
    ///         is always safe.
    class AlignmentAnalysis : public llvm::FunctionPass, public llvm::InstVisitor<AlignmentAnalysis, alignment_t>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        AlignmentAnalysis();

        /// @brief  Destructor
        ~AlignmentAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "AlignmentAnalysisPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        /// @brief  Main entry point.
        virtual bool runOnFunction(llvm::Function& F) override;

        // @ brief Instruction visitors
        alignment_t visitInstruction(llvm::Instruction& I);
        alignment_t visitAllocaInst(llvm::AllocaInst& I);
        alignment_t visitIntToPtrInst(llvm::IntToPtrInst& I);
        alignment_t visitPtrToIntInst(llvm::PtrToIntInst& I);
        alignment_t visitSelectInst(llvm::SelectInst& I);
        alignment_t visitGetElementPtrInst(llvm::GetElementPtrInst& I);
        alignment_t visitPHINode(llvm::PHINode& I);
        alignment_t visitBitCastInst(llvm::BitCastInst& I);
        alignment_t visitAdd(llvm::BinaryOperator& I);
        alignment_t visitMul(llvm::BinaryOperator& I);
        alignment_t visitShl(llvm::BinaryOperator& I);
        alignment_t visitAnd(llvm::BinaryOperator& I);
        alignment_t visitTruncInst(llvm::TruncInst& I);
        alignment_t visitZExtInst(llvm::ZExtInst& I);
        alignment_t visitSExtInst(llvm::SExtInst& I);
        alignment_t visitCallInst(llvm::CallInst& I);

        void SetInstAlignment(llvm::Instruction& I);
        void SetInstAlignment(llvm::LoadInst& I);
        void SetInstAlignment(llvm::StoreInst& I);
        void SetInstAlignment(llvm::MemSetInst& I);
        void SetInstAlignment(llvm::MemCpyInst& I);
        void SetInstAlignment(llvm::MemMoveInst& I);

    protected:
        /// @breif Evaluates the alignment of I based on its operands.
        ///        For Load and Store instructions, also sets the alignment
        ///        of the operation itself.
        /// @param I The instruction to process
        bool processInstruction(llvm::Instruction* I);

        /// @brief Returns the alignment for V, if it is known.
        ///        Otherwise, returns the maximum alignment.
        /// @param V the value the alignment of which we're interested in
        auto getAlignValue(llvm::Value* V) const;

        /// @brief Returns the alignment of a constant integer.
        ///        This is normally 1 << ctz(C) (the highest power of 2 that divides C),
        ///        except when C is 0, when it is the max alignment
        auto getConstantAlignment(uint64_t C) const;

        /// @brief This map stores the known alignment of every value.
        llvm::MapVector<llvm::Value*, alignment_t> m_alignmentMap;

        static const alignment_t MinimumAlignment = 1;

        const llvm::DataLayout* m_DL = nullptr;
    };

} // namespace IGC
