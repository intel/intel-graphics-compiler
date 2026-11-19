/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class ModulePass;
} // namespace llvm

namespace IGC {
// Pre-pass run before LowerGEPForPrivMem and PrivateMemoryResolution.
//
// When a load's pointer comes from a PHI that merges an alloca-derived pointer
// with another pointer, this rewrites the load into one load per PHI
// predecessor, joined by a value PHI. That turns a pointer-level merge into a
// value-level one, which unblocks SoA/GRF promotion (the SOA layout checkers
// reject an alloca whose pointer flows into such a PHI).
//
// Targeted input pattern:
//   if (condition) {
//     %p1 = addrspacecast (gep stack[idx]) to i8 addrspace(4)*  ; from alloca
//   } else {
//     %p2 = bitcast (gep m_data, off) to i8 addrspace(4)*     ; from arg
//   }
//   %phi = phi i8 addrspace(4)* [ %p1, bb1 ], [ %p2, bb2 ]
//   %v = load float, %phi
//
// The rewrite is applied where it is provably non-speculative (each PHI predecessor's sole successor is the merge
// block) and clobber-free (each load is in the merge block with no memory-writing instruction before it). Additionally,
// when across function SoA is enabled and platform has LSC support, split is done using predicated loads in cases where
// it would otherwise be unsafe to hoist the load non-speculatively.
//
// VerifyAlloca restricts the rewrite to allocas for which splitting a pointer merge is worthwhile.
// CheckCrossFnArgs additionally treats the callee arguments an alloca pointer flows into as roots of the merge
// (EnableSOAAcrossCalls).
llvm::ModulePass *createSplitPHIsOfAllocaPointers(bool VerifyAlloca = false, bool CheckCrossFnArgs = false);
} // namespace IGC
