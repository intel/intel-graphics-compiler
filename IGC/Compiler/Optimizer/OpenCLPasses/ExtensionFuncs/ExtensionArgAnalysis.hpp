/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
