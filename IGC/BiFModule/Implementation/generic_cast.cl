/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.h"
#include "../Headers/spirv.h"

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE private void *__attribute__((overloadable)) __spirv_GenericCastToPtrExplicit_ToPrivate(generic char *Pointer,
                                                                                              int Storage) {
  return __builtin_IB_to_private(Pointer);
}

INLINE local void *__attribute__((overloadable)) __spirv_GenericCastToPtrExplicit_ToLocal(generic char *Pointer,
                                                                                          int Storage) {
  return __builtin_IB_to_local(Pointer);
}

INLINE global void *__attribute__((overloadable)) __spirv_GenericCastToPtrExplicit_ToGlobal(generic char *Pointer,
                                                                                            int Storage) {
  if ((__spirv_GenericCastToPtrExplicit_ToLocal(Pointer, Storage) == NULL) &
      (__spirv_GenericCastToPtrExplicit_ToPrivate(Pointer, Storage) == NULL)) {
    return (global void *)(Pointer);
  }

  return NULL;
}

INLINE uint __attribute__((overloadable)) __spirv_GenericPtrMemSemantics(const generic char *Pointer) {
  if (__spirv_GenericCastToPtrExplicit_ToLocal(Pointer, StorageWorkgroup) != NULL) {
    return (uint)WorkgroupMemory;
  }

  return (uint)CrossWorkgroupMemory;
}
#endif
