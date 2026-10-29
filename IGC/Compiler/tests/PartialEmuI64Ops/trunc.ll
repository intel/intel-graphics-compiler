;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : trunc of a split i64
; ------------------------------------------------

; A split i64 with a non-split user is rebuilt as bitcast <2 x i32> to i64.
; trunc of the rebuilt value must read element 0, so the rebuild stays dead
; when its only other user reads the high half.

define void @test_trunc_xor(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_trunc_xor(
; CHECK:         [[LO:%.*]] = xor i32
; CHECK-NEXT:    [[HI:%.*]] = xor i32
; CHECK-NEXT:    [[V0:%.*]] = insertelement <2 x i32> undef, i32 [[LO]], i32 0
; CHECK-NEXT:    [[V1:%.*]] = insertelement <2 x i32> [[V0]], i32 [[HI]], i32 1
; CHECK-NEXT:    [[I64:%.*]] = bitcast <2 x i32> [[V1]] to i64
; CHECK-NEXT:    [[VEC:%.*]] = bitcast i64 [[I64]] to <2 x i32>
; CHECK-NEXT:    [[HIUSE:%.*]] = extractelement <2 x i32> [[VEC]], i64 1
; CHECK-NEXT:    [[LOUSE:%.*]] = extractelement <2 x i32> [[V1]], i32 0
; CHECK-NEXT:    call void @use.i32(i32 [[LOUSE]], i32 [[HIUSE]])
; CHECK-NOT:     trunc
; CHECK:         ret void
;
  %1 = xor i64 %src1, %src2
  %2 = bitcast i64 %1 to <2 x i32>
  %3 = extractelement <2 x i32> %2, i64 1
  %4 = trunc i64 %1 to i32
  call void @use.i32(i32 %4, i32 %3)
  ret void
}

define void @test_trunc_i16(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_trunc_i16(
; CHECK:         [[I64:%.*]] = bitcast <2 x i32> {{.*}} to i64
; CHECK-NEXT:    [[TR:%.*]] = trunc i64 [[I64]] to i16
; CHECK-NEXT:    call void @use.i16(i16 [[TR]])
;
  %1 = and i64 %src1, %src2
  %2 = trunc i64 %1 to i16
  call void @use.i16(i16 %2)
  ret void
}

declare void @use.i32(i32, i32)
declare void @use.i16(i16)

!igc.functions = !{!0, !3}

!0 = !{void (i64, i64)* @test_trunc_xor, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i64, i64)* @test_trunc_i16, !1}
