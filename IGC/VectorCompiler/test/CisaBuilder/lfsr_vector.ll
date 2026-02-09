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

declare <16 x i32> @llvm.genx.lfsr.v16i32(<16 x i32>, <16 x i32>, i8)

; CHECK-LABEL: .kernel "lfsr"
define dllexport spir_kernel void @lfsr(i64 %buffer) local_unnamed_addr #0 {
  %pseed = inttoptr i64 %buffer to <16 x i32> addrspace(1)*
  %ppoly = getelementptr <16 x i32>, <16 x i32> addrspace(1)* %pseed, i32 1

  %seed.0 = load <16 x i32>, <16 x i32> addrspace(1)* %pseed
  %poly = load <16 x i32>, <16 x i32> addrspace(1)* %ppoly

; CHECK: lfsr.b32 (M1, 16) [[SEED:V[0-9]+\(0,0\)]]<1> [[SEED]]<1;1,0> [[POLY:V[0-9]+\(0,0\)]]<1;1,0>
  %seed.1 = call <16 x i32> @llvm.genx.lfsr.v16i32(<16 x i32> %seed.0, <16 x i32> %poly, i8 0)
; CHECK: lfsr.b16v2 (M1, 16) [[SEED]]<1> [[SEED]]<1;1,0> [[POLY]]<1;1,0>
  %seed.2 = call <16 x i32> @llvm.genx.lfsr.v16i32(<16 x i32> %seed.1, <16 x i32> %poly, i8 1)
; CHECK: lfsr.b8v4 (M1, 16) [[SEED]]<1> [[SEED]]<1;1,0> [[POLY]]<1;1,0>
  %seed.3 = call <16 x i32> @llvm.genx.lfsr.v16i32(<16 x i32> %seed.2, <16 x i32> %poly, i8 2)

  store <16 x i32> %seed.3, <16 x i32> addrspace(1)* %pseed

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
!4 = !{void (i64)* @lfsr, !"lfsr", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0}
!6 = !{i32 64}
!7 = !{!"svmptr_t"}
!8 = !{void (i64)* @lfsr, null, null, null, null}
