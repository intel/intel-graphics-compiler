/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_CM_CL_SUPPORT_ATOMICSIFACE_H
#define VC_CM_CL_SUPPORT_ATOMICSIFACE_H

namespace cmcl {
namespace atomic {

namespace MemorySemantics {

// This must be kept in sync with enum in opencl_def.h
enum Enum {
  Relaxed = 0,
  Acquire = 1,
  Release = 2,
  AcquireRelease = 3,
  SequentiallyConsistent = 4
};

} // namespace MemorySemantics

namespace MemoryScope {

// This must be kept in sync with enum in opencl_def.h.
enum Enum {
  Workitem = 0,
  Workgroup = 1,
  Device = 2,
  AllSVMDevices = 3,
  AllDevices = AllSVMDevices,
  Subgroup = 4
};

namespace Name {

constexpr const char *ScopeAllDevices = "all_devices";
constexpr const char *ScopeDevice = "device";
constexpr const char *ScopeWorkgroup = "workgroup";
constexpr const char *ScopeSubgroup = "subgroup";
constexpr const char *ScopeWorkitem = "workitem";

} // namespace Name

inline const char *getScopeNameFromCMCL(Enum S) {
  switch (S) {
  case Workitem:
    return Name::ScopeWorkitem;
  case Subgroup:
    return Name::ScopeSubgroup;
  case Workgroup:
    return Name::ScopeWorkgroup;
  case Device:
    return Name::ScopeDevice;
  case AllDevices:
    return Name::ScopeAllDevices;
  }
  llvm_unreachable("unhandled cmcocl scope");
}

} // namespace MemoryScope

namespace Operation {

// This must be kept in sync with enum in define.h
enum Enum {
  Add = 0x0,
  Sub = 0x1,
  Min = 0x2,
  Max = 0x3,
  Xchg = 0x4,
  Andl = 0x5,
  Orl = 0x6,
  Xorl = 0x7,
  MinSInt = 0x8,
  MaxSInt = 0x9,
  Load = 0xA,
  Store = 0xB,
  CmpXchg = 0xC,
  Fadd = 0xD,
  Fsub = 0xE,
  Fmax = 0xF,
  Fmin = 0x10,
};

} // namespace Operation

} // namespace atomic
} // namespace cmcl

#endif // VC_CM_CL_SUPPORT_ATOMICSIFACE_H
