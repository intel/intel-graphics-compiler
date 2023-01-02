;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -dx10 -regkey EnableGenUpdateCB=1 -GenUpdateCB -dce -S < %s | FileCheck %s
;
; Requires a fix for windows
; COM: FileCheck %s --input-file=OCL_gencb.ll -check-prefix=GENCB
; ------------------------------------------------
; GenUpdateCB: Load
; ------------------------------------------------

; This checks are for separate CB module
; GENCB-LABEL: @CBEntry(
; GENCB-NEXT:  entry:
; GENCB-NEXT:    [[TMP0:%.*]] = alloca float
; GENCB-NEXT:    [[TMP1:%.*]] = load float, float addrspace(65538)* null
; GENCB-NEXT:    [[TMP2:%.*]] = fadd float [[TMP1]], [[TMP1]]
; GENCB-NEXT:    [[TMP3:%.*]] = fadd float [[TMP2]], 1.300000e+01
; GENCB-NEXT:    store float [[TMP3]], float* [[TMP0]]
; GENCB-NEXT:    ret void

; Current module checks

define void @test_update_cb(float %src1) {
; CHECK-LABEL: @test_update_cb(
; CHECK-NEXT:    [[TMP1:%.*]] = call float @llvm.genx.GenISA.RuntimeValue(i32 0)
; CHECK-NEXT:    call void @use.f32(float [[TMP1]])
; CHECK-NEXT:    ret void
;
  %1 = load float, float addrspace(65538)* null
  %2 = fadd float %1, %1
  %3 = fadd float %2, 13.0
  call void @use.f32(float %3)
  ret void
}

declare void @use.f32(float)
