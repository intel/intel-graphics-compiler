/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXTargetInfo.h"

#include "llvmWrapper/Support/TargetRegistry.h"

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm/MC/MCSubtargetInfo.h"

using namespace llvm;

Target &llvm::getTheGenXTarget32() {
  static Target TheGenXTarget32;
  return TheGenXTarget32;
}

Target &llvm::getTheGenXTarget64() {
  static Target TheGenXTarget64;
  return TheGenXTarget64;
}

extern "C" void LLVMInitializeGenXTargetInfo() {
  RegisterTarget<> X(getTheGenXTarget32(), "genx32", "Intel GenX 32-bit",
                     "genx32");
  RegisterTarget<> Y(getTheGenXTarget64(), "genx64", "Intel GenX 64-bit",
                     "genx64");
}

namespace {
// GenX never emits through LLVM's real MC/AsmPrinter layer (it goes straight
// to vISA via GenXFinalizer), so these only need to be non-null: nothing
// exercises their actual asm/register/feature-table content. Without them,
// TargetMachine::getMCAsmInfo() et al. return null, which LLVM 23's
// MCContext::getTargetOptions() (MAI.getTargetOptions()) unconditionally
// dereferences, crashing as soon as llc builds its standard codegen pipeline
// (MachineModuleInfoWrapperPass). Pre-23 LLVM tolerated the null MCAsmInfo.
MCAsmInfo *createGenXMCAsmInfo(const MCRegisterInfo &, const Triple &,
                               const MCTargetOptions &Options) {
#if LLVM_VERSION_MAJOR >= 23
  return new MCAsmInfo(Options);
#else
  (void)Options;
  return new MCAsmInfo();
#endif
}

MCRegisterInfo *createGenXMCRegisterInfo(const Triple &) {
  return new MCRegisterInfo();
}

MCInstrInfo *createGenXMCInstrInfo() { return new MCInstrInfo(); }

MCSubtargetInfo *createGenXMCSubtargetInfo(const Triple &TT, StringRef CPU,
                                           StringRef FS) {
  // TuneCPU must stay empty: with an empty ProcDesc below, a non-empty
  // TuneCPU makes the base ctor look CPU up in that (empty) table and print
  // "not a recognized processor" for every platform on every compile.
#if LLVM_VERSION_MAJOR >= 23
  return new MCSubtargetInfo(
      TT, CPU, /*TuneCPU=*/StringRef(), FS, StringTable(""),
      /*PF=*/{}, /*PD=*/{}, &MCSchedModel::Default, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr);
#elif LLVM_VERSION_MAJOR >= 22
  return new MCSubtargetInfo(TT, CPU, /*TuneCPU=*/StringRef(), FS, /*PN=*/{},
                             /*PF=*/{}, /*PD=*/{}, nullptr, nullptr, nullptr,
                             nullptr, nullptr, nullptr);
#else
  return new MCSubtargetInfo(TT, CPU, /*TuneCPU=*/StringRef(), FS, /*PF=*/{},
                             /*PD=*/{}, nullptr, nullptr, nullptr, nullptr,
                             nullptr, nullptr);
#endif
}
} // namespace

extern "C" void LLVMInitializeGenXTargetMC() {
  for (Target *T : {&getTheGenXTarget32(), &getTheGenXTarget64()}) {
    TargetRegistry::RegisterMCAsmInfo(*T, createGenXMCAsmInfo);
    TargetRegistry::RegisterMCRegInfo(*T, createGenXMCRegisterInfo);
    TargetRegistry::RegisterMCInstrInfo(*T, createGenXMCInstrInfo);
    TargetRegistry::RegisterMCSubtargetInfo(*T, createGenXMCSubtargetInfo);
  }
}
