;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-vector-bitcast-opt -S < %s | FileCheck %s

; This the component-wise case

; CHECK-LABEL: define float @test_vbitcast0
; CHECK:  [[EE0:%[a-zA-Z0-9_.%-]+]] = extractelement <2 x i32> %src, i32 1
; CHECK-NEXT: [[BC0:%[a-zA-Z0-9_.%-]+]] = bitcast i32 [[EE0]] to float
; CHECK-NEXT: ret float [[BC0]]
define float @test_vbitcast0(<2 x i32> %src) {
  %1 = bitcast <2 x i32> %src to <2 x float>
  %2 = extractelement <2 x float> %1, i32 1
  ret float %2
}


