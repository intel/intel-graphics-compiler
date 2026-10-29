;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -SplitIndirectEEtoSel -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SplitIndirectEEtoSel
; ------------------------------------------------

define void @test_mul_var(i32 %x, i32 %m, <4 x float> %v, ptr %dst) {
; CHECK-LABEL: @test_mul_var(
; CHECK:    [[IDX:%.*]] = mul nsw i32 [[X:%.*]], [[M:%.*]]
; CHECK:    extractelement <4 x float> [[V:%.*]], i32 [[IDX]]
; CHECK:    [[C0:%.*]] = icmp eq i32 [[IDX]], 0
; CHECK:    [[E0:%.*]] = extractelement <4 x float> [[V]], i32 0
; CHECK:    [[S0:%.*]] = select i1 [[C0]], float [[E0]], float undef
; CHECK:    [[C1:%.*]] = icmp eq i32 [[IDX]], 1
; CHECK:    [[E1:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK:    [[S1:%.*]] = select i1 [[C1]], float [[E1]], float [[S0]]
; CHECK:    [[C2:%.*]] = icmp eq i32 [[IDX]], 2
; CHECK:    [[E2:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK:    [[S2:%.*]] = select i1 [[C2]], float [[E2]], float [[S1]]
; CHECK:    [[C3:%.*]] = icmp eq i32 [[IDX]], 3
; CHECK:    [[E3:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK:    [[S3:%.*]] = select i1 [[C3]], float [[E3]], float [[S2]]
; CHECK:    store float [[S3]], ptr [[DST:%.*]], align 4
;
  %idx = mul nsw i32 %x, %m
  %e = extractelement <4 x float> %v, i32 %idx
  store float %e, ptr %dst, align 4
  ret void
}

define void @test_mul_var_add(i32 %x, i32 %m, <4 x float> %v, ptr %dst) {
; CHECK-LABEL: @test_mul_var_add(
; CHECK:    [[MM:%.*]] = mul nsw i32 [[X:%.*]], [[M:%.*]]
; CHECK:    [[IDX:%.*]] = add nsw i32 [[MM]], 1
; CHECK:    extractelement <4 x float> [[V:%.*]], i32 [[IDX]]
; CHECK:    [[C0:%.*]] = icmp eq i32 [[IDX]], 0
; CHECK:    [[E0:%.*]] = extractelement <4 x float> [[V]], i32 0
; CHECK:    [[S0:%.*]] = select i1 [[C0]], float [[E0]], float undef
; CHECK:    [[C1:%.*]] = icmp eq i32 [[IDX]], 1
; CHECK:    [[E1:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK:    [[S1:%.*]] = select i1 [[C1]], float [[E1]], float [[S0]]
; CHECK:    [[C2:%.*]] = icmp eq i32 [[IDX]], 2
; CHECK:    [[E2:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK:    [[S2:%.*]] = select i1 [[C2]], float [[E2]], float [[S1]]
; CHECK:    [[C3:%.*]] = icmp eq i32 [[IDX]], 3
; CHECK:    [[E3:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK:    [[S3:%.*]] = select i1 [[C3]], float [[E3]], float [[S2]]
; CHECK:    store float [[S3]], ptr [[DST:%.*]], align 4
;
  %mm = mul nsw i32 %x, %m
  %idx = add nsw i32 %mm, 1
  %e = extractelement <4 x float> %v, i32 %idx
  store float %e, ptr %dst, align 4
  ret void
}

define void @test_shl_var(i32 %x, i32 %s, <4 x float> %v, ptr %dst) {
; CHECK-LABEL: @test_shl_var(
; CHECK:    [[IDX:%.*]] = shl nuw nsw i32 [[X:%.*]], [[S:%.*]]
; CHECK:    extractelement <4 x float> [[V:%.*]], i32 [[IDX]]
; CHECK:    [[C0:%.*]] = icmp eq i32 [[IDX]], 0
; CHECK:    [[E0:%.*]] = extractelement <4 x float> [[V]], i32 0
; CHECK:    [[S0:%.*]] = select i1 [[C0]], float [[E0]], float undef
; CHECK:    [[C1:%.*]] = icmp eq i32 [[IDX]], 1
; CHECK:    [[E1:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK:    [[S1:%.*]] = select i1 [[C1]], float [[E1]], float [[S0]]
; CHECK:    [[C2:%.*]] = icmp eq i32 [[IDX]], 2
; CHECK:    [[E2:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK:    [[S2:%.*]] = select i1 [[C2]], float [[E2]], float [[S1]]
; CHECK:    [[C3:%.*]] = icmp eq i32 [[IDX]], 3
; CHECK:    [[E3:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK:    [[S3:%.*]] = select i1 [[C3]], float [[E3]], float [[S2]]
; CHECK:    store float [[S3]], ptr [[DST:%.*]], align 4
;
  %idx = shl nuw nsw i32 %x, %s
  %e = extractelement <4 x float> %v, i32 %idx
  store float %e, ptr %dst, align 4
  ret void
}

define void @test_shl_var_or(i32 %x, i32 %s, <4 x float> %v, ptr %dst) {
; CHECK-LABEL: @test_shl_var_or(
; CHECK:    [[SH:%.*]] = shl nuw nsw i32 [[X:%.*]], [[S:%.*]]
; CHECK:    [[IDX:%.*]] = or i32 [[SH]], 1
; CHECK:    extractelement <4 x float> [[V:%.*]], i32 [[IDX]]
; CHECK:    [[C0:%.*]] = icmp eq i32 [[IDX]], 0
; CHECK:    [[E0:%.*]] = extractelement <4 x float> [[V]], i32 0
; CHECK:    [[S0:%.*]] = select i1 [[C0]], float [[E0]], float undef
; CHECK:    [[C1:%.*]] = icmp eq i32 [[IDX]], 1
; CHECK:    [[E1:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK:    [[S1:%.*]] = select i1 [[C1]], float [[E1]], float [[S0]]
; CHECK:    [[C2:%.*]] = icmp eq i32 [[IDX]], 2
; CHECK:    [[E2:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK:    [[S2:%.*]] = select i1 [[C2]], float [[E2]], float [[S1]]
; CHECK:    [[C3:%.*]] = icmp eq i32 [[IDX]], 3
; CHECK:    [[E3:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK:    [[S3:%.*]] = select i1 [[C3]], float [[E3]], float [[S2]]
; CHECK:    store float [[S3]], ptr [[DST:%.*]], align 4
;
  %sh = shl nuw nsw i32 %x, %s
  %idx = or i32 %sh, 1
  %e = extractelement <4 x float> %v, i32 %idx
  store float %e, ptr %dst, align 4
  ret void
}

define void @test_mul_zero(i32 %x, <4 x float> %v, ptr %dst) {
; CHECK-LABEL: @test_mul_zero(
; CHECK:    [[IDX:%.*]] = mul nsw i32 [[X:%.*]], 0
; CHECK:    extractelement <4 x float> [[V:%.*]], i32 [[IDX]]
; CHECK:    [[C0:%.*]] = icmp eq i32 [[IDX]], 0
; CHECK:    [[E0:%.*]] = extractelement <4 x float> [[V]], i32 0
; CHECK:    [[S0:%.*]] = select i1 [[C0]], float [[E0]], float undef
; CHECK:    [[C1:%.*]] = icmp eq i32 [[IDX]], 1
; CHECK:    [[E1:%.*]] = extractelement <4 x float> [[V]], i32 1
; CHECK:    [[S1:%.*]] = select i1 [[C1]], float [[E1]], float [[S0]]
; CHECK:    [[C2:%.*]] = icmp eq i32 [[IDX]], 2
; CHECK:    [[E2:%.*]] = extractelement <4 x float> [[V]], i32 2
; CHECK:    [[S2:%.*]] = select i1 [[C2]], float [[E2]], float [[S1]]
; CHECK:    [[C3:%.*]] = icmp eq i32 [[IDX]], 3
; CHECK:    [[E3:%.*]] = extractelement <4 x float> [[V]], i32 3
; CHECK:    [[S3:%.*]] = select i1 [[C3]], float [[E3]], float [[S2]]
; CHECK:    store float [[S3]], ptr [[DST:%.*]], align 4
;
  %idx = mul nsw i32 %x, 0
  %e = extractelement <4 x float> %v, i32 %idx
  store float %e, ptr %dst, align 4
  ret void
}

define void @test_mul_const_add(i32 %x, <12 x float> %v, ptr %dst) {
; CHECK-LABEL: @test_mul_const_add(
; CHECK:    [[MM:%.*]] = mul nsw i32 [[X:%.*]], 3
; CHECK:    [[IDX:%.*]] = add nsw i32 [[MM]], 1
; CHECK:    extractelement <12 x float> [[V:%.*]], i32 [[IDX]]
; CHECK:    [[C0:%.*]] = icmp eq i32 [[X]], 0
; CHECK:    [[E0:%.*]] = extractelement <12 x float> [[V]], i32 1
; CHECK:    [[S0:%.*]] = select i1 [[C0]], float [[E0]], float undef
; CHECK:    [[C1:%.*]] = icmp eq i32 [[X]], 1
; CHECK:    [[E1:%.*]] = extractelement <12 x float> [[V]], i32 4
; CHECK:    [[S1:%.*]] = select i1 [[C1]], float [[E1]], float [[S0]]
; CHECK:    [[C2:%.*]] = icmp eq i32 [[X]], 2
; CHECK:    [[E2:%.*]] = extractelement <12 x float> [[V]], i32 7
; CHECK:    [[S2:%.*]] = select i1 [[C2]], float [[E2]], float [[S1]]
; CHECK:    [[C3:%.*]] = icmp eq i32 [[X]], 3
; CHECK:    [[E3:%.*]] = extractelement <12 x float> [[V]], i32 10
; CHECK:    [[S3:%.*]] = select i1 [[C3]], float [[E3]], float [[S2]]
; CHECK:    store float [[S3]], ptr [[DST:%.*]], align 4
;
  %mm = mul nsw i32 %x, 3
  %idx = add nsw i32 %mm, 1
  %e = extractelement <12 x float> %v, i32 %idx
  store float %e, ptr %dst, align 4
  ret void
}
