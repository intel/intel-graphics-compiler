;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Verify that an [N x i32] alloca accessed through <2 x half> loads/stores is
; promoted to a vector register.  The SOA checker should allow same-total-size
; reinterpretations (e.g. i32 ↔ <2 x half>, both 32 bits).

; --- Positive: load <2 x half> from [3 x i32] ---
define void @test_load_v2half_from_i32_array(ptr %output) {
; CHECK-LABEL: @test_load_v2half_from_i32_array(
; CHECK: alloca <3 x i32>
; The general path bitcasts through integer types: i32 → <2 x i16> → <2 x half>
; CHECK: bitcast i32 %{{.*}} to <2 x i16>
; CHECK: bitcast <2 x i16> %{{.*}} to <2 x half>
; CHECK: ret void
  %private = alloca [3 x i32], align 4, !uniform !4
  %gep = getelementptr [3 x i32], ptr %private, i32 0, i32 1
  %val = load <2 x half>, ptr %gep, align 4
  store <2 x half> %val, ptr %output, align 4
  ret void
}

; --- Positive: store <2 x half> into [3 x i32] ---
define void @test_store_v2half_to_i32_array(<2 x half> %v) {
; CHECK-LABEL: @test_store_v2half_to_i32_array(
; CHECK: alloca <3 x i32>
; The general path bitcasts through integer types: <2 x half> → half → i16 → <2 x i16> → i32
; CHECK: bitcast <2 x i16> %{{.*}} to i32
; CHECK: insertelement <3 x i32>
; CHECK: ret void
  %private = alloca [3 x i32], align 4, !uniform !4
  %gep = getelementptr [3 x i32], ptr %private, i32 0, i32 2
  store <2 x half> %v, ptr %gep, align 4
  ret void
}

; --- Positive: two sequential stores of <2 x half> ---
define void @test_sequential_v2half_stores(<2 x half> %v0, <2 x half> %v1) {
; CHECK-LABEL: @test_sequential_v2half_stores(
; CHECK: alloca <3 x i32>
; CHECK: bitcast <2 x i16> %{{.*}} to i32
; CHECK: insertelement <3 x i32> %{{.*}}, i32 %{{.*}}, i32 0
; CHECK: bitcast <2 x i16> %{{.*}} to i32
; CHECK: insertelement <3 x i32> %{{.*}}, i32 %{{.*}}, i32 1
; CHECK: ret void
  %private = alloca [3 x i32], align 4, !uniform !4
  %gep0 = getelementptr [3 x i32], ptr %private, i32 0, i32 0
  store <2 x half> %v0, ptr %gep0, align 4
  %gep1 = getelementptr [3 x i32], ptr %private, i32 0, i32 1
  store <2 x half> %v1, ptr %gep1, align 4
  ret void
}

; --- Negative: <3 x i16> is 48 bits, i32 is 32 bits — size mismatch ---
define void @negative_size_mismatch(ptr %output) {
; CHECK-LABEL: @negative_size_mismatch(
; CHECK: alloca [3 x i32]
; CHECK-NOT: alloca <3 x i32>
; CHECK: ret void
  %private = alloca [3 x i32], align 4, !uniform !4
  %gep = getelementptr [3 x i32], ptr %private, i32 0, i32 0
  %val = load <3 x i16>, ptr %gep, align 2
  store <3 x i16> %val, ptr %output, align 2
  ret void
}

; --- Negative: aggregate user { i16, i16 } — same bits but aggregate ---
define void @negative_aggregate_user(ptr %output) {
; CHECK-LABEL: @negative_aggregate_user(
; CHECK: alloca [2 x i32]
; CHECK-NOT: alloca <2 x i32>
; CHECK: ret void
  %private = alloca [2 x i32], align 4, !uniform !4
  %gep = getelementptr [2 x i32], ptr %private, i32 0, i32 0
  %val = load { i16, i16 }, ptr %gep, align 2
  store { i16, i16 } %val, ptr %output, align 2
  ret void
}

!igc.functions = !{!0, !5, !6, !7, !8}

!0 = !{ptr @test_load_v2half_from_i32_array, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
!5 = !{ptr @test_store_v2half_to_i32_array, !1}
!6 = !{ptr @test_sequential_v2half_stores, !1}
!7 = !{ptr @negative_size_mismatch, !1}
!8 = !{ptr @negative_aggregate_user, !1}
