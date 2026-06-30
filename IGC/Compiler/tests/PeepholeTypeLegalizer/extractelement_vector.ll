;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt -igc-int-type-legalizer -S < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; i1 elements are legal, so the extract is kept.
; CHECK-LABEL: @test_i1(
; CHECK:    [[E:%.*]] = extractelement <4 x i1> %0, i32 0
; CHECK:    zext i1 [[E]] to i32
; CHECK:    ret void
define spir_kernel void @test_i1(i32 %idx, <4 x i1> %0, i32 addrspace(1)* %out) #0 {
entry:
  %e = extractelement <4 x i1> %0, i64 0
  %z = zext i1 %e to i32
  store i32 %z, i32 addrspace(1)* %out
  ret void
}

; Sub-byte (i2) extractelement: reinterpreted as legal-width integers and
; extracted with a mask (index 0, so no shift needed).
; CHECK-LABEL: @test_i2(
; CHECK:    [[BC2:%.*]] = bitcast <4 x i2> %0 to <1 x i8>
; CHECK:    [[E2:%.*]] = extractelement <1 x i8> [[BC2]], i32 0
; CHECK:    and i8 [[E2]], 3
; CHECK:    ret void
define spir_kernel void @test_i2(i32 %idx, <4 x i2> %0, i32 addrspace(1)* %out) #0 {
entry:
  %e = extractelement <4 x i2> %0, i64 0
  %z = zext i2 %e to i32
  store i32 %z, i32 addrspace(1)* %out
  ret void
}

; Sub-byte (i4) extractelement: nibble 9 is in i32 chunk 1 (9/8) at bit offset 4
; ((9%8)*4), so reinterpret as <4 x i32>, extract chunk 1, shift right by 4, mask.
; CHECK-LABEL: @test_i4(
; CHECK:    [[BC:%.*]] = bitcast <4 x float> %0 to <4 x i32>
; CHECK:    [[C:%.*]] = extractelement <4 x i32> [[BC]], i32 1
; CHECK:    [[S:%.*]] = lshr i32 [[C]], 4
; CHECK:    and i32 [[S]], 15
; CHECK-NOT: i4
; CHECK:    ret void
define spir_kernel void @test_i4(i32 %idx, <4 x float> %0, i32 addrspace(1)* %out) #0 {
entry:
  %bc = bitcast <4 x float> %0 to <32 x i4>
  %nib = extractelement <32 x i4> %bc, i64 9
  %z = zext i4 %nib to i32
  store i32 %z, i32 addrspace(1)* %out
  ret void
}

attributes #0 = { convergent noinline nounwind }

!igc.functions = !{!1, !2, !3}

!0 = !{}
!1 = !{void (i32, <4 x i1>, i32 addrspace(1)*)* @test_i1, !0}
!2 = !{void (i32, <4 x i2>, i32 addrspace(1)*)* @test_i2, !0}
!3 = !{void (i32, <4 x float>, i32 addrspace(1)*)* @test_i4, !0}
