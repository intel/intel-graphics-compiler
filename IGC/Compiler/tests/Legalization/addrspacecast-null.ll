;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -opaque-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: addrspacecast
; ------------------------------------------------

; Checks legalization of addrspacecast from local addrspace
; 1) for non-generic - nullify it (except for private AS)
; 2) for cast null to generic addrspace - nullify

define i32 @test_addrcast_p0(ptr addrspace(3) %p1) {
; CHECK-LABEL: define i32 @test_addrcast_p0(
; CHECK-SAME: ptr addrspace(3) [[P1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = addrspacecast ptr addrspace(3) [[P1]] to ptr
; CHECK:    [[TMP2:%.*]] = load i32, ptr [[TMP1]], align 4
; CHECK:    ret i32 [[TMP2]]
;
  %1 = addrspacecast ptr addrspace(3) %p1 to ptr
  %2 = load i32, ptr %1, align 4
  ret i32 %2
}

define i32 @test_addrcast_p1(ptr addrspace(3) %p1) {
; CHECK-LABEL: define i32 @test_addrcast_p1(
; CHECK-SAME: ptr addrspace(3) [[P1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = load i32, ptr addrspace(1) null, align 4
; CHECK:    ret i32 [[TMP1]]
;
  %1 = addrspacecast ptr addrspace(3) %p1 to ptr addrspace(1)
  %2 = load i32, ptr addrspace(1) %1, align 4
  ret i32 %2
}

define i32 @test_addrcast_p2(ptr addrspace(3) %p1) {
; CHECK-LABEL: define i32 @test_addrcast_p2(
; CHECK-SAME: ptr addrspace(3) [[P1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = load i32, ptr addrspace(2) null, align 4
; CHECK:    ret i32 [[TMP1]]
;
  %1 = addrspacecast ptr addrspace(3) %p1 to ptr addrspace(2)
  %2 = load i32, ptr addrspace(2) %1, align 4
  ret i32 %2
}

define i32 @test_addrcast_null_p4() {
; CHECK-LABEL: define i32 @test_addrcast_null_p4() {
; CHECK:    [[TMP1:%.*]] = load i32, ptr addrspace(4) null, align 4
; CHECK:    ret i32 [[TMP1]]
;
  %1 = addrspacecast ptr addrspace(3) null to ptr addrspace(4)
  %2 = load i32, ptr addrspace(4) %1, align 4
  ret i32 %2
}

!igc.functions = !{!0, !2, !3, !4}

!0 = !{ptr @test_addrcast_p0, !1}
!1 = !{}
!2 = !{ptr @test_addrcast_p1, !1}
!3 = !{ptr @test_addrcast_p2, !1}
!4 = !{ptr @test_addrcast_null_p4, !1}
