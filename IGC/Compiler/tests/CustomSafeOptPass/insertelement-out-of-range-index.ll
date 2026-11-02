;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
;
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -S < %s | FileCheck %s
;
; Out-of-range insertelement index must not crash the pass.

define void @test(ptr %p, i32 %x) {
; CHECK-LABEL: @test(
; CHECK: insertelement <4 x i32> %v, i32 %x, i32 5
  %v = load <4 x i32>, ptr %p
  %ie = insertelement <4 x i32> %v, i32 %x, i32 5
  store <4 x i32> %ie, ptr %p
  ret void
}
