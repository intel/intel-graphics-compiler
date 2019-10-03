/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
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
        }

        /// @brief  Main entry point.
        ///         Runs on all GlobalVariables in this module, finds the constants, and
        ///         generates annotations for them.
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

    protected:
        typedef std::vector<unsigned char> DataVector;
        typedef std::map<llvm::GlobalVariable*, int> BufferOffsetMap;

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
            unsigned addressSpace);

        /// @brief  Align the buffer according to the required alignment
        /// @param  buffer     The buffer to align.
        /// @param  alignment  Required alignment in bytes.
        void alignBuffer(DataVector& buffer, unsigned int alignment);

        const llvm::DataLayout* m_DL;
    };

} // namespace IGC