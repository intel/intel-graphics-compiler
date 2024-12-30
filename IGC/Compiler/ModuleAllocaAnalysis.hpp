/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"

namespace llvm{
    class AllocaInst;
    class Function;
}

namespace IGC
{
    namespace IGCMD
    {
        class MetaDataUtils;
    }
    class GenXFunctionGroupAnalysis;
    class FunctionGroup;

    /// \brief Analyze alloca instructions and determine the size and offset of
    /// each alloca and the total amount of private memory needed by each kernel.
    class ModuleAllocaAnalysis : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        ModuleAllocaAnalysis();
        ~ModuleAllocaAnalysis();
        ModuleAllocaAnalysis(const ModuleAllocaAnalysis&) = delete;
        ModuleAllocaAnalysis& operator=(const ModuleAllocaAnalysis&) = delete;

        virtual llvm::StringRef getPassName() const override;

        virtual bool runOnModule(llvm::Module& M) override;

        /// @brief  Adds the analysis required by this pass
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        /// Initialize setup like UseScratchSpacePrivateMemory.
        bool safeToUseScratchSpace() const;

        /// \brief Return the offset of alloca instruction in private memory buffer.
        //  This function should not be called when AI is variable length alloca
        unsigned getConstBufferOffset(llvm::AllocaInst* AI) const;

        /// \brief Return the size of alloca instruction in private memory buffer.
        //  This function should not be called when AI is variable length alloca
        unsigned getConstBufferSize(llvm::AllocaInst* AI) const;

        /// \brief Return all alloca instructions of a given function.
        llvm::SmallVector<llvm::AllocaInst*, 8>& getAllocaInsts(llvm::Function* F) const;

        /// \brief Return the total private memory size per WI of a given function.
        unsigned getTotalPrivateMemPerWI(llvm::Function* F) const;

        /// \brief Return maximum alignment among all allocas of a given function.
        alignment_t getPrivateMemAlignment(llvm::Function* F) const;

    private:
        struct FunctionAllocaInfo {
            void setAllocaDesc(llvm::AllocaInst* AI, unsigned Offset, unsigned Size)
            {
                AllocaDesc[AI] = std::make_pair(Offset, Size);
            }

            /// \brief Total amount of private memory size per kernel. All functions in
            /// a kernel will have the same size.
            unsigned TotalSize = 0;

            /// \brief Maximum alignment among all allocas.
            alignment_t MaximumAlignment = 1;

            /// \brief Alloca instructions for a function.
            llvm::SmallVector<llvm::AllocaInst*, 8> Allocas;

            /// \brief Alloca instruction, its offset and size in buffer.
            llvm::DenseMap<llvm::AllocaInst*, std::pair<unsigned, unsigned>> AllocaDesc;
        };

        /// \brief The module being analyzed.
        llvm::Module* M = nullptr;

        /// \brief The optional function group analysis.
        GenXFunctionGroupAnalysis* FGA = nullptr;

        /// \brief Each function has an entry that describes its private memory
        /// usage information.
        llvm::DenseMap<llvm::Function*, FunctionAllocaInfo*> InfoMap;

        FunctionAllocaInfo* getFuncAllocaInfo(llvm::Function* F) const;

        FunctionAllocaInfo* getOrCreateFuncAllocaInfo(llvm::Function* F);

        /// \brief Analyze the module and fill function alloca info map.
        void analyze();

        /// \brief When function group analysis is available, visit group by group.
        void analyze(IGC::FunctionGroup* FG);

        /// \brief Analyze individual functions.
        void analyze(llvm::Function* F, unsigned& gOffset, alignment_t& gAlignment);
    };

    void initializeModuleAllocaAnalysisPass(llvm::PassRegistry&);

}
