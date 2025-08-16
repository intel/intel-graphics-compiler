;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; This test verifies that the pass can handle storing <3 x i16> (48 bits total with i16 lanes) into <2 x i32> (64 bits total with i32 lanes).

define void @test(<3 x i16> %some_value) {
; CHECK-LABEL: @test(
; Promoted private is a vector <2 x i32> so 64 bits total with 32 bit lanes.
; CHECK: [[ALLOCA:%.*]] = alloca <2 x i32>

; Source vector is packed to i48, then zero-extended to i64, and split.
; CHECK: [[I48:%.*]] = bitcast <3 x i16> [[SOME_VALUE:%.*]] to i48
; CHECK: [[I64:%.*]] = zext i48 [[I48]] to i64
; CHECK: [[SPLIT:%.*]] = bitcast i64 [[I64]] to <2 x i32>

; Overwrite lanes by inserting into a loaded vector.
; CHECK: [[WHOLE:%.*]] = load <2 x i32>, ptr [[ALLOCA]]
; CHECK: insertelement <2 x i32> [[WHOLE]], i32 {{%.*}}, i32 {{.*}}
; CHECK: store <2 x i32> {{%.*}}, ptr [[ALLOCA]]
; CHECK: ret void

  %private = alloca [2 x i32], align 4, !uniform !4
  %gep = getelementptr <3 x i16>, ptr %private, i32 0
  store <3 x i16> %some_value, ptr %gep, align 2
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
