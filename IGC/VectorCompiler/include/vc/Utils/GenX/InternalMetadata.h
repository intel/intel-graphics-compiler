/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_INTERNALMETADATA_H
#define VC_UTILS_GENX_INTERNALMETADATA_H

namespace llvm {
class Function;
} // namespace llvm

namespace vc {

namespace FunctionMD {
inline constexpr const char GenXKernelInternal[] = "genx.kernel.internal";
inline constexpr const char VCEmulationRoutine[] = "VC.Emulation.Routine";

// amount of stack calculated for kernel
// no attribute means that GenXStackUsage pass failed (recursion, etc)
// attribute created by GenXStackUsage and used to create patch token
inline constexpr const char VCStackAmount[] = "VC.Stack.Amount";
} // namespace FunctionMD

namespace InstMD {
// SVMBlockType metadata serves interesting purpose:
//   Finalizer now don't support properly SVM gathers/scatters less then dword
//   So we are extending everything to 32 but preserving real type in metadata
//   To use it later in CISA builder when we are creating gather/scatter
inline constexpr const char SVMBlockType[] = "SVMBlockType";

// These two are used in prologue/epilogue insertion
inline constexpr const char FuncArgSize[] = "FuncArgSize";
inline constexpr const char FuncRetSize[] = "FuncRetSize";
} // namespace InstMD

namespace internal {

namespace KernelMDOp {
enum {
  FunctionRef,
  OffsetInArgs, // Implicit arguments offset in the byval argument
  ArgIndexes,   // Kernel argument index. Index may not be equal to the IR argNo
                // in the case of linearization
  LinearizationArgs,
  BTIndices,
  Last
};
}
namespace ArgLinearizationMDOp {
enum { Explicit, Linearization, Last };
}
namespace LinearizationMDOp {
enum { Argument, Offset, Last };
}

// ExternalMD is created by vc-intrinsics. Internal has to be created by VC BE.
// This creates initial internal metadata structure. Definition in
// KernelInfo.cpp
void createInternalMD(llvm::Function &F);
void replaceInternalFunctionRef(const llvm::Function &From, llvm::Function &To);

} // namespace internal
} // namespace vc

#endif // VC_UTILS_GENX_INTERNALMETADATA_H
