;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: llc %s -march=genx64 -mcpu=XeHPG -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s


; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK: .decl  [[BFDST:V[^ ]+]] v_type=G type=bf num_elts=8
; CHECK: mov (M1, 8) [[BFDST]](0,0)<1> [[ANOTHER:V[^ ]+]](0,0)<1;1,0>


; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <8 x float> @llvm.genx.oword.ld.v8f32(i32, i32, i32)
declare <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float>)
declare void @llvm.genx.oword.st.v8i16(i32, i32, <8 x i16>)

define dllexport spir_kernel void @bf_cvtKernel_out(i32 %0, i32 %1) local_unnamed_addr #0 {
  %vec = tail call <8 x float> @llvm.genx.oword.ld.v8f32(i32 0, i32 %0, i32 0)
  %upd_vec = call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> %vec)
  tail call void @llvm.genx.oword.st.v8i16(i32 %1, i32 0, <8 x i16> %upd_vec)
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
!4 = !{void (i32, i32)* @bf_cvtKernel_out, !"bf_cvtKernel_out", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @bf_cvtKernel_out, null, null, null, null}
