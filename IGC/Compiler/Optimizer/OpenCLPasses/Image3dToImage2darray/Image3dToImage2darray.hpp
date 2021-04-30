/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instruction.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief Mark 3d images as 2d image arrays if possible.
    class Image3dToImage2darray : public llvm::FunctionPass, public llvm::InstVisitor<Image3dToImage2darray>
    {
    public:
        /// @brief  Pass identification.
        static char ID;

        Image3dToImage2darray();

        ~Image3dToImage2darray() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "Image3dToImage2darray";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        void visitCallInst(llvm::CallInst& CI);

    private:
        bool m_Changed;
        IGCMD::MetaDataUtils* m_MetadataUtils;
        IGC::ModuleMetaData* m_modMD;
        static bool createImageAnnotations(
            llvm::GenIntrinsicInst* pCall,
            unsigned imageIdx,
            const IGCMD::MetaDataUtils* pMdUtils,
            const IGC::ModuleMetaData* m_modMD,
            const llvm::Value* pCoord);
    };

} // namespace IGC
