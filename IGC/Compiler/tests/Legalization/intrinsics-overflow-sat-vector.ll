;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: intrinsics
; ------------------------------------------------

; Vector *.with.overflow and *.sat intrinsics must be lowered to scalar
; instructions. Legalization runs after scalarization, so vector ALU
; instructions created here reach EmitVISA, which cannot emit them
; (e.g. vector sub asserted for SIMD32 in EmitPass::Sub).

define <2 x i32> @test_usub_v2i32(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: define <2 x i32> @test_usub_v2i32(
; CHECK-NOT:     @llvm.usub.with.overflow
; CHECK-NOT:     sub <2 x i32>
; CHECK-NOT:     icmp {{.*}} <2 x i32>
; CHECK:         [[A0:%.*]] = extractelement <2 x i32> %a, i64 0
; CHECK:         [[B0:%.*]] = extractelement <2 x i32> %b, i64 0
; CHECK:         [[S0:%.*]] = sub i32 [[A0]], [[B0]]
; CHECK:         [[O0:%.*]] = icmp ugt i32 [[S0]], [[A0]]
; CHECK:         [[R0:%.*]] = insertelement <2 x i32> poison, i32 [[S0]], i64 0
; CHECK:         [[F0:%.*]] = insertelement <2 x i1> poison, i1 [[O0]], i64 0
; CHECK:         [[A1:%.*]] = extractelement <2 x i32> %a, i64 1
; CHECK:         [[B1:%.*]] = extractelement <2 x i32> %b, i64 1
; CHECK:         [[S1:%.*]] = sub i32 [[A1]], [[B1]]
; CHECK:         [[O1:%.*]] = icmp ugt i32 [[S1]], [[A1]]
; CHECK:         [[R1:%.*]] = insertelement <2 x i32> [[R0]], i32 [[S1]], i64 1
; CHECK:         [[F1:%.*]] = insertelement <2 x i1> [[F0]], i1 [[O1]], i64 1
; CHECK:         [[Z:%.*]] = zext <2 x i1> [[F1]] to <2 x i32>
; CHECK:         [[RES:%.*]] = add <2 x i32> [[R1]], [[Z]]
; CHECK:         ret <2 x i32> [[RES]]
;
  %1 = call { <2 x i32>, <2 x i1> } @llvm.usub.with.overflow.v2i32(<2 x i32> %a, <2 x i32> %b)
  %r = extractvalue { <2 x i32>, <2 x i1> } %1, 0
  %c = extractvalue { <2 x i32>, <2 x i1> } %1, 1
  %z = zext <2 x i1> %c to <2 x i32>
  %res = add <2 x i32> %r, %z
  ret <2 x i32> %res
}

; Lane extracts must use the scalar values directly, so zext(icmp) stays
; visible to later pattern matching instead of going through insertelement.
define i32 @test_usub_v2i32_lane_extract(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: define i32 @test_usub_v2i32_lane_extract(
; CHECK:         [[A1:%.*]] = extractelement <2 x i32> %a, i64 1
; CHECK:         [[B1:%.*]] = extractelement <2 x i32> %b, i64 1
; CHECK:         [[S1:%.*]] = sub i32 [[A1]], [[B1]]
; CHECK:         [[O1:%.*]] = icmp ugt i32 [[S1]], [[A1]]
; CHECK-NOT:     extractelement <2 x i1>
; CHECK-NOT:     extractelement <2 x i32> {{%.*}}, i32 1
; CHECK:         [[Z:%.*]] = zext i1 [[O1]] to i32
; CHECK:         [[RES:%.*]] = add i32 [[S1]], [[Z]]
; CHECK:         ret i32 [[RES]]
;
  %1 = call { <2 x i32>, <2 x i1> } @llvm.usub.with.overflow.v2i32(<2 x i32> %a, <2 x i32> %b)
  %r = extractvalue { <2 x i32>, <2 x i1> } %1, 0
  %c = extractvalue { <2 x i32>, <2 x i1> } %1, 1
  %r1 = extractelement <2 x i32> %r, i32 1
  %c1 = extractelement <2 x i1> %c, i32 1
  %z = zext i1 %c1 to i32
  %res = add i32 %r1, %z
  ret i32 %res
}

define <4 x i1> @test_uadd_v4i32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: define <4 x i1> @test_uadd_v4i32(
; CHECK-NOT:     @llvm.uadd.with.overflow
; CHECK-NOT:     add <4 x i32>
; CHECK-COUNT-4: icmp ult i32
; CHECK-NOT:     icmp {{.*}} <4 x i32>
; CHECK:         ret <4 x i1>
;
  %1 = call { <4 x i32>, <4 x i1> } @llvm.uadd.with.overflow.v4i32(<4 x i32> %a, <4 x i32> %b)
  %c = extractvalue { <4 x i32>, <4 x i1> } %1, 1
  ret <4 x i1> %c
}

define <2 x i1> @test_sadd_v2i16(<2 x i16> %a, <2 x i16> %b) {
; CHECK-LABEL: define <2 x i1> @test_sadd_v2i16(
; CHECK-NOT:     @llvm.sadd.with.overflow
; CHECK-NOT:     add <2 x i16>
; CHECK-COUNT-2: icmp slt i16
; CHECK:         ret <2 x i1>
;
  %1 = call { <2 x i16>, <2 x i1> } @llvm.sadd.with.overflow.v2i16(<2 x i16> %a, <2 x i16> %b)
  %c = extractvalue { <2 x i16>, <2 x i1> } %1, 1
  ret <2 x i1> %c
}

define <2 x i1> @test_umul_v2i32(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: define <2 x i1> @test_umul_v2i32(
; CHECK-NOT:     @llvm.umul.with.overflow
; CHECK-NOT:     mul <2 x i32>
; CHECK-COUNT-2: icmp ne i32
; CHECK:         ret <2 x i1>
;
  %1 = call { <2 x i32>, <2 x i1> } @llvm.umul.with.overflow.v2i32(<2 x i32> %a, <2 x i32> %b)
  %c = extractvalue { <2 x i32>, <2 x i1> } %1, 1
  ret <2 x i1> %c
}

define { <2 x i32>, <2 x i1> } @test_usub_v2i32_freeze(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: define { <2 x i32>, <2 x i1> } @test_usub_v2i32_freeze(
; CHECK-NOT:     @llvm.usub.with.overflow
; CHECK-NOT:     sub <2 x i32>
; CHECK:         [[AGG0:%.*]] = insertvalue { <2 x i32>, <2 x i1> } poison, <2 x i32> {{%.*}}, 0
; CHECK:         [[AGG1:%.*]] = insertvalue { <2 x i32>, <2 x i1> } [[AGG0]], <2 x i1> {{%.*}}, 1
; CHECK:         [[FR:%.*]] = freeze { <2 x i32>, <2 x i1> } [[AGG1]]
; CHECK:         ret { <2 x i32>, <2 x i1> } [[FR]]
;
  %1 = call { <2 x i32>, <2 x i1> } @llvm.usub.with.overflow.v2i32(<2 x i32> %a, <2 x i32> %b)
  %fr = freeze { <2 x i32>, <2 x i1> } %1
  ret { <2 x i32>, <2 x i1> } %fr
}

define <2 x i32> @test_usub_sat_v2i32(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: define <2 x i32> @test_usub_sat_v2i32(
; CHECK-NOT:     @llvm.usub.sat
; CHECK-NOT:     @llvm.usub.with.overflow
; CHECK-NOT:     sub <2 x i32>
; CHECK-NOT:     select {{.*}} <2 x i32>
; CHECK-COUNT-2: select i1 {{%.*}}, i32 0, i32
; CHECK:         ret <2 x i32>
;
  %1 = call <2 x i32> @llvm.usub.sat.v2i32(<2 x i32> %a, <2 x i32> %b)
  ret <2 x i32> %1
}

declare { <2 x i32>, <2 x i1> } @llvm.usub.with.overflow.v2i32(<2 x i32>, <2 x i32>)
declare { <4 x i32>, <4 x i1> } @llvm.uadd.with.overflow.v4i32(<4 x i32>, <4 x i32>)
declare { <2 x i16>, <2 x i1> } @llvm.sadd.with.overflow.v2i16(<2 x i16>, <2 x i16>)
declare { <2 x i32>, <2 x i1> } @llvm.umul.with.overflow.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.usub.sat.v2i32(<2 x i32>, <2 x i32>)

!igc.functions = !{!0, !1, !2, !3, !4, !5, !7}

!0 = !{<2 x i32> (<2 x i32>, <2 x i32>)* @test_usub_v2i32, !6}
!1 = !{<4 x i1> (<4 x i32>, <4 x i32>)* @test_uadd_v4i32, !6}
!2 = !{<2 x i1> (<2 x i16>, <2 x i16>)* @test_sadd_v2i16, !6}
!3 = !{<2 x i1> (<2 x i32>, <2 x i32>)* @test_umul_v2i32, !6}
!4 = !{{ <2 x i32>, <2 x i1> } (<2 x i32>, <2 x i32>)* @test_usub_v2i32_freeze, !6}
!5 = !{<2 x i32> (<2 x i32>, <2 x i32>)* @test_usub_sat_v2i32, !6}
!6 = !{}
!7 = !{i32 (<2 x i32>, <2 x i32>)* @test_usub_v2i32_lane_extract, !6}
