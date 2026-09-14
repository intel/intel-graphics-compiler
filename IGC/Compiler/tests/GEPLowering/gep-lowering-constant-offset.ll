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

define spir_kernel void @test_positive_wrap_local(ptr addrspace(3) %base) #0 {
; CHECK-LABEL: @test_positive_wrap_local(
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(3) %base to i32
; CHECK-NEXT: %[[OFFSET:.*]] = add i32 %[[BASE]], 4
; CHECK-NEXT: inttoptr i32 %[[OFFSET]] to ptr addrspace(3)
  %gep = getelementptr i32, ptr addrspace(3) %base, i32 1073741825
  ret void
}

define spir_kernel void @test_negative_wrap_local(ptr addrspace(3) %base) #0 {
; CHECK-LABEL: @test_negative_wrap_local(
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(3) %base to i32
; CHECK-NEXT: %[[OFFSET:.*]] = add i32 %[[BASE]], -4
; CHECK-NEXT: inttoptr i32 %[[OFFSET]] to ptr addrspace(3)
  %gep = getelementptr i32, ptr addrspace(3) %base, i32 -1073741825
  ret void
}

define spir_kernel void @test_sign_bit_local(ptr addrspace(3) %base) #0 {
; CHECK-LABEL: @test_sign_bit_local(
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(3) %base to i32
; CHECK-NEXT: %[[OFFSET:.*]] = add i32 %[[BASE]], -2147483648
; CHECK-NEXT: inttoptr i32 %[[OFFSET]] to ptr addrspace(3)
  %gep = getelementptr i32, ptr addrspace(3) %base, i32 536870912
  ret void
}

define spir_kernel void @test_positive_global(ptr addrspace(1) %base) #0 {
; CHECK-LABEL: @test_positive_global(
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(1) %base to i64
; CHECK-NEXT: %[[OFFSET:.*]] = add i64 %[[BASE]], 4294967300
; CHECK-NEXT: inttoptr i64 %[[OFFSET]] to ptr addrspace(1)
  %gep = getelementptr i32, ptr addrspace(1) %base, i32 1073741825
  ret void
}

define spir_kernel void @test_negative_global(ptr addrspace(1) %base) #0 {
; CHECK-LABEL: @test_negative_global(
; CHECK: %[[BASE:.*]] = ptrtoint ptr addrspace(1) %base to i64
; CHECK-NEXT: %[[OFFSET:.*]] = add i64 %[[BASE]], -4294967300
; CHECK-NEXT: inttoptr i64 %[[OFFSET]] to ptr addrspace(1)
  %gep = getelementptr i32, ptr addrspace(1) %base, i32 -1073741825
  ret void
}

attributes #0 = { convergent nounwind }

!igc.functions = !{!2, !11, !12, !13, !14, !15}
!IGCMetadata = !{!6}

!0 = !{!1}
!1 = !{!"function_type", i32 0}
!2 = !{ptr @test_negative_constant_private, !0}
!6 = !{!"ModuleMD", !7}
!7 = !{!"compOpt", !8, !9, !10}
!8 = !{!"GreaterThan2GBBufferRequired", i1 false}
!9 = !{!"GreaterThan4GBBufferRequired", i1 false}
!10 = !{!"UseScratchSpacePrivateMemory", i1 true}
!11 = !{ptr @test_positive_wrap_local, !0}
!12 = !{ptr @test_negative_wrap_local, !0}
!13 = !{ptr @test_sign_bit_local, !0}
!14 = !{ptr @test_positive_global, !0}
!15 = !{ptr @test_negative_global, !0}
