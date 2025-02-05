/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/UndefinedReferences/UndefinedReferencesPass.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"

#include "Compiler/Optimizer/OpenCLPasses/BufferBoundsChecking/BufferBoundsCheckingPatcher.hpp"

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

static void ReportUndefinedReference(CodeGenContext *CGC, StringRef name, Value *ctx)
{
    std::string message;
    message += "undefined reference to `" + name.str() + "'";
    CGC->EmitError(message.c_str(), ctx);
}

///////////////////////////////////////////////////////////////////////////////
//
// ExistUndefinedReferencesInModule()
//
// LLVM's linker only does the job of tying together references across modules;
// for LLVM since a declaration in LLVM IR is perfectly valid, it does not make
// sense to check for any remaining undefined references.  Call this function
// after linking to determine this.  A true return value indicates there are
// undefined references, the errors will be reported to CodeGenContext as they
// are detected.
//
static bool ExistUndefinedReferencesInModule(Module& module, CodeGenContext *CGC)
{
    bool foundUndef = false;

    Module::global_iterator GVarIter = module.global_begin();
    for (; GVarIter != module.global_end();)
    {
        GlobalVariable* pGVar = &(*GVarIter);

        // Increment the iterator before attempting to remove a global variable
        GVarIter++;

        if ((pGVar->hasAtLeastLocalUnnamedAddr() == false || pGVar->hasNUsesOrMore(1)) &&
            (pGVar->hasExternalLinkage() || pGVar->hasCommonLinkage()))
        {
            continue;
        }

        if (pGVar->isDeclaration() && pGVar->hasNUsesOrMore(1))
        {
            ReportUndefinedReference(CGC, pGVar->getName(), pGVar);
            foundUndef = true;
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
                !funcName.startswith("__builtin_bf16") &&
                !funcName.startswith("__igcbuiltin_") &&
                !funcName.startswith("__translate_sampler_initializer") &&
                !funcName.startswith("_Z20__spirv_SampledImage") &&
                !funcName.startswith("_Z21__spirv_VmeImageINTEL") &&
                funcName != BufferBoundsCheckingPatcher::BUFFER_SIZE_PLACEHOLDER_FUNCTION_NAME &&
                !F.hasFnAttribute("referenced-indirectly"))
            {
                ReportUndefinedReference(CGC, funcName, &F);
                foundUndef = true;
            }
        }
    }
    return foundUndef;
}

bool UndefinedReferencesPass::runOnModule(Module& M)
{
    // At this point all references should have been linked to definitions, any
    // undefined references should generate errors.
    CodeGenContext *CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ExistUndefinedReferencesInModule(M, CGC);
    return false;
}
