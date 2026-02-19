;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: addrspacecast
; ------------------------------------------------

; Checks legalization of addrspacecast from local addrspace
; 1) for non-generic - nullify it (except for private AS)
; 2) for cast null to generic addrspace - nullify

define i32 @test_addrcast_p0(i32 addrspace(3)* %p1) {
; CHECK-LABEL: define i32 @test_addrcast_p0(
; CHECK-SAME: i32 addrspace(3)* [[P1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = addrspacecast i32 addrspace(3)* [[P1]] to i32*
; CHECK:    [[TMP2:%.*]] = load i32, i32* [[TMP1]]
; CHECK:    ret i32 [[TMP2]]
;
  %1 = addrspacecast i32 addrspace(3)* %p1 to i32 addrspace(0)*
  %2 = load i32, i32 addrspace(0)* %1
  ret i32 %2
}

define i32 @test_addrcast_p1(i32 addrspace(3)* %p1) {
; CHECK-LABEL: define i32 @test_addrcast_p1(
; CHECK-SAME: i32 addrspace(3)* [[P1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(1)* null
; CHECK:    ret i32 [[TMP1]]
;
  %1 = addrspacecast i32 addrspace(3)* %p1 to i32 addrspace(1)*
  %2 = load i32, i32 addrspace(1)* %1
  ret i32 %2
}

define i32 @test_addrcast_p2(i32 addrspace(3)* %p1) {
; CHECK-LABEL: define i32 @test_addrcast_p2(
; CHECK-SAME: i32 addrspace(3)* [[P1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(2)* null
; CHECK:    ret i32 [[TMP1]]
;
  %1 = addrspacecast i32 addrspace(3)* %p1 to i32 addrspace(2)*
  %2 = load i32, i32 addrspace(2)* %1
  ret i32 %2
}

define i32 @test_addrcast_null_p4() {
; CHECK-LABEL: define i32 @test_addrcast_null_p4() {
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(4)* null
; CHECK:    ret i32 [[TMP1]]
;
  %1 = addrspacecast i32 addrspace(3)* null to i32 addrspace(4)*
  %2 = load i32, i32 addrspace(4)* %1
  ret i32 %2
}

!igc.functions = !{!0,!1,!2,!3}

!0 = !{i32 (i32 addrspace(3)*)* @test_addrcast_p0, !10}
!1 = !{i32 (i32 addrspace(3)*)* @test_addrcast_p1, !10}
!2 = !{i32 (i32 addrspace(3)*)* @test_addrcast_p2, !10}
!3 = !{i32 ()* @test_addrcast_null_p4, !10}
!10 = !{}

