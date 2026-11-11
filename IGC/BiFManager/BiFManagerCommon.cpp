/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

#include "BiFManagerCommon.hpp"
#include <Probe/Assertion.h>
#include <cstdint>
#include <map>

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace IGC::BiFManager;

BiFManagerCommon::BiFManagerCommon(LLVMContext &Context) : Context(Context) {}

BiFManagerCommon::~BiFManagerCommon() {}

// Stdlib-independent 64-bit FNV-1a. std::hash<std::string> is implementation-
// defined and differs between libstdc++ (host BiFManager-bin) and libc++ (cross
// libigc.so runtime): the host tool bakes case <hash>ULL labels into OCLBiFImpl.h
// while the runtime recomputes a different value, so every builtin lookup missed
// and no BiF bodies were linked (all __spirv_* undefined). A fixed hash makes the
// generated case labels and the runtime lookups agree regardless of stdlib.
size_t BiFManagerCommon::getHash(const std::string &FlagName) {
  uint64_t h = 1469598103934665603ULL;
  for (unsigned char c : FlagName) {
    h ^= static_cast<uint64_t>(c);
    h *= 1099511628211ULL;
  }
  return static_cast<size_t>(h);
}

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

