;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: (bitcast <3 x i1> cmp to i3) == 0 must become all(!cmp), and != 0 must
; COM: become any(cmp): GenXLegalization cannot split a bitcast from a
; COM: non-power-of-2 predicate.

define i1 @test_odd_pred_bitcast(<3 x i16> %id) {
; CHECK-LABEL: @test_odd_pred_bitcast(
; CHECK: [[CMP:%[^ ]+]] = icmp eq <3 x i16> %id, zeroinitializer
; CHECK-NOT: bitcast
; CHECK: [[ALL:%[^ ]+]] = call i1 @llvm.genx.all.v3i1(<3 x i1> [[CMP]])
; CHECK: ret i1 [[ALL]]
  %cmp = icmp ne <3 x i16> %id, zeroinitializer
  %bc = bitcast <3 x i1> %cmp to i3
  %res = icmp eq i3 %bc, 0
  ret i1 %res
}

define i1 @test_odd_pred_bitcast_any(<3 x i16> %id) {
; CHECK-LABEL: @test_odd_pred_bitcast_any(
; CHECK: [[CMP:%[^ ]+]] = icmp ne <3 x i16> %id, zeroinitializer
; CHECK-NOT: bitcast
; CHECK: [[ANY:%[^ ]+]] = call i1 @llvm.genx.any.v3i1(<3 x i1> [[CMP]])
; CHECK: ret i1 [[ANY]]
  %cmp = icmp ne <3 x i16> %id, zeroinitializer
  %bc = bitcast <3 x i1> %cmp to i3
  %res = icmp ne i3 %bc, 0
  ret i1 %res
}
