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

#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/MDFrameWork.h"

namespace IGC
{
    // This pass provides access to the metadata api.
    //
    // To use this from within another pass:
    //  1. Add MetaDataUtilsWrapper to the analysis usage of the pass:
    //      virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {AU.addRequired<MetaDataUtilsWrapper>();}
    //  2. Add MetaDataUtilsWrapper as a dependency of the pass: 
    //      IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
    //  3. Use getAnalysis on MetaDataUtilsWrapper:
    //      MetaDataUtilsWrapper &mduw = getAnalysis<MetaDataUtilsWrapper>();
    //  4. Get the MetaDataUtils:
    //      MetaDataUtils *mdUtils = mduw.getMetaDataUtils(); 
    class MetaDataUtilsWrapper : public llvm::ImmutablePass
    {
    public:
        static char ID;

        // Constructs a wrapper to an uninitialized MetaDataUtils instance.
        // To complete initialization, a module must be passed to MetaDataUtils by
        // calling getMetaDataUtils()->setModule(), or by making sure that
        // MetaDataUtilsWrapperInitializer pass runs before using the MetaDataUtils.
        //
        // MetaDataUtilsWrapper will be the owner of the MetaDataUtils, and will be 
        // responsible for freeing the allocated memory.
        MetaDataUtilsWrapper();

        // Constructs a wrapper to the given MetaDataUtils instance.
        // 
        // MetaDataUtilsWrapper will NOT be the owner of the MetaDataUtils.
        MetaDataUtilsWrapper(IGCMD::MetaDataUtils* pMdUtils, ModuleMetaData* moduleMD = nullptr);

        ~MetaDataUtilsWrapper();

        IGCMD::MetaDataUtils* getMetaDataUtils();
        ModuleMetaData* getModuleMetaData();

        virtual llvm::StringRef getPassName() const override
        {
            return "MetaData Utils Wrapper";
        }

    private:
        IGCMD::MetaDataUtils* m_pMdUtils;
        IGC::ModuleMetaData* modMD;
        bool m_isUtilsOwner;

    };

} // namespace IGC
