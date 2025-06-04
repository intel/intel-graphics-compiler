;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:64:64:64-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:32:32-v96:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

; RUN: igc_opt --opaque-pointers -S --platformbmg --igc-ocl-merge-allocas %s | FileCheck %s

; This is a weird loop case found in Cyberpunk, where there seems to be no clear starting point for the lifetime analysis

declare void @escape(ptr %a)

; Function Attrs: alwaysinline nounwind
define void @"\01?rgs_shadow_main@@YAXXZ"() #0 {
start:
  %alloca_inf = alloca float, align 8
  %alloca = alloca float, align 8
; CHECK: alloca float
; CHECK: alloca float
  br label %header

header:
  store float undef, ptr %alloca
  br i1 undef, label %bb6, label %latch_exiting

bb6:
  br i1 undef, label %exiting1, label %latch_exiting

exiting1:
  br i1 undef, label %exit, label %bb23

bb23:
  br i1 undef, label %bb37, label %latch_exiting

bb37:
  call void @escape(ptr %alloca_inf)
  br label %latch_exiting

latch_exiting:
  br i1 undef, label %exit, label %header

exit:
  ret void
}

attributes #0 = { nounwind }
