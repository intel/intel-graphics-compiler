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

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/IRBuilder.h"

#include <llvm/Pass.h>
#include <llvm/IR/Constants.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

class GlobalToLocal : public llvm::FunctionPass
{
public:
    GlobalToLocal() : FunctionPass(ID)
    {

    }
    static char ID;

    bool runOnFunction(llvm::Function& F) override;

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
    }

    virtual llvm::StringRef getPassName() const override
    {
        return "GlobalToLocal";
    }
};

char GlobalToLocal::ID = 0;

Pass* createGlobalToLocalPass()
{
    return new GlobalToLocal();
}

bool GlobalToLocal::runOnFunction(llvm::Function& F)
{
    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }
    Instruction* topFirstBB = &(*F.getEntryBlock().begin());

    IGCLLVM::IRBuilder<> builder(topFirstBB);

    Module::GlobalListType& globalList = F.getParent()->getGlobalList();
    for (auto GI = globalList.begin(), GE = globalList.end(); GI != GE; ++GI)
    {
        GlobalVariable* global = &(*GI);
        if (!global->use_empty())
        {
            // removes constant expressions which may be added by one of the LLVM optimization passes
            // If these constant expressions are not removed it causes issues when replaceAllUsesWith() 
            // is called on the Global
            global->removeDeadConstantUsers();

            builder.SetInsertPoint(topFirstBB);
            Instruction* alloc = builder.CreateAlloca(global->getType()->getPointerElementType());
            if (global->hasInitializer())
            {
                new StoreInst(global->getInitializer(), alloc, topFirstBB);
            }
            global->replaceAllUsesWith(alloc);
            // Leave the global dead, some globals are used to pass data from front end to codegen
            //global->eraseFromParent();
        }
    }
    return false;
}
