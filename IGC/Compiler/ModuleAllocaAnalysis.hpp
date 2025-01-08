/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include "common/Types.hpp"
#include "iStdLib/utility.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include <optional>
#include <list>

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

        /// \brief Return the stride of alloca instruction in private memory buffer.
        //  This function should not be called when AI is variable length alloca
        unsigned getBufferStride(llvm::AllocaInst* AI) const;

        /// \brief Return the offset of alloca instruction in private memory buffer.
        //  This function should not be called when AI is variable length alloca
        unsigned getBufferOffset(llvm::AllocaInst* AI) const;

        /// \brief Return if the allocation is uniform
        unsigned isUniform(llvm::AllocaInst* AI) const;

        llvm::Value* getPerThreadOffset(
            IGCLLVM::IRBuilder<>& IRB,
            llvm::AllocaInst* AI,
            llvm::Value* simdSize,
            llvm::Value* threadId,
            bool return64bitOffset = false) const;

        llvm::Value* getOffset(
            IGCLLVM::IRBuilder<>& IRB,
            llvm::AllocaInst* AI,
            llvm::Value* simdSize,
            llvm::Value* simdLaneId,
            std::optional<uint32_t> perLaneStride = std::nullopt) const;

        /// \brief Return all alloca instructions of a given function.
        llvm::SmallVector<llvm::AllocaInst*, 8>& getAllocaInsts(llvm::Function* F) const;

        /// \brief Return the total private memory size per WI of a given function.
        unsigned getTotalPrivateMemPerWI(llvm::Function* F) const;

        /// \brief Return maximum alignment among all allocas of a given function.
        unsigned getPrivateMemAlignment(llvm::Function* F) const;

    private:

        /// \brief Return the total private memory size per WI of a given function.
        unsigned getMinSimdSize(llvm::Function* F) const;

        struct PrivateMemoryDescription {
            /// \brief The total size for uniform allocas
            unsigned TotalSizeForUniformAllocas = 0;

            /// \brief The total stride for non-uniform allocas
            unsigned TotalStrideForNonUniformAllocas = 0;

            /// \brief Maximum alignment among all non-uniform allocas.
            unsigned MaximumAlignmentForNonUniformAllocas = 1;

            /// \brief Maximum alignment among all non-uniform allocas.
            unsigned MaximumAlignmentForUniformAllocas = 1;

            unsigned GetMaxAlignment() const
            {
                return std::max(
                    MaximumAlignmentForNonUniformAllocas,
                    MaximumAlignmentForUniformAllocas);
            }

            unsigned GetOffsetForUniformAllocas() const
            {
                return 0;
            }

            unsigned GetOffsetForNonUniformAllocas() const
            {
                return iSTD::Align(
                    TotalSizeForUniformAllocas,
                    GetMaxAlignment());
            }

            unsigned GetStrideForNonUniformAllocas() const
            {
                return iSTD::Align(
                    TotalStrideForNonUniformAllocas,
                    MaximumAlignmentForNonUniformAllocas);
            }

            uint64_t GetTotalSize(unsigned simdSize) const
            {
                unsigned size = GetOffsetForNonUniformAllocas() +
                    simdSize * GetStrideForNonUniformAllocas();
                return iSTD::Align(size, GetMaxAlignment());
            }
        };

        /// \brief All allocas are stored in the following way:
        /// +---------------------------------------+
        /// | 1. uniform alloca                     |
        /// +---------------------------------------+
        /// | 2. uniform alloca                     |
        /// +---------------------------------------+
        /// | ...                                   |
        /// +---------------------------------------+
        /// | n-th uniform alloca                   |
        /// +---------------------------------------+
        /// | 1. non-uniform alloca (lane 0)        |
        /// +---------------------------------------+
        /// | 1. non-uniform alloca (lane 1)        |
        /// +---------------------------------------+
        /// | ...                                   |
        /// +---------------------------------------+
        /// | 1. non-uniform alloca (lane SIMD-1)   |
        /// +---------------------------------------+
        /// | 2. non-uniform alloca (lane 0)        |
        /// +---------------------------------------+
        /// | 2. non-uniform alloca (lane 1)        |
        /// +---------------------------------------+
        /// | ...                                   |
        /// +---------------------------------------+
        /// | 2. non-uniform alloca (lane SIMD-1)   |
        /// +---------------------------------------+
        /// | ...                                   |
        /// +---------------------------------------+
        /// | m-th non-uniform alloca (lane 0)      |
        /// +---------------------------------------+
        /// | m-th non-uniform alloca (lane 1)      |
        /// +---------------------------------------+
        /// | ...                                   |
        /// +---------------------------------------+
        /// | m-th non-uniform alloca (lane SIMD-1) |
        /// +---------------------------------------+
        /// Note that the stride and offset for non-uniform
        /// allocas must be multiplied by the SIMD size.
        /// Note that transposition changes the layout for
        /// a cluster of the uniform alloca:
        /// +-----------------------------------------------------+
        /// | 1. chunk of m-th non-uniform alloca (lane 0)        |
        /// +-----------------------------------------------------+
        /// | 1. chunk of m-th non-uniform alloca (lane 1)        |
        /// +-----------------------------------------------------+
        /// | ...                                                 |
        /// +-----------------------------------------------------+
        /// | 1. chunk of m-th non-uniform alloca (lane SIMD-1)   |
        /// +-----------------------------------------------------+
        /// | 2. chunk of m-th non-uniform alloca (lane 0)        |
        /// +-----------------------------------------------------+
        /// | 2. chunk of m-th non-uniform alloca (lane 1)        |
        /// +-----------------------------------------------------+
        /// | ...                                                 |
        /// +-----------------------------------------------------+
        /// | 2. chunk of m-th non-uniform alloca (lane SIMD-1)   |
        /// +-----------------------------------------------------+
        /// | ...                                                 |
        /// +-----------------------------------------------------+
        /// | o-th chunk of m-th non-uniform alloca (lane 0)      |
        /// +-----------------------------------------------------+
        /// | o-th chunk of m-th non-uniform alloca (lane 1)      |
        /// +-----------------------------------------------------+
        /// | ...                                                 |
        /// +-----------------------------------------------------+
        /// | o-th chunk of m-th non-uniform alloca (lane SIMD-1) |
        /// +-----------------------------------------------------+
        struct FunctionAllocaInfo {

            static unsigned getAlignment(llvm::AllocaInst* AI, const llvm::DataLayout* DL);

            static unsigned getSize(llvm::AllocaInst* AI, const llvm::DataLayout* DL);

            void AssignAlloca(llvm::AllocaInst* AI, const llvm::DataLayout* DL, bool SupportsUniformPrivateMemory);

            PrivateMemoryDescription* MemoryDescription;

            /// \brief Alloca instructions for a function.
            llvm::SmallVector<llvm::AllocaInst*, 8> Allocas;

            /// \brief Alloca instruction, its offset in buffer.
            llvm::DenseMap<llvm::AllocaInst*, unsigned> UniformAllocaDesc;

            /// \brief Alloca instruction, its offset and stride in buffer.
            llvm::DenseMap<llvm::AllocaInst*, std::pair<unsigned, unsigned>> NonUniformAllocaDesc;
        };

        /// \brief The module being analyzed.
        llvm::Module* M = nullptr;

        /// \brief The optional function group analysis.
        GenXFunctionGroupAnalysis* FGA = nullptr;

        /// \brief Each function has an entry that describes its private memory
        /// usage information.
        llvm::DenseMap<llvm::Function*, FunctionAllocaInfo*> InfoMap;

        /// \brief Each function has an entry that describes its private memory
        /// usage information.
        std::list<PrivateMemoryDescription> MemoryDescriptions;

        /// \brief Informs if uniform private memory space can be used
        bool SupportsUniformPrivateMemory = false;

        FunctionAllocaInfo* getFuncAllocaInfo(llvm::Function* F) const;

        FunctionAllocaInfo* getOrCreateFuncAllocaInfo(llvm::Function* F);

        /// \brief Analyze the module and fill function alloca info map.
        void analyze();

        /// \brief When function group analysis is available, visit group by group.
        void analyze(IGC::FunctionGroup* FG);

        /// \brief Analyze individual functions.
        void analyze(llvm::Function* F, PrivateMemoryDescription& memoryDescription);
    };

    void initializeModuleAllocaAnalysisPass(llvm::PassRegistry&);

}
