;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s


; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK: fcvt

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <4 x float> @llvm.genx.oword.ld.v4f32(i32, i32, i32)
declare <4 x i32> @llvm.vc.internal.round.to.tf32.v4i32.v4f32(<4 x float>)
declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>)

define dllexport spir_kernel void @tf32_cvtKernel_in(i32 %0, i32 %1) local_unnamed_addr #0 {
  %vec = tail call <4 x float> @llvm.genx.oword.ld.v4f32(i32 0, i32 %0, i32 0)
  %upd_vec = tail call <4 x i32> @llvm.vc.internal.round.to.tf32.v4i32.v4f32(<4 x float> %vec)
  tail call void @llvm.genx.oword.st.v4i32(i32 %1, i32 0, <4 x i32> %upd_vec)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32)* @tf32_cvtKernel_in, !"tf32_cvtKernel_in", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @tf32_cvtKernel_in, null, null, null, null}
