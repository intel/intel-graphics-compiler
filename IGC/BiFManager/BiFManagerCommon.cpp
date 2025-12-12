/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

#include "BiFManagerCommon.hpp"
#include <Probe/Assertion.h>
#include <map>

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace IGC::BiFManager;

BiFManagerCommon::BiFManagerCommon(LLVMContext &Context) : Context(Context) {}

BiFManagerCommon::~BiFManagerCommon() {}

size_t BiFManagerCommon::getHash(const std::string &FlagName) { return std::hash<std::string>{}(FlagName); }

CollectBuiltinsPass::CollectBuiltinsPass(TFunctionsVec &neededBuiltinsFunc,
                                         const std::function<bool(llvm::Function *)> &predicate)
    : neededBuiltinsFunc(neededBuiltinsFunc), predicate(predicate) {}

CollectBuiltinsPass::~CollectBuiltinsPass() {}

void CollectBuiltinsPass::visitCallInst(llvm::CallInst &callInst) {
  auto pFunc = callInst.getCalledFunction();

  if (pFunc != nullptr &&
      std::find(neededBuiltinsFunc.begin(), neededBuiltinsFunc.end(), pFunc) == neededBuiltinsFunc.end()) {
    if (predicate(pFunc)) {
      neededBuiltinsFunc.push_back(pFunc);
    }
  }
}

