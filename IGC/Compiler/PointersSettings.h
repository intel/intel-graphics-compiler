/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class LLVMContext;
}

namespace IGC {
// How pointers are represented within a single LLVM context. LLVM 16 stores
// this per context, so one IGC build can serve APIs running in either mode.
enum class PointerMode { Typed, Opaque };

// Mode for a context serving any non-compute API. This is also the mode of
// every build-time bitcode artifact that those adaptors parse into their own
// context, and of the offline tools producing them.
PointerMode GetDefaultPointerMode();

// Mode for a context serving the compute APIs.
PointerMode GetComputePointerMode();

// Mode the given context is actually operating in. Prefer this over the
// per-API queries above whenever a context is at hand (it stays correct for
// modules whose mode was decided elsewhere).
bool AreOpaquePointersEnabled(llvm::LLVMContext &Ctx);
} // namespace IGC
