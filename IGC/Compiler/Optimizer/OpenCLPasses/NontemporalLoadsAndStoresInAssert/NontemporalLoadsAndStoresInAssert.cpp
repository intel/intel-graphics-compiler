/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/NontemporalLoadsAndStoresInAssert/NontemporalLoadsAndStoresInAssert.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-nontemporal-loads-and-stores-in-assert"
#define PASS_DESCRIPTION "Mark loads and stores inside assert implementation as nontemporal to avoid caching"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(NontemporalLoadsAndStoresInAssert, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(NontemporalLoadsAndStoresInAssert, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char NontemporalLoadsAndStoresInAssert::ID = 0;

static const char *ASSERT_FUNCTION_NAME = "__devicelib_assert_fail";

NontemporalLoadsAndStoresInAssert::NontemporalLoadsAndStoresInAssert()
    : ModulePass(ID) {
  initializeNontemporalLoadsAndStoresInAssertPass(
      *PassRegistry::getPassRegistry());
}

bool NontemporalLoadsAndStoresInAssert::runOnModule(Module &M) {
  bool changed = false;
  for (Function &F : M) {

    if (!F.getName().equals(ASSERT_FUNCTION_NAME))
      continue;

    for (auto I = inst_begin(F); I != inst_end(F); ++I) {
      if (isa<LoadInst>(*I) || isa<StoreInst>(*I)) {
        Constant *One = ConstantInt::get(Type::getInt32Ty(I->getContext()), 1);
        MDNode *Node =
            MDNode::get(I->getContext(), ConstantAsMetadata::get(One));
        I->setMetadata(LLVMContext::MD_nontemporal, Node);
        changed = true;
      }
    }
  }
  return changed;
}
