;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-vector-bitcast-opt -S < %s | FileCheck %s

; This the larger to smaller element type case

; CHECK-LABEL: define i32 @test_vbitcast0
; CHECK:  [[EE0:%[a-zA-Z0-9_.%-]+]] = extractelement <2 x i64> %src, i32 1
; CHECK-NEXT: [[BC0:%[a-zA-Z0-9_.%-]+]] = bitcast i64 [[EE0]] to <2 x i32>
; CHECK-NEXT:  [[EE1:%[a-zA-Z0-9_.%-]+]] = extractelement <2 x i32> [[BC0]], i32 1
; CHECK-NEXT: ret i32 [[EE1]]
define i32 @test_vbitcast0(<2 x i64> %src) {
  %1 = bitcast <2 x i64> %src to <4 x i32>
  %2 = extractelement <4 x i32> %1, i32 3
  ret i32 %2
}


