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

#include "Compiler/Optimizer/OpenCLPasses/UndefinedReferences/UndefinedReferencesPass.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "undefined-references"
#define PASS_DESCRIPTION "Emit linker warnings to user"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(UndefinedReferencesPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(UndefinedReferencesPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char UndefinedReferencesPass::ID = 0;

UndefinedReferencesPass::UndefinedReferencesPass() : ModulePass(ID)
{
    initializeUndefinedReferencesPassPass(*PassRegistry::getPassRegistry());
}

///////////////////////////////////////////////////////////////////////////////
//
// ExistUndefinedReferencesInModule()
//
// LLVM's linker only does the job of tying together references across modules;
// for LLVM since a declaration in LLVM IR is perfectly valid, it does not make
// sense to check for any remaining undefined references.  Call this function
// after linking to determine this.  A true return value indicates there are
// undefined references and errorMessage will be appended with the appropriate
// information.
//
static bool ExistUndefinedReferencesInModule(Module& module, std::string& errorMessage)
{
    raw_string_ostream strStream(errorMessage);
    bool foundUndef = false;

    std::string msg = "undefined reference to `";

    Module::global_iterator GVarIter = module.global_begin();
    for (; GVarIter != module.global_end();)
    {
        GlobalVariable* pGVar = &(*GVarIter);
        if (pGVar->isDeclaration() && pGVar->hasNUsesOrMore(1))
        {
            strStream << msg << GVarIter->getName().str() << "'\n";
            foundUndef = true;
        }

        // Increment the iterator before attempting to remove a global variable
        GVarIter++;

        if (IGC_IS_FLAG_ENABLED(EnableGlobalRelocation) &&
            (pGVar->hasAtLeastLocalUnnamedAddr() == false) &&
            (pGVar->hasExternalLinkage() || pGVar->hasCommonLinkage()))
        {
            continue;
        }

        if (!pGVar->isDeclaration() && pGVar->use_empty())
        {
            // Remove the declaration
            pGVar->eraseFromParent();
        }
    }

    for (auto& F : module)
    {
        if (F.isDeclaration() && !F.isIntrinsic() && !GenISAIntrinsic::isIntrinsic(&F) && F.hasNUsesOrMore(1))
        {
            StringRef funcName = F.getName();
            if (!funcName.startswith("__builtin_IB") && funcName != "printf" &&
                !funcName.startswith("__igcbuiltin_") &&
                !funcName.startswith("__translate_sampler_initializer"))
            {
                if (F.hasFnAttribute("referenced-indirectly"))
                {
                    F.addFnAttr("visaStackCall");
                    F.addFnAttr("IndirectlyCalled");
                    F.setLinkage(GlobalValue::ExternalLinkage);
                    continue;
                }
                strStream << msg << funcName << "()'\n";
                foundUndef = true;
            }
        }
    }

    strStream.flush();

    return foundUndef;
}

bool UndefinedReferencesPass::runOnModule(Module& M)
{
    // At this point all references should have been linked to definitions, any
    // undefined references should generate errors.
    std::string errorMessage;
    if (ExistUndefinedReferencesInModule(M, errorMessage))
    {
        if (!errorMessage.empty())
        {
            getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError(errorMessage.c_str());
        }
    }

    return false;
}
