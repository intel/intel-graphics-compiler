;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys,llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg --regkey EnableOpaquePointersBackend=1 -S %s 2>&1

; Make sure we correctly handle the ptr bitcast on opaque pointers and the compilation suceedees.

define spir_kernel void @test() {
  %arr = alloca [32 x float], align 4
  %p = bitcast ptr %arr to ptr
  call spir_func void @foo(ptr %p)
  ret void
}

declare spir_func void @foo(ptr)

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
