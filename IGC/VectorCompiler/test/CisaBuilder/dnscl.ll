;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
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

declare <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32>, <16 x i32>, <16 x i32>, i8, i8, i8)

; CHECK-LABEL: .kernel "dnscl"
define dllexport spir_kernel void @dnscl(i64 %buffer) local_unnamed_addr #0 {
  %p0 = inttoptr i64 %buffer to <16 x i32> addrspace(1)*
  %p1 = getelementptr <16 x i32>, <16 x i32> addrspace(1)* %p0, i64 1
  %p2 = getelementptr <16 x i32>, <16 x i32> addrspace(1)* %p0, i64 2

  %src0 = load <16 x i32>, <16 x i32> addrspace(1)* %p0, align 64
  %src1 = load <16 x i32>, <16 x i32> addrspace(1)* %p1, align 64
  %bias = load <16 x i32>, <16 x i32> addrspace(1)* %p2, align 64

; CHECK: dnscl.hftoint4.mode0.srnd (M1, 16) {{V[0-9]+.0}} [[SRC0:V[0-9]+.0]] [[SRC1:V[0-9]+.0]] [[BIAS:V[0-9]+.0]]
  %m0 = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %src0, <16 x i32> %src1, <16 x i32> %bias, i8 5, i8 0, i8 0)
; CHECK: dnscl.hftoe2m1.mode1.srnd (M1, 16) {{V[0-9]+.0}} [[SRC0]] [[SRC1]] [[BIAS]]
  %m1 = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %src0, <16 x i32> %src1, <16 x i32> %bias, i8 4, i8 1, i8 0)
; CHECK: dnscl.bftoint4.mode3.rne (M1, 16) {{V[0-9]+.0}} [[SRC0]] [[SRC1]] %null.0
  %m2 = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %src0, <16 x i32> %src1, <16 x i32> undef, i8 2, i8 3, i8 1)
; CHECK: dnscl.bftoe2m1.mode2.srnd (M1, 16) {{V[0-9]+.0}} [[SRC0]] [[SRC1]] [[BIAS]]
  %m3 = call <16 x i32> @llvm.genx.4bit.downconvert.v16i32(<16 x i32> %src0, <16 x i32> %src1, <16 x i32> %bias, i8 1, i8 2, i8 0)

  %r0 = or <16 x i32> %m0, %m1
  %r1 = or <16 x i32> %m2, %m3

  store <16 x i32> %r0, <16 x i32> addrspace(1)* %p0, align 64
  store <16 x i32> %r1, <16 x i32> addrspace(1)* %p1, align 64

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
!4 = !{void (i64)* @dnscl, !"dnscl", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0}
!6 = !{i32 64}
!7 = !{!"svmptr_t"}
!8 = !{void (i64)* @dnscl, null, null, null, null}
