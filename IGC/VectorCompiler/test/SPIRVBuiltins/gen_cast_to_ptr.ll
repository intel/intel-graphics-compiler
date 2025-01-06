;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func i8* @_Z42__spirv_GenericCastToPtrExplicit_ToPrivatePU3AS4ci(i8 addrspace(4)*, i32) #0
declare spir_func i8 addrspace(3)* @_Z40__spirv_GenericCastToPtrExplicit_ToLocalPU3AS4ci(i8 addrspace(4)*, i32) #0
declare spir_func i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPU3AS4ci(i8 addrspace(4)*, i32) #0

define spir_func i8* @to_private(i8 addrspace(4)* %ptr) {
  ; CHECK-LABEL: @to_private
  ; CHECK-TYPED-PTRS: %res = call i8* @llvm.vc.internal.cast.to.ptr.explicit.p0i8(i8 addrspace(4)* %ptr)
  ; CHECK-OPAQUE-PTRS: %res = call ptr @llvm.vc.internal.cast.to.ptr.explicit.p0(ptr addrspace(4) %ptr)
  %res = call i8* @_Z42__spirv_GenericCastToPtrExplicit_ToPrivatePU3AS4ci(i8 addrspace(4)* %ptr, i32 7)
  ret i8* %res
}

define spir_func i8 addrspace(3)* @to_local(i8 addrspace(4)* %ptr) {
  ; CHECK-LABEL: @to_local
  ; CHECK-TYPED-PTRS: %res = call i8 addrspace(3)* @llvm.vc.internal.cast.to.ptr.explicit.p3i8(i8 addrspace(4)* %ptr)
  ; CHECK-OPAQUE-PTRS: %res = call ptr addrspace(3) @llvm.vc.internal.cast.to.ptr.explicit.p3(ptr addrspace(4) %ptr)
  %res = call i8 addrspace(3)* @_Z40__spirv_GenericCastToPtrExplicit_ToLocalPU3AS4ci(i8 addrspace(4)* %ptr, i32 4)
  ret i8 addrspace(3)* %res
}

define spir_func i8 addrspace(1)* @to_global(i8 addrspace(4)* %ptr) {
  ; CHECK-LABEL: @to_global
  ; CHECK-TYPED-PTRS: %res = call i8 addrspace(1)* @llvm.vc.internal.cast.to.ptr.explicit.p1i8(i8 addrspace(4)* %ptr)
  ; CHECK-OPAQUE-PTRS: %res = call ptr addrspace(1) @llvm.vc.internal.cast.to.ptr.explicit.p1(ptr addrspace(4) %ptr)
  %res = call i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPU3AS4ci(i8 addrspace(4)* %ptr, i32 5)
  ret i8 addrspace(1)* %res
}


attributes #0 = { nounwind }
