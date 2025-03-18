;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Default test run w/o AddImplicitArgs is for sanity reasons(so module won't break)
; RUN: igc_opt --typed-pointers -igc-legalization -S -dce -disable-output < %s
;
; AddImplicitArgs is expected to be present in pipeline
; RUN: igc_opt --typed-pointers -igc-add-implicit-args -igc-legalization -S -dce < %s | FileCheck %s
;
; ------------------------------------------------
; Legalization: addrspacecast
; ------------------------------------------------

; Checks legalization of addrspacecast from local addrspace
; Non-null cast to GAS, will try to get the argument from MD

define i32 @test_addrcast_slm_p4(i32 addrspace(3)* %p1) {
; CHECK-LABEL: define i32 @test_addrcast_slm_p4(
; CHECK-SAME: i32 addrspace(3)* [[P1:%.*]], i8 addrspace(1)* [[LOCALMEMSTATELESSWINDOWSTARTADDR:%.*]]) {
; CHECK:    [[TMP1:%.*]] = ptrtoint i32 addrspace(3)* [[P1]] to i16
; CHECK:    [[TMP2:%.*]] = zext i16 [[TMP1]] to i64
; CHECK:    [[TMP3:%.*]] = ptrtoint i8 addrspace(1)* [[LOCALMEMSTATELESSWINDOWSTARTADDR]] to i64
; CHECK:    [[TMP4:%.*]] = add i64 [[TMP3]], [[TMP2]]
; CHECK:    [[TMP5:%.*]] = inttoptr i64 [[TMP4]] to i32 addrspace(4)*
; CHECK:    [[TMP6:%.*]] = load i32, i32 addrspace(4)* [[TMP5]]
; CHECK:    ret i32 [[TMP6]]
;
  %1 = addrspacecast i32 addrspace(3)* %p1 to i32 addrspace(4)*
  %2 = load i32, i32 addrspace(4)* %1
  ret i32 %2
}

!igc.functions = !{!4}

!4 = !{i32 (i32 addrspace(3)*)* @test_addrcast_slm_p4, !11}
!11 = !{!12, !13, !14}
!12 =  !{ !"function_type", i32 1}
!13 =  !{ !"arg_desc"}
!14 =  !{ !"implicit_arg_desc",  !15}
!15 =  !{i32 49}
