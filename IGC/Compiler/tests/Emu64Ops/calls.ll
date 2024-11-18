;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus

; RUN: igc_opt --opaque-pointers --platformdg2 --enable-debugify --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; CHECK-LABEL: @test_abs(
; CHECK: %[[CAST:.+]] = bitcast i64 %arg to <2 x i32>
; CHECK: %[[ARG_LO:.+]] = extractelement <2 x i32> %[[CAST]], i32 0
; CHECK: %[[ARG_HI:.+]] = extractelement <2 x i32> %[[CAST]], i32 1

; CHECK: %[[COND_NEG:.+]] = icmp slt i32 %[[ARG_HI]], 0
; CHECK: %[[NEGATE:.+]] = call { i32, i32 } @llvm.genx.GenISA.sub.pair(
; CHECK-SAME: i32 0, i32 0, i32 %[[ARG_LO]], i32 %[[ARG_HI]])
; CHECK: %[[NEG_LO:.+]] = extractvalue { i32, i32 } %[[NEGATE]], 0
; CHECK: %[[NEG_HI:.+]] = extractvalue { i32, i32 } %[[NEGATE]], 1

; CHECK: %[[SEL_LO:.+]] = select i1 %[[COND_NEG]], i32 %[[NEG_LO]], i32 %[[ARG_LO]]
; CHECK: %[[SEL_HI:.+]] = select i1 %[[COND_NEG]], i32 %[[NEG_HI]], i32 %[[ARG_HI]]
; CHECK: %[[RES_LO:.+]] = insertelement <2 x i32> undef, i32 %[[SEL_LO]], i32 0
; CHECK: %[[RES_VEC:.+]] = insertelement <2 x i32> %[[RES_LO]], i32 %[[SEL_HI]], i32 1
; CHECK: %[[RES_CAST:.+]] = bitcast <2 x i32> %[[RES_VEC]] to i64
; CHECK: call void @use.i64(i64 %[[RES_CAST]])
; CHECK: ret void
define void @test_abs(i64 %arg) {
  %1 = call i64 @llvm.abs.i64(i64 %arg, i1 false)
  call void @use.i64(i64 %1)
  ret void
}

declare i64 @llvm.abs.i64(i64, i1)
declare void @use.i64(i64)

!igc.functions = !{!0}

!0 = !{void (i64)* @test_abs, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
