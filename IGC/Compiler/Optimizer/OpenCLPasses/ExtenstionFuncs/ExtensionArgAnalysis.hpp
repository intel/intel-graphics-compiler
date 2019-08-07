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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"


namespace llvm
{
    class Argument;
}

namespace IGC
{
    /// @brief  ExtensionArgAnalysis pass used for analyzing if VME extension functions arguments.
    ///         This information needed by ResourceAllocator and helps create the right VME/media patch tokens

    class ExtensionArgAnalysis : public llvm::FunctionPass, public llvm::InstVisitor<ExtensionArgAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ExtensionArgAnalysis();

        /// @brief  Destructor
        ~ExtensionArgAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ExtensionArgAnalysis";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        /// @brief  Function entry point.
        ///         Checks if this is a VME function and analyzes its function arguments (images, samplers)
        /// @param  F The destination function.
        bool runOnFunction(llvm::Function& F) override;

        /// @brief  Returns true if the given argument is a VME image or sampler argument
        /// @param  arg A function argument
        /// @return true if the given argument is a VME image or sampler argument, false otherwise
        bool isMediaArg(const llvm::Argument* arg)
        {
            return (m_MediaArgs.count(const_cast<llvm::Argument*>(arg)) > 0) ? true : false;
        }
        bool isMediaSamplerArg(const llvm::Argument* arg)
        {
            return (m_MediaSamplerArgs.count(const_cast<llvm::Argument*>(arg)) > 0) ? true : false;
        }
        bool isMediaBlockArg(const llvm::Argument* arg)
        {
            return (m_MediaBlockArgs.count(const_cast<llvm::Argument*>(arg)) > 0) ? true : false;
        }
        bool isVaArg(const llvm::Argument* arg)
        {
            return (m_vaArgs.count(const_cast<llvm::Argument*>(arg)) > 0) ? true : false;
        }

        ResourceExtensionTypeEnum GetExtensionSamplerType()
        {
            return m_extensionType;
        }

        void visitCallInst(llvm::CallInst& CI);
    private:

        llvm::DenseMap<llvm::Argument*, ResourceExtensionTypeEnum> m_ExtensionMap;


        /// @brief  Contains the VME image and sampler arguments of the function
        llvm::SmallPtrSet<llvm::Argument*, 3> m_MediaArgs;
        llvm::SmallPtrSet<llvm::Argument*, 3> m_MediaSamplerArgs;
        llvm::SmallPtrSet<llvm::Argument*, 3> m_MediaBlockArgs;
        llvm::SmallPtrSet<llvm::Argument*, 2> m_vaArgs;
        ResourceExtensionTypeEnum m_extensionType;
    };

} // namespace IGC
