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
