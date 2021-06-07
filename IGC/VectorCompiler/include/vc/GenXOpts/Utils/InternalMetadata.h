/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LLVM_GENX_INTERNALMETADATA_H
#define LLVM_GENX_INTERNALMETADATA_H

#include "llvm/IR/Function.h"

namespace llvm {
namespace genx {

namespace DebugMD {
inline constexpr const char DebuggableKernels[] = "VC.Debug.Enable";
}

namespace FunctionMD {
inline constexpr const char GenXKernelInternal[] = "genx.kernel.internal";
inline constexpr const char VCEmulationRoutine[] = "VC.Emulation.Routine";
}

namespace InstMD {
// SVMBlockType metadata serves interesting purpose:
//   Finalizer now don't support properly SVM gathers/scatters less then dword
//   So we are extending everything to 32 but preserving real type in metadata
//   To use it later in CISA builder when we are creating gather/scatter
inline constexpr const char SVMBlockType[] = "SVMBlockType";

// These two are used in prologue/epilogue insertion
inline constexpr const char FuncArgSize[] = "FuncArgSize";
inline constexpr const char FuncRetSize[] = "FuncRetSize";
}

namespace ModuleMD {
inline constexpr const char UseSVMStack[] = "genx.useGlobalMem";
}

namespace internal {

namespace KernelMDOp {
enum {
  FunctionRef,
  OffsetInArgs, // Implicit arguments offset in the byval argument
  ArgIndexes,   // Kernel argument index. Index may not be equal to the IR argNo
                // in the case of linearization
  LinearizationArgs,
  Last
};
}
namespace ArgLinearizationMDOp {
enum {
  Explicit,
  Linearization,
  Last
};
}
namespace LinearizationMDOp {
enum {
  Argument,
  Offset,
  Last
};
}

// ExternalMD is created by vc-intrinsics. Internal has to be created by VC BE.
// This creates initial internal metadata structure. Definition in
// KernelInfo.cpp
void createInternalMD(Function &F);
void replaceInternalFunctionRef(const Function &From, Function &To);

} // namespace internal
} // namespace genx
} // namespace llvm

#endif
