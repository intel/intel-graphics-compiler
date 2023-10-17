;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: llc %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK-DAG: .decl  [[SRC0:V[^ ]+]] v_type=G type=f num_elts=8
; CHECK-DAG: .decl  [[SRC1:V[^ ]+]] v_type=G type=uw num_elts=16
; CHECK-DAG: .decl  [[DST:V[^ ]+]] v_type=G type=hf num_elts=8
; CHECK: srnd (M1_NM, 8) [[DST]](0,0)<1> [[SRC0]](0,0)<1;1,0> [[SRC1]](0,0)<2;1,0>

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <8 x float> @llvm.genx.oword.ld.v8f32(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i16(i32, i32, <8 x half>)
declare <8 x half> @llvm.vc.internal.stochastic.round.to.f16.v8f16.v8f32.v8i16(<8 x float>, <8 x i16>)
declare <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16>, i32, i32, i32, i16, i32)

define dllexport spir_kernel void @srnd_hf_Kernel_out(i32 %0, i32 %1) local_unnamed_addr #0 {
  %vec = tail call <8 x float> @llvm.genx.oword.ld.v8f32(i32 0, i32 %0, i32 0)
  %rnd = tail call <8 x float> @llvm.genx.oword.ld.v8f32(i32 0, i32 %0, i32 256)
  %rndi = bitcast <8 x float> %rnd to <16 x i16>
  %rndx = call <8 x i16> @llvm.genx.rdregioni.v8i16.v16i16.i16(<16 x i16> %rndi, i32 2, i32 1, i32 0, i16 0, i32 undef)
  %upd_vec = call <8 x half> @llvm.vc.internal.stochastic.round.to.f16.v8f16.v8f32.v8i16(<8 x float> %vec, <8 x i16> %rndx)
  tail call void @llvm.genx.oword.st.v8i16(i32 %1, i32 0, <8 x half> %upd_vec)
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
!4 = !{void (i32, i32)* @srnd_hf_Kernel_out, !"srnd_hf_Kernel_out", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @srnd_hf_Kernel_out, null, null, null, null}
