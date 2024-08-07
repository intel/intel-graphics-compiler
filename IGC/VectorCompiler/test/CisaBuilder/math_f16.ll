;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: llc %s -march=genx64 -mcpu=XeHPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s


; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK: .decl  [[SRC:V[^ ]+]] v_type=G type=hf num_elts=8
; CHECK: cos (M1, 8) [[COS:V[^ ]+]](0,0)<1> [[SRC]](0,0)<1;1,0>
; CHECK: exp (M1, 8) [[EXP:V[^ ]+]](0,0)<1> [[SRC]](0,0)<1;1,0>
; CHECK: log (M1, 8) [[LOG:V[^ ]+]](0,0)<1> [[SRC]](0,0)<1;1,0>
; CHECK: sin (M1, 8) [[SIN:V[^ ]+]](0,0)<1> [[SRC]](0,0)<1;1,0>

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <8 x half> @llvm.genx.oword.ld.v8f16(i32, i32, i32)
declare void @llvm.genx.oword.st.v8f16(i32, i32, <8 x half>)

declare <8 x half> @llvm.cos.v8f16(<8 x half>)
declare <8 x half> @llvm.exp2.v8f16(<8 x half>)
declare <8 x half> @llvm.log2.v8f16(<8 x half>)
declare <8 x half> @llvm.sin.v8f16(<8 x half>)

define dllexport spir_kernel void @math(i32 %0, i32 %1) local_unnamed_addr #0 {
  %src = tail call <8 x half> @llvm.genx.oword.ld.v8f16(i32 0, i32 %0, i32 0)

  %cos = call afn <8 x half> @llvm.cos.v8f16(<8 x half> %src)
  %exp = call afn <8 x half> @llvm.exp2.v8f16(<8 x half> %src)
  %log = call afn <8 x half> @llvm.log2.v8f16(<8 x half> %src)
  %sin = call afn <8 x half> @llvm.sin.v8f16(<8 x half> %src)

  tail call void @llvm.genx.oword.st.v8f16(i32 %1, i32 0, <8 x half> %cos)
  tail call void @llvm.genx.oword.st.v8f16(i32 %1, i32 2, <8 x half> %exp)
  tail call void @llvm.genx.oword.st.v8f16(i32 %1, i32 4, <8 x half> %log)
  tail call void @llvm.genx.oword.st.v8f16(i32 %1, i32 6, <8 x half> %sin)
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
!4 = !{void (i32, i32)* @math, !"math", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @math, null, null, null, null}
