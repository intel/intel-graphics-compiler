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
#include <llvm/Support/Regex.h>
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
IGC_INITIALIZE_PASS_BEGIN(UndefinedReferencesPassLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(UndefinedReferencesPassLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char UndefinedReferencesPassLPM::ID = 0;

UndefinedReferencesPassLPM::UndefinedReferencesPassLPM() : ModulePass(ID) {
  initializeUndefinedReferencesPassLPMPass(*PassRegistry::getPassRegistry());
}

static void ReportUndefinedReference(CodeGenContext *CGC, StringRef name, Value *ctx) {
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
static bool ExistUndefinedReferencesInModule(Module &module, CodeGenContext *CGC) {
  bool foundUndef = false;

  Module::global_iterator GVarIter = module.global_begin();
  for (; GVarIter != module.global_end();) {
    GlobalVariable *pGVar = &(*GVarIter);

    // Increment the iterator before attempting to remove a global variable
    GVarIter++;

    if ((pGVar->hasAtLeastLocalUnnamedAddr() == false || pGVar->hasNUsesOrMore(1)) &&
        (pGVar->hasExternalLinkage() || pGVar->hasCommonLinkage())) {
      continue;
    }

    if (pGVar->isDeclaration() && pGVar->hasNUsesOrMore(1)) {
      ReportUndefinedReference(CGC, pGVar->getName(), pGVar);
      foundUndef = true;
    }

    if (!pGVar->isDeclaration() && pGVar->use_empty()) {
      // Remove the declaration
      pGVar->eraseFromParent();
    }
  }

  for (auto &F : module) {
    if (F.isDeclaration() && !F.isIntrinsic() && !GenISAIntrinsic::isIntrinsic(&F) && F.hasNUsesOrMore(1)) {
      StringRef funcName = F.getName();
      if (!IGCLLVM::starts_with(funcName, "__builtin_IB") && funcName != "printf" &&
          !Regex("^_Z[0-9]+__builtin_bf16").match(funcName) && !IGCLLVM::starts_with(funcName, "__igcbuiltin_") &&
          !IGCLLVM::starts_with(funcName, "__translate_sampler_initializer") &&
          !IGCLLVM::starts_with(funcName, "_Z20__spirv_SampledImage") &&
          !IGCLLVM::starts_with(funcName, "_Z21__spirv_VmeImageINTEL") &&
          funcName != BufferBoundsCheckingPatcher::BUFFER_SIZE_PLACEHOLDER_FUNCTION_NAME &&
          !F.hasFnAttribute("referenced-indirectly")) {
        ReportUndefinedReference(CGC, funcName, &F);
        foundUndef = true;
      }
    }
  }
  return foundUndef;
}

bool UndefinedReferencesPass::run(Module &M, CodeGenContext *pCtx) {
  // At this point all references should have been linked to definitions, any
  // undefined references should generate errors.
  ExistUndefinedReferencesInModule(M, pCtx);
  return false;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses UndefinedReferencesPassNPM::run(Module &M, ModuleAnalysisManager &AM) {
  bool changed = UndefinedReferencesPass().run(M, AM.getResult<CodeGenContextAnalysis>(M).Ctx);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
