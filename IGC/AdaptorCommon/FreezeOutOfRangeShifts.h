/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This pass stops propagation of poison values produced by a shift whose shift
// amount may be greater than or equal to the bit width of the shifted type.
//
// SPIR-V leaves such a shift undefined and so does LLVM IR. Producers should not
// emit one. DPC++ emits one for SYCL CTS sub_group_mask_constructors, which
// accumulates a 64-bit mask with a 32-bit shift whose amount runs up to 62:
//
//   OpDecorate %shl NoUnsignedWrap                 ; nuw claimed on top of it
//   %amt = OpUConvert %uint %i                     ; i64 -> i32, i < 64 step 2
//   %shl = OpShiftLeftLogical %uint %uint_1 %amt
//   %acc = OpBitwiseOr %ulong %acc %wide           ; %wide = UConvert %ulong %shl
//
// translated into LLVM IR as:
//
//   %amt = trunc i64 %i to i32
//   %shl = shl nuw i32 1, %amt                     ; poison once %amt >= 32
//   %acc = or i64 %acc, (zext i32 %shl to i64)
//
// This only became fatal with LLVM 16+. Both LLVM 14 and 16 fold that poison
// into a 'br i1 poison', but they diverge in SCCP. LLVM 14 resolves the branch
// to a concrete successor, so every loop exit survives, whereas LLVM 16 treats
// it as UB, replaces the block with unreachable, and SimplifyCFG then rewrites
// the preceding loop's exit into an unconditional back edge. The resulting
// exitless loop hangs the GPU.

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
llvm::FunctionPass *createFreezeOutOfRangeShiftsPass();
void initializeFreezeOutOfRangeShiftsLPMPass(llvm::PassRegistry &);

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. No analysis dependencies, so a plain function pass. name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class FreezeOutOfRangeShiftsNPM : public llvm::PassInfoMixin<FreezeOutOfRangeShiftsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-freeze-out-of-range-shifts"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
