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
%struct.Foo = type { float }

define void @test() #0 {
  %alloca1 = alloca %struct.Foo
  %alloca2 = alloca %struct.Foo
; CHECK: alloca %struct.Foo
; CHECK-NOT: alloca %struct.Foo
  %gep1 = getelementptr inbounds %struct.Foo, ptr %alloca1, i32 0, i32 0
  %load1 = load float, ptr %gep1
  br label %bb1

bb1:
  %gep2 = getelementptr inbounds %struct.Foo, ptr %alloca2, i32 0, i32 0
  %load2 = load float, ptr %gep2
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
