;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:32:32-v96:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

; RUN: igc_opt --opaque-pointers -S --platformbmg --igc-ocl-merge-allocas %s | FileCheck %s

define void @test() #0 {
  %alloca1 = alloca float
  %alloca2 = alloca float
; CHECK: alloca float
; CHECK-NOT: alloca float
  %load1 = load float, ptr %alloca1
  %load2 = load float, ptr %alloca1
  %load3 = load float, ptr %alloca2
  ret void
}

attributes #0 = { nounwind readnone }


!llvm.ident = !{!0, !0, !0, !0}
!dx.version = !{!1, !1, !1, !1}
!dx.valver = !{!2, !2, !2, !2}
!dx.shaderModel = !{!3, !3, !3, !3}

!0 = !{!"clang version 3.7 (tags/RELEASE_370/final)"}
!1 = !{i32 1, i32 5}
!2 = !{i32 1, i32 6}
!3 = !{!"lib", i32 6, i32 5}
