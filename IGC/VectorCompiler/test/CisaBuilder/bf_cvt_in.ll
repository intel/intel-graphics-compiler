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

; CHECK: .decl  [[BFIN:V[^ ]+]] v_type=G type=bf num_elts=8
; CHECK: mov (M1, 8) [[ANOTHER:V[^ ]+]](0,0)<1> [[BFIN]](0,0)<1;1,0>



; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <8 x i16> @llvm.genx.oword.ld.v8i16(i32, i32, i32)
declare <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16>)
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x float>)

define dllexport spir_kernel void @bf_cvtKernel_in(i32 %0, i32 %1) local_unnamed_addr #0 {
  %vec = tail call <8 x i16> @llvm.genx.oword.ld.v8i16(i32 0, i32 %0, i32 0)
  %upd_vec = call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> %vec)
  tail call void @llvm.genx.oword.st.v8i32(i32 %1, i32 0, <8 x float> %upd_vec)
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
!4 = !{void (i32, i32)* @bf_cvtKernel_in, !"bf_cvtKernel_in", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @bf_cvtKernel_in, null, null, null, null}
