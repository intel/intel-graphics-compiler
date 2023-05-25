/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass creates annotations for OpenCL program-scope structures.
    //          Currently this is program-scope constants, but for OpenCL 2.0, it should
    //          also support program-scope globals.
    class ProgramScopeConstantAnalysis : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ProgramScopeConstantAnalysis();

        /// @brief  Destructor
        ~ProgramScopeConstantAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ProgramScopeConstantAnalysisPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        ///         Runs on all GlobalVariables in this module, finds the constants, and
        ///         generates annotations for them.
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

    protected:
        typedef std::vector<unsigned char> DataVector;
        typedef llvm::MapVector<llvm::GlobalVariable*, int> BufferOffsetMap;

        struct PointerOffsetInfo
        {
            unsigned AddressSpaceWherePointerResides;
            uint64_t PointerOffsetFromBufferBase;
            unsigned AddressSpacePointedTo;

            PointerOffsetInfo(
                unsigned AddressSpaceWherePointerResides,
                uint64_t PointerOffsetFromBufferBase,
                unsigned AddressSpacePointedTo) :
                AddressSpaceWherePointerResides(AddressSpaceWherePointerResides),
                PointerOffsetFromBufferBase(PointerOffsetFromBufferBase),
                AddressSpacePointedTo(AddressSpacePointedTo) {}
        };

        typedef std::list<PointerOffsetInfo> PointerOffsetInfoList;

        /// @brief  Add data from the inline constant into the buffer.
        /// @param  initializer           The initializer of the constant being added.
        /// @param  inlineConstantBuffer  The buffer the data is being added to.
        void addData(
            llvm::Constant* initializer,
            DataVector& inlineConstantBuffer,
            PointerOffsetInfoList& pointerOffsetInfo,
            BufferOffsetMap& inlineProgramScopeOffsets,
            unsigned addressSpace,
            bool forceAlignmentOne=false);

        /// @brief  Align the buffer according to the required alignment
        /// @param  buffer     The buffer to align.
        /// @param  alignment  Required alignment in bytes.
        void alignBuffer(DataVector& buffer, alignment_t alignment);

        const llvm::DataLayout* m_DL;
        ModuleMetaData* m_pModuleMd;
    };

} // namespace IGC
