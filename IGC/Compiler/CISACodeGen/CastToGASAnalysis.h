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

#include "Compiler/CodeGenContextWrapper.hpp"

#include <utility>

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

        bool isPrivateAllocatedInGlobalMemory() const {
            return allocatePrivateAsGlobalBuffer;
        }

        bool canGenericPointToLocal(llvm::Function& F) const {
            auto E = FunctionMap.find(&F);
            if (E == FunctionMap.end())
                return true;

            return E->second & HasLocalToGenericCast;
        }

        bool isNoLocalToGenericOptionEnabled() const {
            return noLocalToGenericOptionEnabled;
        }

    private:
        using FunctionMapTy = llvm::DenseMap<const llvm::Function*, unsigned>;
        FunctionMapTy FunctionMap;

        // True when -cl-intel-no-local-to-generic is enabled
        bool noLocalToGenericOptionEnabled = false;
        bool allocatePrivateAsGlobalBuffer = false;

        friend class CastToGASAnalysis;
    };

    class CastToGASAnalysis : public llvm::ModulePass {
    public:
        static char ID;

        CastToGASAnalysis() : llvm::ModulePass(ID) {}

        ~CastToGASAnalysis() = default;

        virtual llvm::StringRef getPassName() const override
        {
            return "Cast To GAS Analysis";
        }

        bool runOnModule(llvm::Module & M) override;

        void getAnalysisUsage(llvm::AnalysisUsage & AU) const override {
            AU.setPreservesAll();
            AU.addRequired<llvm::CallGraphWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        GASInfo& getGASInfo() { return GI; }

    private:
        CodeGenContext* m_ctx = nullptr;
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

    // EmitPass's analysis passes are either function passes or immutable
    // passes for performance reason. To pass the CastToGASAnalysis's
    // info to EmitPass, the following immutable pass is used for holding
    // info and CastToGASInfoWrapper is used to create this immutable pass.
    // Note that no more CastToGASAnalysis after CastToGASInfoWrapper, as
    // CastToGASInfoWrapper invalides CastToGASAnalysis by taking its 'GI'.
    // Any pass after CastToGASInfoWrapper should use CastToGASInfo immutable
    // pass to access the info.
    class CastToGASInfo : public llvm::ImmutablePass {
    public:
        static char ID;
        GASInfo GI;
    public:
        CastToGASInfo();

        virtual llvm::StringRef getPassName() const override
        {
            return "Cast To GAS info for EmitPass";
        }

        const GASInfo& getGASInfo() const { return GI; }
        void setGASInfo(GASInfo& aGI) {
            GI.~GASInfo();
            GI = std::move(aGI);
        }
    };

    class CastToGASInfoWrapper : public llvm::ModulePass {
    public:
        static char ID;

        CastToGASInfoWrapper();

        bool runOnModule(llvm::Module& M) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "Cast To GAS Info Generation for EmitPass";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesAll();
            AU.addRequired<CastToGASAnalysis>();
            AU.addRequired<CastToGASInfo>();
        }
    };
}
