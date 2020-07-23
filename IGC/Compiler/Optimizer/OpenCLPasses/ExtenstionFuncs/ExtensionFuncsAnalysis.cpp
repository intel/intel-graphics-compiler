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

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtenstionFuncs/ExtensionFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-extension-funcs-analysis"
#define PASS_DESCRIPTION "Analyzes extension functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ExtensionFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ExtensionFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ExtensionFuncsAnalysis::ID = 0;

ExtensionFuncsAnalysis::ExtensionFuncsAnalysis() : ModulePass(ID)
{
    initializeExtensionFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

const StringRef ExtensionFuncsAnalysis::VME_MB_BLOCK_TYPE = "__builtin_IB_vme_mb_block_type";
const StringRef ExtensionFuncsAnalysis::VME_SUBPIXEL_MODE = "__builtin_IB_vme_subpixel_mode";
const StringRef ExtensionFuncsAnalysis::VME_SAD_ADJUST_MODE = "__builtin_IB_vme_sad_adjust_mode";
const StringRef ExtensionFuncsAnalysis::VME_SEARCH_PATH_TYPE = "__builtin_IB_vme_search_path_type";
const StringRef ExtensionFuncsAnalysis::VME_HELPER_GET_HANDLE = "__builtin_IB_vme_helper_get_handle";
const StringRef ExtensionFuncsAnalysis::VME_HELPER_GET_AS = "__builtin_IB_vme_helper_get_as";

bool ExtensionFuncsAnalysis::runOnModule(Module& M)
{
    bool changed = false;
    m_pMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    // Run on all functions defined in this module
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);
        if (pFunc->isDeclaration()) continue;
        if (runOnFunction(*pFunc))
        {
            changed = true;
        }
    }

    // Update LLVM metadata based on IGC MetadataUtils
    if (changed)
        m_pMDUtils->save(M.getContext());

    return changed;
}

bool ExtensionFuncsAnalysis::runOnFunction(Function& F)
{
    // Processing new function
    m_hasVME = false;

    // Visit the function
    visit(F);

    // Check if VME implicit information is needed based on the function analysis
    if (!m_hasVME) return false;

    // Add the implicit arguments needed by this function
    SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;

    implicitArgs.push_back(ImplicitArg::VME_MB_BLOCK_TYPE);
    implicitArgs.push_back(ImplicitArg::VME_SUBPIXEL_MODE);
    implicitArgs.push_back(ImplicitArg::VME_SAD_ADJUST_MODE);
    implicitArgs.push_back(ImplicitArg::VME_SEARCH_PATH_TYPE);

    // Create the metadata representing the VME implicit args needed by this function
    ImplicitArgs::addImplicitArgs(F, implicitArgs, m_pMDUtils);

    return true;
}

void ExtensionFuncsAnalysis::visitCallInst(CallInst& CI)
{
    // Check for VME function calls
    if (Function * F = CI.getCalledFunction())
    {
        StringRef funcName = F->getName();
        if (funcName.equals(VME_MB_BLOCK_TYPE) ||
            funcName.equals(VME_SUBPIXEL_MODE) ||
            funcName.equals(VME_SAD_ADJUST_MODE) ||
            funcName.equals(VME_SEARCH_PATH_TYPE) ||
            funcName.startswith(VME_HELPER_GET_HANDLE) ||
            funcName.startswith(VME_HELPER_GET_AS))
        {
            m_hasVME = true;
        }
    }
}
