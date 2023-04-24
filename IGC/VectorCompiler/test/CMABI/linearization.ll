;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define dllexport void @cmk_kmeans(i1 zeroext %0, i64 %privBase) #0 {
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
; CHECK: !4 = !{void
; CHECK-NOT: i1
; CHECK-SAME: i8
!4 = !{void (i1, i64)* @cmk_kmeans, !"cmk_kmeans", !5, i32 0, i32 0, !6, !7, i32 0}
!5 = !{i32 0, i32 96}
!6 = !{i32 0}
!7 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"", !""}
; CHECK: !8 = !{void
; CHECK-NOT: i1
; CHECK-SAME: i8
!8 = !{void (i1, i64)* @cmk_kmeans, null, null, !2, null}
