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

; Test interaction between control flow and escaping lifetime

declare void @escape(ptr %a)

; Function Attrs: alwaysinline nounwind
define void @fun() #0 {
start:
  %alloca_inf = alloca float
  %alloca = alloca float
; CHECK: alloca float
; CHECK: alloca float
  br label %altpath

altpath:
  br i1 undef, label %altpath1, label %altpath2

altpath1:
  call void @escape(ptr %alloca_inf)
  br label %altpathexit

altpath2:
  br label %altpathexit

altpathexit:
  %a = load float, ptr %alloca
  ret void
}

attributes #0 = { nounwind }
