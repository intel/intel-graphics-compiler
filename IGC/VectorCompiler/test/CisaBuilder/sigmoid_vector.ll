;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s



target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <16 x float> @llvm.vc.internal.sigmoid.v16f32(<16 x float>)
declare <16 x half> @llvm.vc.internal.sigmoid.v16f16(<16 x half>)

; CHECK-LABEL: .kernel "sigmoid"
; CHECK-DAG: .decl [[VF32:V[0-9]+]] v_type=G type=f num_elts=16
; CHECK-DAG: .decl [[VF16:V[0-9]+]] v_type=G type=hf num_elts=16
define dllexport spir_kernel void @sigmoid(i64 %buffer) local_unnamed_addr #0 {
  %pf32 = inttoptr i64 %buffer to <16 x float> addrspace(1)*
  %pf16 = inttoptr i64 %buffer to <16 x half> addrspace(1)*

  %f32s = load <16 x float>, <16 x float> addrspace(1)* %pf32
  %f16s = load <16 x half>, <16 x half> addrspace(1)* %pf16

  ; CHECK: sigm (M1, 16) [[VF32]](0,0)<1> [[VF32]](0,0)<1;1,0>
  %f32d = call <16 x float> @llvm.vc.internal.sigmoid.v16f32(<16 x float> %f32s)
  ; CHECK: sigm (M1, 16) [[VF16]](0,0)<1> [[VF16]](0,0)<1;1,0>
  %f16d = call <16 x half> @llvm.vc.internal.sigmoid.v16f16(<16 x half> %f16s)

  store <16 x float> %f32d, <16 x float> addrspace(1)* %pf32
  store <16 x half> %f16d, <16 x half> addrspace(1)* %pf16

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
!1 = !{i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i16 6, i16 14}
!4 = !{void (i64)* @sigmoid, !"sigmoid", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0}
!6 = !{i32 64}
!7 = !{!"svmptr_t"}
!8 = !{void (i64)* @sigmoid, null, null, null, null}
