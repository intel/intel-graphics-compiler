/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file implements the GenX specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "GenXSubtarget.h"

#include "vc/Utils/GenX/IntrinsicsWrapper.h"

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "IGC/common/StringMacros.hpp"
#include "Probe/Assertion.h"
#include "common/StringMacros.hpp"

#include <algorithm>

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
static cl::opt<bool> EnforceLongLongEmulation("dbgonly-enforce-i64-emulation",
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
                 .Case("XeLPGPlus", XeLPGPlus)
                 .Case("XeHPC", XeHPC)
                 .Case("XeHPCVG", XeHPCVG)
                 .Case("Xe2", Xe2)
                 .Case("Xe3", Xe3)
                 .Default(Invalid);

  std::string CPUName(CPU);
  if (CPUName.empty() || TargetId == Invalid)
    report_fatal_error("Undefined or blank arch passed");

  ParseSubtargetFeatures(CPUName, /*TuneCPU=*/CPUName, FS);
  if (EnforceLongLongEmulation)
    EmulateLongLong = true;
  if (EnforceDivRem32Emulation)
    HasIntDivRem32 = false;
}

GenXSubtarget::GenXSubtarget(const Triple &TT, const std::string &CPU,
                             const std::string &FS)
    : GenXGenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS), TargetTriple(TT) {
  initSubtargetFeatures(CPU, FS);
}

uint32_t GenXSubtarget::getMaxThreadsNumPerSubDevice() const {
  switch (TargetId) {
  default:
    break;
  case XeHP:
  case XeHPG:
  case XeLPG:
  case XeLPGPlus:
  case XeHPC:
  case XeHPCVG:
    return 1 << 12;
  case Xe2:
    return 1 << 13;
  case Xe3:
    return 1 << 15;
  }
  return 0;
}

ArrayRef<std::pair<int, int>> GenXSubtarget::getThreadIdReservedBits() const {
  // HWTID is calculated using a concatenation of TID:EUID:SubSliceID:SliceID
  switch (TargetId) {
  case GenXSubtarget::XeHP:
  case GenXSubtarget::XeHPG:
  case GenXSubtarget::XeLPG:
  case GenXSubtarget::XeLPGPlus: {
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
  case GenXSubtarget::XeHPC:
  case GenXSubtarget::XeHPCVG: {
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
  case GenXSubtarget::Xe2: {
    // [15:11] Slice ID.
    // [10] : Reserved MBZ
    // [9:8] SubSlice ID
    // [7] : Reserved MBZ
    // [6:4] : EUID
    // [3] : Reserved MBZ
    // [2:0] : TID
    static const std::pair<int, int> Bits[] = {{10, 1}, {7, 1}, {3, 1}};
    return Bits;
  }
  case GenXSubtarget::Xe3: {
    // [17:14] Slice ID.
    // [13:12] : Reserved MBZ
    // [11:8] SubSlice ID
    // [7] : Reserved MBZ
    // [6:4] : EUID
    // [3:0] : TID
    static const std::pair<int, int> Bits[] = {{12, 2}, {7, 1}};
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
  case GenXSubtarget::XeLPG:
  case GenXSubtarget::XeLPGPlus: {
    // [13:11] Slice ID.
    // [10:9] Dual - SubSlice ID
    // [8] SubSlice ID.
    static const std::pair<int, int> Bits[] = {{8, 6}};
    return Bits;
  }
  case GenXSubtarget::XeHPC:
  case GenXSubtarget::XeHPCVG: {
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
  case GenXSubtarget::XeLPG:
  case GenXSubtarget::XeLPGPlus: {
    // [7] : EUID[2]
    // [6] : Reserved
    // [5:4] EUID[1:0]
    static const std::pair<int, int> Bits[] = {{4, 2}, {7, 1}};
    return Bits;
  }
  case GenXSubtarget::XeHPC:
  case GenXSubtarget::XeHPCVG: {
    // [8] : EUID[2]
    // [7:6] : Reserved
    // [5:4] EUID[1:0]
    static const std::pair<int, int> Bits[] = {{4, 2}, {8, 1}};
    return Bits;
  }
  case GenXSubtarget::Xe2:
  case GenXSubtarget::Xe3: {
    // [6:4] : EUID
    static const std::pair<int, int> Bits[] = {{4, 3}};
    return Bits;
  }
  default:
    // All other platforms have pre-defined Thread ID register
    return {};
  }
}

ArrayRef<std::pair<int, int>> GenXSubtarget::getThreadIdBits() const {
  switch (TargetId) {
  case GenXSubtarget::Xe3: {
    // [3:0] : EUID
    static const std::pair<int, int> Bits[] = {{0, 4}};
    return Bits;
  }
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
  case XeLPGPlus:
    return TARGET_PLATFORM::Xe_ARL;
  case XeHPC:
    if (!partialI64Emulation())
      return TARGET_PLATFORM::Xe_PVC;
    LLVM_FALLTHROUGH;
  case XeHPCVG:
    return TARGET_PLATFORM::Xe_PVCXT;
  case Xe2:
    return TARGET_PLATFORM::Xe2;
  case Xe3:
    return TARGET_PLATFORM::Xe3;
  default:
    return TARGET_PLATFORM::GENX_NONE;
  }
}

bool GenXSubtarget::isIntrinsicSupported(unsigned ID) const {
  if (vc::InternalIntrinsic::isInternalIntrinsic(ID))
    return isInternalIntrinsicSupported(ID);
  if (GenXIntrinsic::isGenXIntrinsic(ID))
    return GenXIntrinsic::isSupportedPlatform(getCPU().str(), ID);
  return ID < Intrinsic::num_intrinsics;
}

bool GenXSubtarget::isInternalIntrinsicSupported(unsigned ID) const {
  IGC_ASSERT(vc::InternalIntrinsic::isInternalIntrinsic(ID));

#define GET_INTRINSIC_TARGET_FEATURE_TABLE
#define GET_INTRINSIC_TARGET_FEATURE_CHECKER
#include "InternalIntrinsicDescription.gen"
#undef GET_INTRINSIC_TARGET_FEATURE_TABLE
#undef GET_INTRINSIC_TARGET_FEATURE_CHECKER

  return true;
}

ArrayRef<unsigned> GenXSubtarget::getSupportedGRFSizes() const {
  switch (TargetId) {
  case GenXSubtarget::XeHP:
  case GenXSubtarget::XeHPG:
  case GenXSubtarget::XeLPG:
  case GenXSubtarget::XeLPGPlus:
  case GenXSubtarget::XeHPC:
  case GenXSubtarget::XeHPCVG:
  case GenXSubtarget::Xe2: {
    static const unsigned Supported[] = {128, 256};
    return Supported;
  }
  case GenXSubtarget::Xe3: {
    static const unsigned Supported[] = {32, 64, 96, 128, 160, 192, 256};
    return Supported;
  }
  default: {
    static const unsigned Supported[] = {128}; // platforms <= TGL
    return Supported;
  }
  }
}

bool GenXSubtarget::isValidGRFSize(unsigned Size) const {
  const auto GrfSizes = getSupportedGRFSizes();
  return std::binary_search(GrfSizes.begin(), GrfSizes.end(), Size);
}
