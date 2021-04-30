/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "type-legalizer"
#include "TypeLegalizer.h"
#include "InstSoftener.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC::Legalizer;

bool InstSoftener::soften(Instruction* I) {
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}
