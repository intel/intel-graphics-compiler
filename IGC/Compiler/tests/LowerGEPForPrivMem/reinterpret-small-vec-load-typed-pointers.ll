;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Typed-pointer variant: verify that bitcast from i32* to <2 x half>* still
; allows the alloca to be promoted when total store sizes match (32 == 32).

; --- Positive: bitcast i32* → <2 x half>*, load ---
define void @test_bitcast_i32ptr_to_v2half_load(<2 x half>* %output) {
; CHECK-LABEL: @test_bitcast_i32ptr_to_v2half_load(
; CHECK: alloca <3 x i32>
; The general path bitcasts through integer types: i32 → <2 x i16> → <2 x half>
; CHECK: bitcast i32 %{{.*}} to <2 x i16>
; CHECK: bitcast <2 x i16> %{{.*}} to <2 x half>
; CHECK: ret void
  %private = alloca [3 x i32], align 4, !uniform !4
  %gep = getelementptr [3 x i32], [3 x i32]* %private, i32 0, i32 1
  %bc = bitcast i32* %gep to <2 x half>*
  %val = load <2 x half>, <2 x half>* %bc, align 4
  store <2 x half> %val, <2 x half>* %output, align 4
  ret void
}

; --- Positive: bitcast i32* → <2 x half>*, store ---
define void @test_bitcast_i32ptr_to_v2half_store(<2 x half> %v) {
; CHECK-LABEL: @test_bitcast_i32ptr_to_v2half_store(
; CHECK: alloca <3 x i32>
; The general path bitcasts through integer types: <2 x half> → half → i16 → <2 x i16> → i32
; CHECK: bitcast <2 x i16> %{{.*}} to i32
; CHECK: ret void
  %private = alloca [3 x i32], align 4, !uniform !4
  %gep = getelementptr [3 x i32], [3 x i32]* %private, i32 0, i32 2
  %bc = bitcast i32* %gep to <2 x half>*
  store <2 x half> %v, <2 x half>* %bc, align 4
  ret void
}

!igc.functions = !{!0, !5}

!0 = !{void (<2 x half>*)* @test_bitcast_i32ptr_to_v2half_load, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
!5 = !{void (<2 x half>)* @test_bitcast_i32ptr_to_v2half_store, !1}
