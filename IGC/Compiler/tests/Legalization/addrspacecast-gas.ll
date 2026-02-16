;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; Default test run w/o AddImplicitArgs is for sanity reasons(so module won't break)
; RUN: igc_opt -opaque-pointers -igc-legalization -S -dce -disable-output < %s
;
; AddImplicitArgs is expected to be present in pipeline
; RUN: igc_opt -opaque-pointers -igc-add-implicit-args -igc-legalization -S -dce < %s | FileCheck %s
;
; ------------------------------------------------
; Legalization: addrspacecast
; ------------------------------------------------

; Checks legalization of addrspacecast from local addrspace
; Non-null cast to GAS, will try to get the argument from MD

define i32 @test_addrcast_slm_p4(ptr addrspace(3) %p1) {
; CHECK-LABEL: define i32 @test_addrcast_slm_p4(
; CHECK-SAME: ptr addrspace(3) [[P1:%.*]], ptr addrspace(1) [[LOCALMEMSTATELESSWINDOWSTARTADDR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = ptrtoint ptr addrspace(3) [[P1]] to i16
; CHECK:    [[TMP2:%.*]] = zext i16 [[TMP1]] to i64
; CHECK:    [[TMP3:%.*]] = ptrtoint ptr addrspace(1) [[LOCALMEMSTATELESSWINDOWSTARTADDR]] to i64
; CHECK:    [[TMP4:%.*]] = add i64 [[TMP3]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = inttoptr i64 [[TMP4]] to ptr addrspace(4)
; CHECK:    [[TMP6:%.*]] = load i32, ptr addrspace(4) [[TMP5]], align 4
; CHECK:    ret i32 [[TMP6]]
;
  %1 = addrspacecast ptr addrspace(3) %p1 to ptr addrspace(4)
  %2 = load i32, ptr addrspace(4) %1, align 4
  ret i32 %2
}

!igc.functions = !{!0}

!0 = !{ptr @test_addrcast_slm_p4, !1}
!1 = !{!2, !3, !4}
!2 = !{!"function_type", i32 1}
!3 = !{!"arg_desc"}
!4 = !{!"implicit_arg_desc", !5}
!5 = !{i32 43}
