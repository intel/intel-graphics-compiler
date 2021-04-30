/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include <set>

namespace IGC
{
    class HalfPromotion : public llvm::FunctionPass, public llvm::InstVisitor<HalfPromotion>
    {
    public:
        static char ID;

        HalfPromotion();

        virtual llvm::StringRef getPassName() const override
        {
            return "Half Promotion Pass";
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        void visitCallInst(llvm::CallInst& I);
        void visitBinaryOperator(llvm::BinaryOperator& BI);
        void visitFCmp(llvm::FCmpInst& CmpI);
        void visitCastInst(llvm::CastInst& CI);
        void visitPHINode(llvm::PHINode& PHI);
        void visitSelectInst(llvm::SelectInst& SI);

    private:
        void handleGenIntrinsic(llvm::GenIntrinsicInst& I);
        void handleLLVMIntrinsic(llvm::IntrinsicInst& I);

        bool m_changed = false;
    };

} // namespace IGC
