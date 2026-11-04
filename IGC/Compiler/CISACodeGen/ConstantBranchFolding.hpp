/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
enum class ConstantBranchFoldingOpt : unsigned {
#define CONSTANT_BRANCH_FOLDING_FLAG(Name, Val, Description) Name = Val,
#include "igc_regkeys_enums_defs.h"
  CONSTANT_BRANCH_FOLDING_MASK
#undef CONSTANT_BRANCH_FOLDING_FLAG
#undef CONSTANT_BRANCH_FOLDING_MASK
};

/// Turns terminators on constant conditions (`br i1 true/false`, a switch on a constant) into unconditional branches
/// and deletes the blocks that become unreachable. Can optionally merge blocks into predecessors if wanted.
llvm::FunctionPass *createConstantBranchFoldingPass();
} // namespace IGC
