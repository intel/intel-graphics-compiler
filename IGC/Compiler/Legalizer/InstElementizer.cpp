/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// vim:ts=2:sw=2:et:

#define DEBUG_TYPE "type-legalizer"
#include "TypeLegalizer.h"
#include "InstElementizer.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC::Legalizer;

bool InstElementizer::elementize(Instruction* I) {
    IGC_ASSERT_EXIT_MESSAGE(0, "NOT IMPLEMENTED YET!");
    return false;
}
