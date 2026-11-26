/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2026 Intel Corporation

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

uint64_t BiFManagerCommon::getHash(StringRef Name) {
  uint64_t Hash = 0xCBF29CE484222325ULL;
  for (unsigned char C : Name) {
    Hash ^= C;
    Hash *= 0x100000001B3ULL;
  }
  return Hash;
}

