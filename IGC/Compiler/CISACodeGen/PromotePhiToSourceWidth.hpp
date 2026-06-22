/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class FunctionPass;
}

namespace IGC {
// Sinks a width-narrowing cast (fptrunc/trunc, optionally preceded by a constant
// lshr that selects a packed lane and/or followed by a same-width bitcast that
// reinterprets the result, e.g. an fp<->int repack) out of a constant-guarded
// merge PHI, re-typing the PHI to the cast's source width so it matches the loop
// accumulators:
//
//   %p = phi iN [ bitcast(fptrunc(float %a) to half) to iN, L ], [ 0, G ]
//     => %pw = phi float [ %a, L ], [ 0.0, G ]
//        %p  = bitcast(fptrunc(%pw) to half) to iN     ; sunk into the merge block
//
llvm::FunctionPass *createPromotePhiToSourceWidthPass();
} // namespace IGC
