/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/MetaDataApi/PurgeMetaDataUtils.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"


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
    return purgeMetaDataUtils(M, &getAnalysis<MetaDataUtilsWrapper>());
}

bool IGC::purgeMetaDataUtils(Module& M, MetaDataUtilsWrapper* MDUW) {
    IGCMD::MetaDataUtils* MDUtils = MDUW->getMetaDataUtils();
    auto shouldRemoveFunction = [=](llvm::Module& M, void* ptr)
    {
        llvm::Function* F = nullptr;

        // Function may have been removed already.
        for (auto& G : M.getFunctionList())
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

        if (F->use_empty() && !isEntryFunc(MDUtils, F))
        {
            if (F->hasFnAttribute("referenced-indirectly") &&
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

            if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && F->hasFnAttribute("sycl-unmasked"))
                return false;

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
            if (shouldRemoveFunction(M, it->first))
            {
                ToBeDeleted.insert(it->first);
            }
        }
    };

    auto& FuncMD = MDUW->getModuleMetaData()->FuncMD;
    checkFuncRange(MDUtils->begin_FunctionsInfo(), MDUtils->end_FunctionsInfo());
    checkFuncRange(FuncMD.begin(), FuncMD.end());

    // Remove them.
    for (auto F : ToBeDeleted)
    {
        auto Iter = MDUtils->findFunctionsInfoItem(F);
        if (Iter != MDUtils->end_FunctionsInfo())
        {
            MDUtils->eraseFunctionsInfoItem(Iter);
        }

        if (FuncMD.find(F) != FuncMD.end())
        {
            FuncMD.erase(F);
        }
    }

    // Update when there is any change.
    if (!ToBeDeleted.empty())
    {
        MDUtils->save(M.getContext());
    }

    return !ToBeDeleted.empty();
}
