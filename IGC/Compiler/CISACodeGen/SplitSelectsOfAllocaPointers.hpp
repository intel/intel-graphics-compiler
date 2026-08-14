/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class ModulePass;
}

namespace IGC {
// Pass run immediately before PrivateMemoryResolution.
//
// SOALayoutChecker rejects an alloca whose pointer flows into a SELECT because
// TransposePrivMem cannot lower a load/store whose address comes from a runtime
// SELECT of two heterogeneous pointer chains. This rewrites each
// SELECT-of-pointer that has an alloca-derived operand:
//   - LOAD consumers: duplicate the load on each operand and SELECT the value.
//     With scratch-backed private memory a plain speculative load is safe (OOB
//     reads are hardware-masked and the discarded result is selected away);
//     otherwise the duplicated loads are emitted as predicated loads gated on
//     the (possibly negated) condition, so the not-taken operand is never
//     dereferenced.
//   - STORE consumers: split the block into if/then/else and emit one store per
//     operand.
// After this pass the alloca's user chain no longer contains SELECTs, so the
// downstream SOALayoutChecker can promote it.
//
// Controlled by EnableSelectOfAllocaPtrSplit, default OFF.
//
// The pass must stay adjacent to PrivateMemoryResolution: whether plain
// speculative loads are safe depends on private memory being scratch-backed,
// and that decision (ModuleAllocaAnalysis::safeToUseScratchSpace) is only final
// once no pass can add private memory before PrivateMemoryResolution reads it.
llvm::ModulePass *createSplitSelectsOfAllocaPointers();
} // namespace IGC
