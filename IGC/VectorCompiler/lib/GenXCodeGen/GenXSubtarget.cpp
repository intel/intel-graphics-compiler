/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

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

  TargetId = llvm::StringSwitch<GenXTargetId>(CPU)
                 .Case("Gen8", Gen8)
                 .Case("Gen9", Gen9)
                 .Case("Gen9LP", Gen9LP)
                 .Case("Gen11", Gen11)
                 .Case("XeLP", XeLP)
                 .Case("XeHP", XeHP)
                 .Case("XeHPG", XeHPG)
                 .Case("XeLPG", XeLPG)
                 .Case("XeHPC", XeHPC)
                 .Default(Invalid);

  std::string CPUName(CPU);
  if (CPUName.empty() || TargetId == Invalid)
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

uint32_t GenXSubtarget::getMaxThreadsNumPerSubDevice() const {
  switch (TargetId) {
  default:
    break;
  case XeHP:
  case XeHPG:
  case XeLPG:
    return 1 << 12;
  case XeHPC:
    return 1 << 12;
  }
  return 0;
}

ArrayRef<std::pair<int, int>> GenXSubtarget::getThreadIdReservedBits() const {
  // HWTID is calculated using a concatenation of TID:EUID:SubSliceID:SliceID
  switch (TargetId) {
  case GenXSubtarget::XeHP:
  case GenXSubtarget::XeHPG:
  case GenXSubtarget::XeLPG: {
    // [13:11] Slice ID.
    // [10:9] Dual - SubSlice ID
    // [8] SubSlice ID.
    // [7] : EUID[2]
    // [6] : Reserved
    // [5:4] EUID[1:0]
    // [3] : Reserved MBZ
    // [2:0] : TID
    static const std::pair<int, int> Bits[] = {{6, 1}, {3, 1}};
    return Bits;
  }
  case GenXSubtarget::XeHPC: {
    // [14:12] Slice ID.
    // [11:9] SubSlice ID
    // [8] : EUID[2]
    // [7:6] : Reserved
    // [5:4] EUID[1:0]
    // [3] : Reserved MBZ
    // [2:0] : TID
    static const std::pair<int, int> Bits[] = {{6, 2}, {3, 1}};
    return Bits;
  }
  default:
    // All other platforms have pre-defined Thread ID register
    return {};
  }
}

ArrayRef<std::pair<int, int>> GenXSubtarget::getSubsliceIdBits() const {
  if (hasPreemption()) {
    // For targets supporting mid-thread preemption, returns msg0.logical_ssid
    static const std::pair<int, int> Bits[] = {{0, 8}};
    return Bits;
  }

  // Return slice id and subslice id to concatenate them
  switch (TargetId) {
  case GenXSubtarget::XeHP:
  case GenXSubtarget::XeHPG:
  case GenXSubtarget::XeLPG: {
    // [13:11] Slice ID.
    // [10:9] Dual - SubSlice ID
    // [8] SubSlice ID.
    static const std::pair<int, int> Bits[] = {{8, 6}};
    return Bits;
  }
  case GenXSubtarget::XeHPC: {
    // [14:12] Slice ID.
    // [11:9] SubSlice ID
    static const std::pair<int, int> Bits[] = {{9, 6}};
    return Bits;
  }
  default:
    return {};
  }
}

ArrayRef<std::pair<int, int>> GenXSubtarget::getEUIdBits() const {
  switch (TargetId) {
  case GenXSubtarget::XeHP:
  case GenXSubtarget::XeHPG:
  case GenXSubtarget::XeLPG: {
    // [7] : EUID[2]
    // [6] : Reserved
    // [5:4] EUID[1:0]
    static const std::pair<int, int> Bits[] = {{4, 2}, {7, 1}};
    return Bits;
  }
  case GenXSubtarget::XeHPC: {
    // [8] : EUID[2]
    // [7:6] : Reserved
    // [5:4] EUID[1:0]
    static const std::pair<int, int> Bits[] = {{4, 2}, {8, 1}};
    return Bits;
  }
  default:
    // All other platforms have pre-defined Thread ID register
    return {};
  }
}

ArrayRef<std::pair<int, int>> GenXSubtarget::getThreadIdBits() const {
  switch (TargetId) {
  default:
    static const std::pair<int, int> Bits[] = {{0, 3}};
    return Bits;
  }
}

TARGET_PLATFORM GenXSubtarget::getVisaPlatform() const {
  switch (TargetId) {
  case Gen8:
    return TARGET_PLATFORM::GENX_BDW;
  case Gen9:
    return TARGET_PLATFORM::GENX_SKL;
  case Gen9LP:
    return TARGET_PLATFORM::GENX_BXT;
  case Gen11:
    return TARGET_PLATFORM::GENX_ICLLP;
  case XeLP:
    return TARGET_PLATFORM::GENX_TGLLP;
  case XeHP:
    return TARGET_PLATFORM::Xe_XeHPSDV;
  case XeHPG:
    return TARGET_PLATFORM::Xe_DG2;
  case XeLPG:
    return TARGET_PLATFORM::Xe_MTL;
  case XeHPC:
    if (!partialI64Emulation())
      return TARGET_PLATFORM::Xe_PVC;
    return TARGET_PLATFORM::Xe_PVCXT;
  default:
    return TARGET_PLATFORM::GENX_NONE;
  }
}
