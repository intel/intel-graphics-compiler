;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------

declare <4 x i64> @llvm.smin.v4i64(<4 x i64>, <4 x i64>)
declare <4 x i64> @llvm.smax.v4i64(<4 x i64>, <4 x i64>)
declare <4 x i64> @llvm.umin.v4i64(<4 x i64>, <4 x i64>)
declare <4 x i64> @llvm.umax.v4i64(<4 x i64>, <4 x i64>)

define spir_func void @test_smin(<4 x i64> %a, <4 x i64> %b, <4 x i64>* %c)
{
; CHECK-LABEL: @test_smin(
; CHECK: [[B0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 0
; CHECK: [[B1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 1
; CHECK: [[B2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 2
; CHECK: [[B3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 3

; CHECK: [[A0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 0
; CHECK: [[A1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 1
; CHECK: [[A2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 2
; CHECK: [[A3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 3

; CHECK: [[R0:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smin.i64(i64 [[A0]], i64 [[B0]])
; CHECK: [[R1:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smin.i64(i64 [[A1]], i64 [[B1]])
; CHECK: [[R2:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smin.i64(i64 [[A2]], i64 [[B2]])
; CHECK: [[R3:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smin.i64(i64 [[A3]], i64 [[B3]])

; CHECK: [[V0:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> undef, i64 [[R0]], i32 0
; CHECK: [[V1:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V0]], i64 [[R1]], i32 1
; CHECK: [[V2:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V1]], i64 [[R2]], i32 2
; CHECK: [[V3:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V2]], i64 [[R3]], i32 3

; CHECK: store <4 x i64> [[V3]], <4 x i64>* %c
; CHECK: ret void
  %smin = call <4 x i64> @llvm.smin.v4i64(<4 x i64> %a, <4 x i64> %b)
  store <4 x i64> %smin, <4 x i64>* %c
  ret void
}

define spir_func void @test_smax(<4 x i64> %a, <4 x i64> %b, <4 x i64>* %c) {
; CHECK-LABEL: @test_smax(
; CHECK: [[B0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 0
; CHECK: [[B1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 1
; CHECK: [[B2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 2
; CHECK: [[B3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 3

; CHECK: [[A0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 0
; CHECK: [[A1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 1
; CHECK: [[A2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 2
; CHECK: [[A3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 3

; CHECK: [[R0:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smax.i64(i64 [[A0]], i64 [[B0]])
; CHECK: [[R1:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smax.i64(i64 [[A1]], i64 [[B1]])
; CHECK: [[R2:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smax.i64(i64 [[A2]], i64 [[B2]])
; CHECK: [[R3:%[0-9a-zA-Z_.]+]] = call i64 @llvm.smax.i64(i64 [[A3]], i64 [[B3]])

; CHECK: [[V0:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> undef, i64 [[R0]], i32 0
; CHECK: [[V1:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V0]], i64 [[R1]], i32 1
; CHECK: [[V2:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V1]], i64 [[R2]], i32 2
; CHECK: [[V3:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V2]], i64 [[R3]], i32 3

; CHECK: store <4 x i64> [[V3]], <4 x i64>* %c
; CHECK: ret void

  %smax = call <4 x i64> @llvm.smax.v4i64(<4 x i64> %a, <4 x i64> %b)
  store <4 x i64> %smax, <4 x i64>* %c
  ret void
}

define spir_func void @test_umin(<4 x i64> %a, <4 x i64> %b, <4 x i64>* %c) {
; CHECK-LABEL: @test_umin(
; CHECK: [[B0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 0
; CHECK: [[B1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 1
; CHECK: [[B2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 2
; CHECK: [[B3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 3

; CHECK: [[A0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 0
; CHECK: [[A1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 1
; CHECK: [[A2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 2
; CHECK: [[A3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 3

; CHECK: [[R0:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umin.i64(i64 [[A0]], i64 [[B0]])
; CHECK: [[R1:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umin.i64(i64 [[A1]], i64 [[B1]])
; CHECK: [[R2:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umin.i64(i64 [[A2]], i64 [[B2]])
; CHECK: [[R3:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umin.i64(i64 [[A3]], i64 [[B3]])

; CHECK: [[V0:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> undef, i64 [[R0]], i32 0
; CHECK: [[V1:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V0]], i64 [[R1]], i32 1
; CHECK: [[V2:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V1]], i64 [[R2]], i32 2
; CHECK: [[V3:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V2]], i64 [[R3]], i32 3

; CHECK: store <4 x i64> [[V3]], <4 x i64>* %c
; CHECK: ret void

  %umin = call <4 x i64> @llvm.umin.v4i64(<4 x i64> %a, <4 x i64> %b)
  store <4 x i64> %umin, <4 x i64>* %c
  ret void
}

define spir_func void @test_umax_with_usage(<4 x i64> %a, <4 x i64> %b, <4 x i64> %c, <4 x i64>* %d, <4 x i64>* %e) {
; CHECK-LABEL: @test_umax_with_usage(
; CHECK: [[C0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %c, i32 0
; CHECK: [[C1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %c, i32 1
; CHECK: [[C2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %c, i32 2
; CHECK: [[C3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %c, i32 3

; CHECK: [[B0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 0
; CHECK: [[B1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 1
; CHECK: [[B2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 2
; CHECK: [[B3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %b, i32 3

; CHECK: [[A0:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 0
; CHECK: [[A1:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 1
; CHECK: [[A2:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 2
; CHECK: [[A3:%[0-9a-zA-Z_.]+]] = extractelement <4 x i64> %a, i32 3

; CHECK: [[R0:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umax.i64(i64 [[A0]], i64 [[B0]])
; CHECK: [[R1:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umax.i64(i64 [[A1]], i64 [[B1]])
; CHECK: [[R2:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umax.i64(i64 [[A2]], i64 [[B2]])
; CHECK: [[R3:%[0-9a-zA-Z_.]+]] = call i64 @llvm.umax.i64(i64 [[A3]], i64 [[B3]])

; CHECK: [[V0:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> undef, i64 [[R0]], i32 0
; CHECK: [[V1:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V0]], i64 [[R1]], i32 1
; CHECK: [[V2:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V1]], i64 [[R2]], i32 2
; CHECK: [[V3:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[V2]], i64 [[R3]], i32 3

; CHECK: [[Q0:%[0-9a-zA-Z_.]+]] = add i64 [[R0]], [[C0]]
; CHECK: [[Q1:%[0-9a-zA-Z_.]+]] = add i64 [[R1]], [[C1]]
; CHECK: [[Q2:%[0-9a-zA-Z_.]+]] = add i64 [[R2]], [[C2]]
; CHECK: [[Q3:%[0-9a-zA-Z_.]+]] = add i64 [[R3]], [[C3]]

; CHECK: [[X0:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> undef, i64 [[Q0]], i32 0
; CHECK: [[X1:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[X0]], i64 [[Q1]], i32 1
; CHECK: [[X2:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[X1]], i64 [[Q2]], i32 2
; CHECK: [[X3:%[0-9a-zA-Z_.]+]] = insertelement <4 x i64> [[X2]], i64 [[Q3]], i32 3

; CHECK: store <4 x i64> [[V3]], <4 x i64>* %d
; CHECK: store <4 x i64> [[X3]], <4 x i64>* %e
; CHECK: ret void

  %umax = call <4 x i64> @llvm.umax.v4i64(<4 x i64> %a, <4 x i64> %b)
  %add = add <4 x i64> %umax, %c
  store <4 x i64> %umax, <4 x i64>* %d
  store <4 x i64> %add, <4 x i64>* %e
  ret void
}
