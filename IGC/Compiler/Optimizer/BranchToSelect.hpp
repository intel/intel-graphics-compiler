/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class FunctionPass;
}

namespace IGC {

// BranchToSelect linearizes small, memory-free, acyclic branch regions into
// straight-line code: cheap and known speculatable instructions are hoisted into
// the predecessor, and the merge-block PHIs are turned into SELECTs. Only
// branches with a divergent condition are considered.
//
// The patterns recognized (triangle, diamond, shared landing pad), the
// speculation allow-list and the register-pressure profitability test are
// documented in BranchToSelect.cpp.
llvm::FunctionPass *createBranchToSelectPass();

} // namespace IGC
