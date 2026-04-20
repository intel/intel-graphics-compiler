;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-gep-lowering -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GEPLowering
; ------------------------------------------------

; Verifies that GEP lowering correctly falls back to 64-bit pointer arithmetic
; when a 32-bit offset scaling could cause an overflow and drop the high bit.
; It ensures that safe offsets still use 32-bit arithmetic for performance.

define spir_kernel void @test_variable_half(ptr addrspace(1) %base, i32 %raw_offset) #0 {
; CHECK-LABEL: @test_variable_half
; CHECK: %[[EXT:[a-zA-Z0-9_]+]] = zext i32 %raw_offset to i64
; CHECK: %[[SHL:[a-zA-Z0-9_]+]] = shl i64 %[[EXT]], 1
  %offset = zext i32 %raw_offset to i64
  %gep = getelementptr inbounds half, ptr addrspace(1) %base, i64 %offset
  ret void
}

define spir_kernel void @test_constant_overflow(ptr addrspace(1) %base) #0 {
; CHECK-LABEL: @test_constant_overflow
; CHECK-NOT: shl i32
; CHECK: add i64 %{{.*}}, -4294967296
  %gep = getelementptr inbounds half, ptr addrspace(1) %base, i32 2147483648
  ret void
}

define spir_kernel void @test_constant_safe(ptr addrspace(1) %base) #0 {
; CHECK-LABEL: @test_constant_safe
; ConstantFolding folds add i32, zext to i64 and add i64 into single add i64.
; CHECK: add i64 %{{.*}}, 20
  %gep = getelementptr inbounds half, ptr addrspace(1) %base, i32 10
  ret void
}

define spir_kernel void @test_variable_i8(ptr addrspace(1) %base, i16 %raw_offset) #0 {
; CHECK-LABEL: @test_variable_i8
; CHECK: add i32
; CHECK: zext i32 %{{.*}} to i64
  %offset = zext i16 %raw_offset to i32
  %gep = getelementptr inbounds i8, ptr addrspace(1) %base, i32 %offset
  ret void
}

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }

!igc.functions = !{!2, !3, !4, !5}
!IGCMetadata = !{!6}

!0 = !{!1}
!1 = !{!"function_type", i32 0}
!2 = !{ptr @test_variable_half, !0}
!3 = !{ptr @test_constant_overflow, !0}
!4 = !{ptr @test_constant_safe, !0}
!5 = !{ptr @test_variable_i8, !0}
!6 = !{!"ModuleMD", !7}
!7 = !{!"compOpt", !8, !9}
!8 = !{!"GreaterThan2GBBufferRequired", i1 true}
!9 = !{!"GreaterThan4GBBufferRequired", i1 false}
