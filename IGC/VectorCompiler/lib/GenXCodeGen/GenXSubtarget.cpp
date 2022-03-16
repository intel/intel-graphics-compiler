/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file implements the GenX specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"

#include "GenXSubtarget.h"
#include "common/StringMacros.hpp"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "Probe/Assertion.h"

using namespace llvm;

#define DEBUG_TYPE "subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#define GET_SUBTARGETINFO_MC_DESC
#include "GenXGenSubtargetInfo.inc"

static cl::opt<bool>
    StackScratchMem("stack-scratch-mem",
                    cl::desc("Specify what surface should be used for stack"),
                    cl::init(true));
static cl::opt<bool>
  EnforceLongLongEmulation("dbgonly-enforce-i64-emulation",
                           cl::desc("Enforce i64 emulation"),
                           cl::init(false));
static cl::opt<bool>
    EnforceDivRem32Emulation("dbgonly-enforce-divrem32-emulation",
                             cl::desc("Enforce divrem32 emulation"),
                             cl::init(false));

void GenXSubtarget::initSubtargetFeatures(StringRef CPU, StringRef FS) {
  if (StackScratchMem)
    StackSurf = PreDefined_Surface::PREDEFINED_SURFACE_T255;
  else
    StackSurf = PreDefined_Surface::PREDEFINED_SURFACE_STACK;

  GenXVariant = llvm::StringSwitch<GenXTag>(CPU)
    .Case("generic", GENERIC_ARCH)
    .Case("BDW", GENX_BDW)
    .Case("SKL", GENX_SKL)
    .Case("BXT", GENX_BXT)
    .Case("KBL", GENX_KBL)
    .Case("GLK", GENX_GLK)
    .Case("ICLLP", GENX_ICLLP)
    .Case("TGLLP", GENX_TGLLP)
    .Case("RKL", GENX_RKL)
    .Case("DG1", GENX_DG1)
    .Case("ADLS", GENX_ADLS)
    .Case("ADLP", GENX_ADLP)
    .Cases("XEHP", "XEHP_SDV", XE_HP_SDV)
    .Case("DG2", XE_DG2)
    .Case("PVC", XE_PVC)
    .Case("PVCXT_A0", XE_PVCXT_A0)
    .Case("PVCXT", XE_PVCXT)
    .Default(UNDEFINED_ARCH);

  std::string CPUName(CPU);
  if (CPUName.empty() || GenXVariant == UNDEFINED_ARCH)
    report_fatal_error("Undefined or blank arch passed");

  ParseSubtargetFeatures(CPUName,
#if LLVM_VERSION_MAJOR >= 12
                         /*TuneCPU=*/CPUName,
#endif
                         FS);
  if (EnforceLongLongEmulation)
    EmulateLongLong = true;
  if (EnforceDivRem32Emulation)
    HasIntDivRem32 = false;
}

GenXSubtarget::GenXSubtarget(const Triple &TT, const std::string &CPU,
                             const std::string &FS)
    : GenXGenSubtargetInfo(TT, CPU,
#if LLVM_VERSION_MAJOR >= 12
                           /*TuneCPU=*/CPU,
#endif
                           FS),
      TargetTriple(TT) {

  initSubtargetFeatures(CPU, FS);
}
