/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

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
#include "llvm/ADT/SmallVector.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CISACodeGen/helper.h"
#include "common/IGCIRBuilder.h"

namespace IGC
{
    class CodeGenContext;
}

namespace IGC
{
    class LowPrecisionOpt : public llvm::FunctionPass, public llvm::InstVisitor<LowPrecisionOpt>
    {
    private:
        llvm::IGCIRBuilder<>* m_builder = nullptr;
        bool m_changed{};
        llvm::Function* m_func_llvm_GenISA_DCL_inputVec_f16;
        llvm::Function* m_func_llvm_GenISA_DCL_inputVec_f32;
        llvm::Function* m_currFunction;
        llvm::Function* func_llvm_floor_f32;
        ShaderType shdrType{};
        typedef struct _moveBundle
        {
            uint index;
            llvm::GenIntrinsicInst* cInst;
            llvm::FPTruncInst* fpTrunc;
        }moveBundle;
        struct _cmpOperator
        {
            bool operator()(const moveBundle& obj1, const moveBundle& obj2) { return obj1.index > obj2.index; };
        }cmpOperator;
        llvm::SmallVector<moveBundle, 11> bundles;
        bool m_changeSample = false;
        bool m_simplifyAlu = false;
    public:

        static char ID;

        LowPrecisionOpt();

        ~LowPrecisionOpt() {}

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "LowPrecisionOpt Pass";
        }

        void visitFPExtInst(llvm::FPExtInst& I);
        void visitFPTruncInst(llvm::FPTruncInst& I);
        void visitIntrinsicInst(llvm::IntrinsicInst& I);
        void visitCallInst(llvm::CallInst& I);
        bool propagateSamplerType(llvm::GenIntrinsicInst& I);
    };
} // namespace IGC
