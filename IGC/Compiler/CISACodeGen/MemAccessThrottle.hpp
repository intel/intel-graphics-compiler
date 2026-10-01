/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class FunctionPass;
}

namespace IGC {
class CodeGenContext;

// Opt-in only: the -mem-access-throttle internal option requests it, the
// MemAccessThrottle regkey overrides the option either way.
bool doMemAccessThrottle(const CodeGenContext *CGC);

llvm::FunctionPass *createMemAccessThrottlePass();
} // namespace IGC
