;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: This test verifies that spirv.ExecutionMode metadata is not corrupted
; COM: somewhere in VC pipeline.

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe2 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe2 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s

; CHECK: .kernel_attr NumGRF=256
define dllexport spir_kernel void @kernel(i32 %arg) local_unnamed_addr #0 {
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!2}
!genx.kernel.internal = !{!3}
!spirv.ExecutionMode = !{!4}

!0 = !{i32 24}
!1 = !{}
!2 = !{void (i32)* @kernel, !"kernel", !0, i32 0, !0, !1, !1, i32 0}
!3 = !{void (i32)* @kernel, null, null, null, null}
!4 = !{void (i32)* @kernel, i32 6461, i32 256}
