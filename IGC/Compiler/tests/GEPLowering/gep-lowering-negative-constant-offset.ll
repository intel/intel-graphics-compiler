;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
; RUN: igc_opt --opaque-pointers --igc-gep-lowering -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GEPLowering
; ------------------------------------------------

; A negative constant index on a 32-bit (scratch private) pointer must wrap into the 32-bit offset.

define spir_kernel void @test_negative_constant_private(ptr %base) #0 {
; CHECK-LABEL: @test_negative_constant_private
; CHECK: %[[PTI:[a-zA-Z0-9_]+]] = ptrtoint ptr %base to i32
; CHECK: add i32 %[[PTI]], -4
  %gep = getelementptr i8, ptr %base, i64 -4
  %v = load float, ptr %gep, align 4
  ret void
}

attributes #0 = { convergent nounwind }

!igc.functions = !{!2}
!IGCMetadata = !{!6}

!0 = !{!1}
!1 = !{!"function_type", i32 0}
!2 = !{ptr @test_negative_constant_private, !0}
!6 = !{!"ModuleMD", !7}
!7 = !{!"compOpt", !8, !9, !10}
!8 = !{!"GreaterThan2GBBufferRequired", i1 false}
!9 = !{!"GreaterThan4GBBufferRequired", i1 false}
!10 = !{!"UseScratchSpacePrivateMemory", i1 true}
