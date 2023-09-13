;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt --platformdg2 --enable-debugify --igc-emu64ops -S < %s 2>&1 | FileCheck %s
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
;
; CHECK: %[[COND_NEG:.+]] = icmp slt i32 %[[ARG_HI]], 0
; CHECK: %[[NEGATE:.+]] = call { i32, i32 } @llvm.genx.GenISA.sub.pair(
; CHECK-SAME: i32 0, i32 0, i32 %[[ARG_LO]], i32 %[[ARG_HI]])
; CHECK: %[[NEG_LO:.+]] = extractvalue { i32, i32 } %[[NEGATE]], 0
; CHECK: %[[NEG_HI:.+]] = extractvalue { i32, i32 } %[[NEGATE]], 1
;
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

; CHECK-LABEL: @test_smax(
; CHECK: %[[CAST_LHS:.+]] = bitcast i64 %argL to <2 x i32>
; CHECK: %[[LHS_LO:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 0
; CHECK: %[[LHS_HI:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 1
; CHECK: %[[CAST_RHS:.+]] = bitcast i64 %argR to <2 x i32>
; CHECK: %[[RHS_LO:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 0
; CHECK: %[[RHS_HI:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 1
;
; COM: Comparing LSBs in case MSB halves are equal
; CHECK: %[[CMP_LO:.+]] = icmp ugt i32 %[[LHS_LO]], %[[RHS_LO]]
; CHECK: %[[CMP_EQ_HI:.+]] = icmp eq i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[COND_LO:.+]] = and i1 %[[CMP_EQ_HI]], %[[CMP_LO]]
; COM: Comparing signed MSBs - sgt
; CHECK: %[[COND_HI:.+]] = icmp sgt i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[RES_COND:.+]] = or i1 %[[COND_LO]], %[[COND_HI]]
;
; CHECK: %[[SEL_LO:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_LO]], i32 %[[RHS_LO]]
; CHECK: %[[SEL_HI:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_HI]], i32 %[[RHS_HI]]
; CHECK: %[[RES_LO:.+]] = insertelement <2 x i32> undef, i32 %[[SEL_LO]], i32 0
; CHECK: %[[RES_VEC:.+]] = insertelement <2 x i32> %[[RES_LO]], i32 %[[SEL_HI]], i32 1
; CHECK: %[[RES_CAST:.+]] = bitcast <2 x i32> %[[RES_VEC]] to i64
; CHECK: call void @use.i64(i64 %[[RES_CAST]])
; CHECK: ret void
define void @test_smax(i64 %argL, i64 %argR) {
  %1 = call i64 @llvm.smax.i64(i64 %argL, i64 %argR)
  call void @use.i64(i64 %1)
  ret void
}

; CHECK-LABEL: @test_smin(
; CHECK: %[[CAST_LHS:.+]] = bitcast i64 %argL to <2 x i32>
; CHECK: %[[LHS_LO:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 0
; CHECK: %[[LHS_HI:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 1
; CHECK: %[[CAST_RHS:.+]] = bitcast i64 %argR to <2 x i32>
; CHECK: %[[RHS_LO:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 0
; CHECK: %[[RHS_HI:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 1
;
; COM: Comparing LSBs in case MSB halves are equal
; CHECK: %[[CMP_LO:.+]] = icmp ult i32 %[[LHS_LO]], %[[RHS_LO]]
; CHECK: %[[CMP_EQ_HI:.+]] = icmp eq i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[COND_LO:.+]] = and i1 %[[CMP_EQ_HI]], %[[CMP_LO]]
; COM: Comparing signed MSBs - slt
; CHECK: %[[COND_HI:.+]] = icmp slt i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[RES_COND:.+]] = or i1 %[[COND_LO]], %[[COND_HI]]
;
; CHECK: %[[SEL_LO:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_LO]], i32 %[[RHS_LO]]
; CHECK: %[[SEL_HI:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_HI]], i32 %[[RHS_HI]]
; CHECK: %[[RES_LO:.+]] = insertelement <2 x i32> undef, i32 %[[SEL_LO]], i32 0
; CHECK: %[[RES_VEC:.+]] = insertelement <2 x i32> %[[RES_LO]], i32 %[[SEL_HI]], i32 1
; CHECK: %[[RES_CAST:.+]] = bitcast <2 x i32> %[[RES_VEC]] to i64
; CHECK: call void @use.i64(i64 %[[RES_CAST]])
; CHECK: ret void
define void @test_smin(i64 %argL, i64 %argR) {
  %1 = call i64 @llvm.smin.i64(i64 %argL, i64 %argR)
  call void @use.i64(i64 %1)
  ret void
}

; CHECK-LABEL: @test_umax(
; CHECK: %[[CAST_LHS:.+]] = bitcast i64 %argL to <2 x i32>
; CHECK: %[[LHS_LO:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 0
; CHECK: %[[LHS_HI:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 1
; CHECK: %[[CAST_RHS:.+]] = bitcast i64 %argR to <2 x i32>
; CHECK: %[[RHS_LO:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 0
; CHECK: %[[RHS_HI:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 1
;
; COM: Comparing LSBs in case MSB halves are equal
; CHECK: %[[CMP_LO:.+]] = icmp ugt i32 %[[LHS_LO]], %[[RHS_LO]]
; CHECK: %[[CMP_EQ_HI:.+]] = icmp eq i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[COND_LO:.+]] = and i1 %[[CMP_EQ_HI]], %[[CMP_LO]]
; COM: Comparing unsigned MSBs - ugt
; CHECK: %[[COND_HI:.+]] = icmp ugt i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[RES_COND:.+]] = or i1 %[[COND_LO]], %[[COND_HI]]
;
; CHECK: %[[SEL_LO:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_LO]], i32 %[[RHS_LO]]
; CHECK: %[[SEL_HI:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_HI]], i32 %[[RHS_HI]]
; CHECK: %[[RES_LO:.+]] = insertelement <2 x i32> undef, i32 %[[SEL_LO]], i32 0
; CHECK: %[[RES_VEC:.+]] = insertelement <2 x i32> %[[RES_LO]], i32 %[[SEL_HI]], i32 1
; CHECK: %[[RES_CAST:.+]] = bitcast <2 x i32> %[[RES_VEC]] to i64
; CHECK: call void @use.i64(i64 %[[RES_CAST]])
; CHECK: ret void
define void @test_umax(i64 %argL, i64 %argR) {
  %1 = call i64 @llvm.umax.i64(i64 %argL, i64 %argR)
  call void @use.i64(i64 %1)
  ret void
}

; CHECK-LABEL: @test_umin(
; CHECK: %[[CAST_LHS:.+]] = bitcast i64 %argL to <2 x i32>
; CHECK: %[[LHS_LO:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 0
; CHECK: %[[LHS_HI:.+]] = extractelement <2 x i32> %[[CAST_LHS]], i32 1
; CHECK: %[[CAST_RHS:.+]] = bitcast i64 %argR to <2 x i32>
; CHECK: %[[RHS_LO:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 0
; CHECK: %[[RHS_HI:.+]] = extractelement <2 x i32> %[[CAST_RHS]], i32 1
;
; COM: Comparing LSBs in case MSB halves are equal
; CHECK: %[[CMP_LO:.+]] = icmp ult i32 %[[LHS_LO]], %[[RHS_LO]]
; CHECK: %[[CMP_EQ_HI:.+]] = icmp eq i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[COND_LO:.+]] = and i1 %[[CMP_EQ_HI]], %[[CMP_LO]]
; COM: Comparing unsigned MSBs - ult
; CHECK: %[[COND_HI:.+]] = icmp ult i32 %[[LHS_HI]], %[[RHS_HI]]
; CHECK: %[[RES_COND:.+]] = or i1 %[[COND_LO]], %[[COND_HI]]
;
; CHECK: %[[SEL_LO:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_LO]], i32 %[[RHS_LO]]
; CHECK: %[[SEL_HI:.+]] = select i1 %[[RES_COND]], i32 %[[LHS_HI]], i32 %[[RHS_HI]]
; CHECK: %[[RES_LO:.+]] = insertelement <2 x i32> undef, i32 %[[SEL_LO]], i32 0
; CHECK: %[[RES_VEC:.+]] = insertelement <2 x i32> %[[RES_LO]], i32 %[[SEL_HI]], i32 1
; CHECK: %[[RES_CAST:.+]] = bitcast <2 x i32> %[[RES_VEC]] to i64
; CHECK: call void @use.i64(i64 %[[RES_CAST]])
; CHECK: ret void
define void @test_umin(i64 %argL, i64 %argR) {
  %1 = call i64 @llvm.umin.i64(i64 %argL, i64 %argR)
  call void @use.i64(i64 %1)
  ret void
}

declare i64 @llvm.abs.i64(i64, i1)
declare i64 @llvm.smax.i64(i64, i64)
declare i64 @llvm.smin.i64(i64, i64)
declare i64 @llvm.umax.i64(i64, i64)
declare i64 @llvm.umin.i64(i64, i64)
declare void @use.i64(i64)

!igc.functions = !{!0, !3, !4, !5, !6}

!0 = !{void (i64)* @test_abs, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i64, i64)* @test_smax, !1}
!4 = !{void (i64, i64)* @test_smin, !1}
!5 = !{void (i64, i64)* @test_umax, !1}
!6 = !{void (i64, i64)* @test_umin, !1}
