;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-vector-bitcast-opt -S < %s | FileCheck %s

; This the smaller to larger element type case

; CHECK-LABEL: define i64 @test_vbitcast0
; CHECK:  [[EE0:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> %src, i32 2
; CHECK-NEXT:  [[IE0:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[EE0]], i64 0
; CHECK-NEXT:  [[EE1:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> %src, i32 3
; CHECK-NEXT:  [[IE1:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[IE0]], i32 [[EE1]], i64 1
; CHECK-NEXT: [[BC0:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32>  [[IE1]] to i64
; CHECK-NEXT: ret i64 [[BC0]]
define i64 @test_vbitcast0(<4 x i32> %src) {
  %1 = bitcast <4 x i32> %src to <2 x i64>
  %2 = extractelement <2 x i64> %1, i32 1
  ret i64 %2
}


