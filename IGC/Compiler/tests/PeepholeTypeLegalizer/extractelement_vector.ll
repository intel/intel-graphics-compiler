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

; CHECK-LABEL: @test_1(
; CHECK:    [[INS:%.*]] = insertelement <4 x i1> zeroinitializer, i1 false, i32 0
; CHECK:    [[RES:%.*]] = extractelement <4 x i1> %0, i32 0
; CHECK:    ret void
define spir_kernel void @test_1(i32 %idx, <4 x i1> %0) #0 {
entry:
  %1 = insertelement <4 x i1> zeroinitializer, i1 false, i32 0
  %.scalar25 = extractelement <4 x i1> %0, i64 0
  ret void
}

; CHECK-LABEL: @test_i2(
; CHECK:    [[RES2:%.*]] = extractelement <4 x i2> %0, i32 0
; CHECK:    ret void
define spir_kernel void @test_i2(i32 %idx, <4 x i2> %0) #0 {
entry:
  %.scalar = extractelement <4 x i2> %0, i64 0
  ret void
}

attributes #0 = { convergent noinline nounwind }

!igc.functions = !{!1, !2}

!0 = !{}
!1 = !{void (i32, <4 x i1>)* @test_1, !0}
!2 = !{void (i32, <4 x i2>)* @test_i2, !0}