;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; This test verifies that the pass can handle loading <3 x i16> (48 bits total with i16 lanes) from <2 x i32> (64 bits total with i32 lanes).

define void @test(ptr %output) {
; CHECK-LABEL: @test(
; Promoted private is a vector <2 x i32> so 64 bits total with 32 bit lanes.
; CHECK: [[ALLOCA:%.*]] = alloca <2 x i32>

; Load and build the destination vector seeded with poison.
; CHECK: [[WHOLE:%.*]] = load <2 x i32>, ptr [[ALLOCA]]
; CHECK: [[PACK0:%.*]] = insertelement <2 x i32> poison, i32 {{%.*}}, i32 0
; Additional checks for extracting/inserting lanes into the destination vector are skipped in this test.

; Cast into a big 64 bit integer, trunc to the 48 bits needed, and cast into a <3 x i16> vector.
; CHECK: [[I64:%.*]] = bitcast <2 x i32> {{%.*}} to i64
; CHECK: [[I48:%.*]] = trunc i64 [[I64]] to i48
; CHECK: bitcast i48 [[I48]] to <3 x i16>
; CHECK: ret void

  %private = alloca [2 x i32], align 4, !uniform !4
  %gep = getelementptr <3 x i16>, ptr %private, i32 0
  %loaded_v3i16 = load <3 x i16>, ptr %gep, align 2
  store <3 x i16> %loaded_v3i16, ptr %output, align 2
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
