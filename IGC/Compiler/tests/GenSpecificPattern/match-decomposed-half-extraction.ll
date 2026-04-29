;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

; REQUIRES: regkeys, llvm-16-plus

; RUN: igc_opt --opaque-pointers -platformPtl -igc-gen-specific-pattern -regkey EnableMatchDecomposedHalfExtract=1 -S %s | FileCheck %s

; Test: Low half extraction (index 0)
; (bitcast (trunc i32 %val to i16) to half) => extractelement (bitcast %val to <2 x half>), 0

define half @test_low_half(i32 %val) {
; CHECK-LABEL: @test_low_half
; CHECK: [[BC:%.*]] = bitcast i32 %val to <2 x half>
; CHECK: [[EE:%.*]] = extractelement <2 x half> [[BC]], i32 0
; CHECK: ret half [[EE]]
  %trunc = trunc i32 %val to i16
  %half = bitcast i16 %trunc to half
  ret half %half
}

; Test: High half extraction (index 1)
; (bitcast (trunc (lshr i32 %val, 16) to i16) to half) => extractelement (bitcast %val to <2 x half>), 1

define half @test_high_half(i32 %val) {
; CHECK-LABEL: @test_high_half
; CHECK: [[BC:%.*]] = bitcast i32 %val to <2 x half>
; CHECK: [[EE:%.*]] = extractelement <2 x half> [[BC]], i32 1
; CHECK: ret half [[EE]]
  %shr = lshr i32 %val, 16
  %trunc = trunc i32 %shr to i16
  %half = bitcast i16 %trunc to half
  ret half %half
}

; Test: Full pattern from a load (as seen in LLVM 16 output)

define void @test_full_pattern(<2 x i32> addrspace(3)* %ptr, half addrspace(1)* %out) {
; CHECK-LABEL: @test_full_pattern
; CHECK-NOT: lshr
; CHECK-NOT: trunc
; CHECK: bitcast i32 %{{.*}} to <2 x half>
; CHECK: extractelement <2 x half>
; CHECK: extractelement <2 x half>
  %load = load <2 x i32>, <2 x i32> addrspace(3)* %ptr, align 8
  %e0 = extractelement <2 x i32> %load, i32 0
  %e1 = extractelement <2 x i32> %load, i32 1
  %trunc0 = trunc i32 %e0 to i16
  %h0 = bitcast i16 %trunc0 to half
  %shr0 = lshr i32 %e0, 16
  %trunc1 = trunc i32 %shr0 to i16
  %h1 = bitcast i16 %trunc1 to half
  store half %h0, half addrspace(1)* %out
  %out1 = getelementptr half, half addrspace(1)* %out, i32 1
  store half %h1, half addrspace(1)* %out1
  ret void
}

; Negative test: shift amount is not 16 — should NOT transform

define half @test_negative_shift(i32 %val) {
; CHECK-LABEL: @test_negative_shift
; CHECK: lshr i32 %val, 8
; CHECK: trunc
; CHECK: bitcast i16
  %shr = lshr i32 %val, 8
  %trunc = trunc i32 %shr to i16
  %half = bitcast i16 %trunc to half
  ret half %half
}

; Negative test: source is not i32 — should NOT transform

define half @test_negative_i64(i64 %val) {
; CHECK-LABEL: @test_negative_i64
; CHECK: trunc i64
; CHECK: bitcast i16
  %trunc = trunc i64 %val to i16
  %half = bitcast i16 %trunc to half
  ret half %half
}

; Negative test: bitcast destination is not half — should NOT transform

define i16 @test_negative_dest_type(i32 %val) {
; CHECK-LABEL: @test_negative_dest_type
; CHECK: trunc i32 %val to i16
; CHECK-NOT: bitcast {{.*}} <2 x half>
  %trunc = trunc i32 %val to i16
  ret i16 %trunc
}
