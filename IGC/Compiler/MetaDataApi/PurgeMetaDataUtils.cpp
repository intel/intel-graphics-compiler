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

#include "Compiler/MetaDataApi/PurgeMetaDataUtils.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMUtils.h"


using namespace llvm;
using namespace IGC;
// Register pass to igc-opt
#define PASS_FLAG "igc-purgeMetaDataUtils-import"
#define PASS_DESCRIPTION "PurgeMetaDataUtilsImport"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PurgeMetaDataUtils, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PurgeMetaDataUtils, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PurgeMetaDataUtils::ID = 0;

PurgeMetaDataUtils::PurgeMetaDataUtils() : ModulePass(ID)
{
    initializePurgeMetaDataUtilsPass(*PassRegistry::getPassRegistry());
}

// Remove metadata entries in metadata utils for inlined or dead functions.
// TODO: get rid of this step if we do not keep any non-kernel function in the
// metadata util object.
bool PurgeMetaDataUtils::runOnModule(Module& M)
{
    m_pModule = &M;
    m_changed = false;

    CodeGenContext* pContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    IGCMD::MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    auto shouldRemoveFunction = [=](llvm::Module* M, void* ptr)
    {
        llvm::Function* F = nullptr;

        // Function may have been removed already.
        for (auto& G : M->getFunctionList())
        {
            if (&G == ptr)
            {
                F = &G;
                break;
            }
        }

        // Already deleted, or not used non-kernels.
        if (F == nullptr)
        {
            return true;
        }

        if (F->use_empty() && !isEntryFunc(pMdUtils, F))
        {
            if (F->hasFnAttribute("IndirectlyCalled") &&
                GlobalValue::isExternalLinkage(F->getLinkage()))
            {
                // Do not delete externally linked functions, even if there are no uses in the current module.
                // However if it's only used internally, and we somehow resolve the indirect call, we can remove it.
                return false;
            }

            if (isPixelShaderPhaseFunction(F)) {
                // In some cases pixel shader codegen splits the compilation
                // to phases, removing entry functions from metadata, but assumes they
                // will be available in the next pass. Leave such functions in the module.
                return false;
            }

            F->eraseFromParent();
            return true;
        }

        // Kernels or used non-kernels.
        return false;
    };

    // Collect all functions for which metadata will be removed.
    SmallSet<llvm::Function*, 8> ToBeDeleted;

    auto checkFuncRange = [&](auto beginIt, auto endIt) {
        for (auto it = beginIt, e = endIt; it != e; ++it)
        {
            if (shouldRemoveFunction(pContext->getModule(), it->first))
            {
                ToBeDeleted.insert(it->first);
            }
        }
    };

    auto& FuncMD = pContext->getModuleMetaData()->FuncMD;
    checkFuncRange(pMdUtils->begin_FunctionsInfo(), pMdUtils->end_FunctionsInfo());
    checkFuncRange(FuncMD.begin(), FuncMD.end());

    // Remove them.
    for (auto F : ToBeDeleted)
    {
        auto Iter = pMdUtils->findFunctionsInfoItem(F);
        if (Iter != pMdUtils->end_FunctionsInfo())
        {
            pMdUtils->eraseFunctionsInfoItem(Iter);
        }

        if (FuncMD.find(F) != FuncMD.end())
        {
            FuncMD.erase(F);
        }
    }

    // Update when there is any change.
    if (!ToBeDeleted.empty())
    {
        pMdUtils->save(*pContext->getLLVMContext());
        m_changed = true;
    }

    return m_changed;
}