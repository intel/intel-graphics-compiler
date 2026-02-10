/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ConvertUserSemanticDecoratorOnFunctions.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Mangler.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/Type.h"
using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-convert-user-semantic-decorator-on-functions"
#define PASS_DESCRIPTION "Convert user semantic decorator on functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ConvertUserSemanticDecoratorOnFunctions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ConvertUserSemanticDecoratorOnFunctions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)

char ConvertUserSemanticDecoratorOnFunctions::ID = 0;

ConvertUserSemanticDecoratorOnFunctions::ConvertUserSemanticDecoratorOnFunctions() : ModulePass(ID) {
  initializeConvertUserSemanticDecoratorOnFunctionsPass(*PassRegistry::getPassRegistry());
}

// Some of the metadata may disappear when linking LLVM modules; attributes are much more permament.
static void convertAnnotationsToAttributes(llvm::Function *function, const std::vector<std::string> &annotations) {
  for (const auto &annotation : annotations) {
    if (annotation == "igc-force-stackcall") {
      function->addFnAttr("igc-force-stackcall");
    } else if (annotation == "sycl-unmasked") {
      function->addFnAttr("sycl-unmasked");
    } else if (annotation.rfind("num-thread-per-eu", 0) == 0) {
      std::string numThreadPerEU = annotation;
      numThreadPerEU.erase(0, sizeof("num-thread-per-eu") - 1);

      // Remove whitespaces - if they are present
      numThreadPerEU.erase(std::remove_if(numThreadPerEU.begin(), numThreadPerEU.end(), ::isspace),
                           numThreadPerEU.end());

      function->addFnAttr("num-thread-per-eu", numThreadPerEU == "auto" ? "0" : std::move(numThreadPerEU));
    }
  }
}

bool ConvertUserSemanticDecoratorOnFunctions::runOnModule(Module &M) {
  auto MD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  auto annotations_gv = M.getGlobalVariable("llvm.global.annotations");
  if (!annotations_gv) {
    return false;
  }

  //
  // GlobalVariable("llvm.global.annotations"):
  //    ConstantArray:
  //       ConstantStruct:
  //          BitCastOperator:
  //             Function = [ANNOTATED_FUNCTION]
  //          GetElementPtr:
  //             GlobalVariable = [ANNOTATION]
  //       ConstantStruct:
  //       ...
  //

  auto annotations_array = cast<ConstantArray>(annotations_gv->getOperand(0));
  for (const auto &op : annotations_array->operands()) {
    auto annotation_struct = cast<ConstantStruct>(op.get());

    llvm::Function *annotated_function;
    llvm::GlobalVariable *annotation_gv;

    // For opaque pointers we can call only single getOperand() on annotation_struct,
    // beacause we don't need to use e.g. bitcast instruction like for typed pointers
    if (IGCLLVM::isPointerTy(annotation_struct->getOperand(0)->getType())) {
      annotated_function = cast<Function>(annotation_struct->getOperand(0));
      annotation_gv = cast<GlobalVariable>(annotation_struct->getOperand(1));
    } else {
      annotated_function = cast<Function>(annotation_struct->getOperand(0)->getOperand(0));
      annotation_gv = cast<GlobalVariable>(annotation_struct->getOperand(1)->getOperand(0));
    }

    auto annotation = cast<ConstantDataArray>(annotation_gv->getInitializer())->getAsCString();

    auto &funcInfo = MD->FuncMD[annotated_function];
    funcInfo.UserAnnotations.emplace_back(annotation.data());
    convertAnnotationsToAttributes(annotated_function, funcInfo.UserAnnotations);
  }

  IGC::serialize(*MD, &M);
  return true;
}
