/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_SUPPORT_PASSMANAGER_H
#define VC_SUPPORT_PASSMANAGER_H

#include <llvm/IR/LegacyPassManager.h>

#if LLVM_VERSION_MAJOR >= 16
namespace llvm {
class PassInstrumentationCallbacks;
}
#endif

//
// A simple wrapper over a legacy PassManager.
// Extends the default implementation by providing extra hooks that allow
// modification of the default pipeline - like an injection of additional
// verification passes. The intention behind these hooks is to simplify the
// debugging process.
//
//===----------------------------------------------------------------------===//

namespace vc {
struct PassManager : public llvm::legacy::PassManager {
  void add(llvm::Pass *P) override;
};

// Optionally injects additional passes besides the provided \p P.
// Set -vc-choose-pass-manager-override to false to inject those passes inside
// this function and disable inside vc::PassManager::add.
void addPass(llvm::legacy::PassManagerBase &PM, llvm::Pass *P);

#if LLVM_VERSION_MAJOR >= 16
// Registers PassInstrumentationCallbacks to dump IR before/after passes
// in the new pass manager, controlled by -vc-dump-ir-before-pass and
// -vc-dump-ir-after-pass options.
void registerNewPMIRDumpCallbacks(llvm::PassInstrumentationCallbacks &PIC);
#endif
} // namespace vc

#endif // VC_SUPPORT_PASSMANAGER_H
