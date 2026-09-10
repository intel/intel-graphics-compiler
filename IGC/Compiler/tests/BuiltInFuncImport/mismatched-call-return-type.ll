;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: igc_opt --opaque-pointers -igc-builtin-import -disable-verify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; BIImport: removeFunctionBitcasts
; ------------------------------------------------

; A builtin declared in the source with a wrong return type disagrees with the linked
; definition, which cannot be reconciled and has to be reported here.

; CHECK: error: {{.*}}return type of the call to '_Z20__spirv_ocl_prefetchPU3AS1Kfm' does not match the return type of its definition

define spir_kernel void @test_kernel(ptr addrspace(1) %p, i64 %n, ptr addrspace(1) %dst) {
entry:
  %call = call spir_func i32 @_Z20__spirv_ocl_prefetchPU3AS1Kfm(ptr addrspace(1) %p, i64 %n)
  store i32 %call, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_func void @_Z20__spirv_ocl_prefetchPU3AS1Kfm(ptr addrspace(1) %p, i64 %n) {
entry:
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
