;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --platformdg2 --igc-legalization -S %s -o %t.dg2.ll
; RUN: igc_opt --platformCls --igc-legalization -S %s -o %t.cls.ll
; RUN: FileCheck %s --input-file=%t.dg2.ll -check-prefix=X32
; RUN: FileCheck %s --input-file=%t.cls.ll -check-prefix=X64

define void @test_uadd_sat_i64(i64 %a, i64 %b) {
entry:
  %0 = call i64 @llvm.uadd.sat.i64(i64 %a, i64 %b)
  ret void
}

; CHECK-LABEL:  X32: define void @test_uadd_sat_i64
; CHECK:        X32: add i64 %a, %b
; CHECK:        X32: icmp ult i64 %0, %a
; CHECK:        X32: select i1 %1, i64 -1, i64 %0

; CHECK-LABEL:  X64: define void @test_uadd_sat_i64
; CHECK:        X64: call i64 @llvm.uadd.sat.i64(i64 %a, i64 %b)

define void @test_sadd_sat_i64(i64 %a, i64 %b) {
entry:
  %0 = call i64 @llvm.sadd.sat.i64(i64 %a, i64 %b)
  ret void
}

; CHECK-LABEL:  X32: define void @test_sadd_sat_i64
; CHECK:        X32: bitcast i64 %a to i64
; CHECK:        X32: bitcast i64 %b to i64
; CHECK:        X32: add i64 %0, %1
; CHECK:        X32: xor i64 %0, %1
; CHECK:        X32: xor i64 %2, %0
; CHECK:        X32: xor i64 %3, -1
; CHECK:        X32: and i64 %5, %4
; CHECK:        X32: icmp slt i64 %6, 0
; CHECK:        X32: icmp slt i64 0, %b
; CHECK:        X32: select i1 %8, i64 9223372036854775807, i64 -9223372036854775808
; CHECK:        X32: select i1 %7, i64 %9, i64 %2

; CHECK-LABEL:  X64: define void @test_sadd_sat_i64
; CHECK:        X64: call i64 @llvm.sadd.sat.i64(i64 %a, i64 %b)

define void @test_uadd_sat_i32(i32 %a, i32 %b) {
entry:
  %0 = call i32 @llvm.uadd.sat.i32(i32 %a, i32 %b)
  ret void
}

; CHECK-LABEL:  X32: define void @test_uadd_sat_i32
; CHECK:        X32: call i32 @llvm.uadd.sat.i32(i32 %a, i32 %b)

; CHECK-LABEL:  X64: define void @test_uadd_sat_i32
; CHECK:        X64: call i32 @llvm.uadd.sat.i32(i32 %a, i32 %b)

define void @test_sadd_sat_i32(i32 %a, i32 %b) {
entry:
  %0 = call i32 @llvm.sadd.sat.i32(i32 %a, i32 %b)
  ret void
}

; CHECK-LABEL:  X32: define void @test_sadd_sat_i32
; CHECK:        X32: call i32 @llvm.sadd.sat.i32(i32 %a, i32 %b)

; CHECK-LABEL:  X64: define void @test_sadd_sat_i32
; CHECK:        X64: call i32 @llvm.sadd.sat.i32(i32 %a, i32 %b)

declare i64 @llvm.uadd.sat.i64(i64, i64)
declare i64 @llvm.sadd.sat.i64(i64, i64)
declare i32 @llvm.uadd.sat.i32(i32, i32)
declare i32 @llvm.sadd.sat.i32(i32, i32)

!igc.functions = !{!0, !1, !2}
!0 = !{void (i64, i64)* @test_uadd_sat_i64, !{}}
!1 = !{void (i64, i64)* @test_sadd_sat_i64, !{}}
!2 = !{void (i32, i32)* @test_uadd_sat_i32, !{}}
!3 = !{void (i32, i32)* @test_sadd_sat_i32, !{}}

