/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef _CISA_EVALUATEFEEZE_HPP_
#define _CISA_EVALUATEFEEZE_HPP_

// This pass removes LLVM 10+ freeze instructions by either propagating
// the valid value forward or else producing a deterministic value for
// undef values.

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
    llvm::FunctionPass* createEvaluateFreezePass();
    void initializeEvaluateFreezePass(llvm::PassRegistry&);
} // IGC::

#endif // _CISA_EVALUATEFEEZE_HPP_
