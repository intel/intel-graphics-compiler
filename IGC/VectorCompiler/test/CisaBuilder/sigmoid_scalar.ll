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

declare float @llvm.vc.internal.sigmoid.f32(float)
declare half @llvm.vc.internal.sigmoid.f16(half)

; CHECK-LABEL: .kernel "sigmoid"
; CHECK-DAG: .decl [[SF32:V[0-9]+]] v_type=G type=f num_elts=1
; CHECK-DAG: .decl [[SF16:V[0-9]+]] v_type=G type=hf num_elts=2
; CHECK-DAG: .decl [[DF16:V[0-9]+]] v_type=G type=hf num_elts=1
define dllexport spir_kernel void @sigmoid(i64 %buffer) local_unnamed_addr #0 {
  %pf32 = inttoptr i64 %buffer to float addrspace(1)*
  %pf16 = inttoptr i64 %buffer to half addrspace(1)*

  %f32s = load float, float addrspace(1)* %pf32
  %f16s = load half, half addrspace(1)* %pf16

  ; CHECK: sigm (M1, 1) [[SF32]](0,0)<1> [[SF32]](0,0)<0;1,0>
  %f32d = call float @llvm.vc.internal.sigmoid.f32(float %f32s)
  ; CHECK: sigm (M1, 1) [[DF16]](0,0)<1> [[SF16]](0,0)<0;1,0>
  %f16d = call half @llvm.vc.internal.sigmoid.f16(half %f16s)

  store float %f32d, float addrspace(1)* %pf32
  store half %f16d, half addrspace(1)* %pf16

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
