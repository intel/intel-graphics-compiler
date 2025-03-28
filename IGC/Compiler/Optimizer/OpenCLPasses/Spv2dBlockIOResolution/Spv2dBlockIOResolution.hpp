/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "AdaptorOCL/Utils/CacheControlsHelper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

namespace IGC {
    class Spv2dBlockIOResolution final
        : public llvm::ModulePass
        , public llvm::InstVisitor<Spv2dBlockIOResolution> {
    public:
        static char ID;

        Spv2dBlockIOResolution();
        ~Spv2dBlockIOResolution() {}

        virtual llvm::StringRef getPassName() const override {
            return "Spv2dBlockIOResolution";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnModule(llvm::Module& M) override;
        void visitCallInst(llvm::CallInst& CI);

    private:
        enum Op {
            Load,
            LoadTranspose,
            LoadTransform,
            Prefetch,
            Store,
        };

        template<typename CCT>
        void visit2DBlockSPVCallInst(llvm::CallInst& CI, Op op);
        bool isConstantInstruction(llvm::Value* val, llvm::StringRef valName);
        template<typename CCT>
        CacheControlFromMDNodes resolveCacheControlDecorations(llvm::Value *pointerValue);

        llvm::DenseSet<llvm::Function*> m_BuiltinsToRemove;
        bool m_Changed = false;
        IGC::CodeGenContext* m_Ctx = nullptr;
        llvm::Module* m_Module = nullptr;
    };
};
