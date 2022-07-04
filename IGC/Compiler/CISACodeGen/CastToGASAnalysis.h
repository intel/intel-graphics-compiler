/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    enum {
        HasPrivateToGenericCast = 1 << 0,
        HasLocalToGenericCast = 1 << 1,
    };

    class GASInfo {
    public:
        bool canGenericPointToPrivate(llvm::Function& F) const {
            auto E = FunctionMap.find(&F);
            if (E == FunctionMap.end())
                return true;

            return E->second & HasPrivateToGenericCast;
        }

        bool canGenericPointToLocal(llvm::Function& F) const {
            auto E = FunctionMap.find(&F);
            if (E == FunctionMap.end())
                return true;

            return E->second & HasLocalToGenericCast;
        }
    private:
        using FunctionMapTy = llvm::DenseMap<const llvm::Function*, unsigned>;
        FunctionMapTy FunctionMap;

        friend class CastToGASWrapperPass;
    };

    class CastToGASWrapperPass : public llvm::ModulePass {
    public:
        static char ID;

        CastToGASWrapperPass() : llvm::ModulePass(ID) {}

        ~CastToGASWrapperPass() = default;

        virtual llvm::StringRef getPassName() const override
        {
            return "Cast To GAS Analysis";
        }

        bool runOnModule(llvm::Module & M) override;

        void getAnalysisUsage(llvm::AnalysisUsage & AU) const override {
            AU.setPreservesAll();
            AU.addRequired<llvm::CallGraphWrapperPass>();
        }

        GASInfo& getGASInfo() { return GI; }

    private:
        GASInfo GI;
        llvm::DenseMap<const llvm::Function*, unsigned> castInfoCache;

        void setInfoForGroup(
            llvm::SmallPtrSetImpl<const llvm::Function*>& functionsGroup,
            unsigned castInfo);

        unsigned hasCastsToGeneric(const llvm::Function* F);

        void getAllFuncsAccessibleFromKernel(
            const llvm::Function* F,
            llvm::CallGraph& CG,
            llvm::SmallPtrSetImpl<const llvm::Function*>& funcs,
            bool& disruptAnalysis) const;
    };
}
