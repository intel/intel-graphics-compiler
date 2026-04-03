/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class PassRegistry;
}

// Registers all IGC passes with the given PassRegistry.
// Used by igc_opt
void initializeAllIGCPasses(llvm::PassRegistry &Registry);
