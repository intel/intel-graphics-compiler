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

; A freeze of the whole *.with.overflow struct must keep a valid
; operand after lowering. The intrinsic used to be erased out from under it.

define i64 @test_umul_freeze(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_umul_freeze(
; CHECK-NOT:     @llvm.umul.with.overflow
; CHECK:         [[AGG0:%.*]] = insertvalue { i64, i1 } poison, i64 [[UMUL_RESULT:%.*]], 0
; CHECK:         [[AGG1:%.*]] = insertvalue { i64, i1 } [[AGG0]], i1 [[IS_OVERFLOW:%.*]], 1
; CHECK:         [[FR:%.*]] = freeze { i64, i1 } [[AGG1]]
; CHECK:         [[VAL:%.*]] = extractvalue { i64, i1 } [[FR]], 0
; CHECK:         ret i64 [[VAL]]
;
  %1 = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %s1, i64 %s2)
  %fr = freeze { i64, i1 } %1
  %val = extractvalue { i64, i1 } %fr, 0
  ret i64 %val
}

; Mixed direct extracts and a whole-struct user.

define i64 @test_uadd_freeze_and_extract(i64 %s1, i64 %s2) {
; CHECK-LABEL: define i64 @test_uadd_freeze_and_extract(
; CHECK-NOT:     @llvm.uadd.with.overflow
; CHECK:         [[RES:%.*]] = add i64 {{.*}}, {{.*}}
; CHECK:         [[ISOV:%.*]] = icmp ult i64 [[RES]], {{.*}}
; CHECK:         [[AGG0:%.*]] = insertvalue { i64, i1 } poison, i64 [[RES]], 0
; CHECK:         [[AGG1:%.*]] = insertvalue { i64, i1 } [[AGG0]], i1 [[ISOV]], 1
; CHECK:         [[FR:%.*]] = freeze { i64, i1 } [[AGG1]]
; CHECK:         [[FROV:%.*]] = extractvalue { i64, i1 } [[FR]], 1
; CHECK:         [[SEL:%.*]] = select i1 [[FROV]], i64 [[RES]], i64 144
; CHECK:         ret i64 [[SEL]]
;
  %1 = call { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %s1, i64 %s2)
  %val = extractvalue { i64, i1 } %1, 0
  %fr = freeze { i64, i1 } %1
  %frov = extractvalue { i64, i1 } %fr, 1
  %sel = select i1 %frov, i64 %val, i64 144
  ret i64 %sel
}

declare { i64, i1 } @llvm.umul.with.overflow.i64(i64 %a, i64 %b)
declare { i64, i1 } @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)

!igc.functions = !{!0, !1}

!0 = !{i64 (i64, i64)* @test_umul_freeze, !2}
!1 = !{i64 (i64, i64)* @test_uadd_freeze_and_extract, !2}
!2 = !{}
