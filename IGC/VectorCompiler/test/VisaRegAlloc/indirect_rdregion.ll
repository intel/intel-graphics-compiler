;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info \
; RUN: -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info \
; RUN: -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s

; The result of an indirect rdregion must not be coalesced with its source if
; the result spans two GRFs: the compressed indirect move would otherwise
; overwrite in its first part elements its second part still has to read.

; CHECK-LABEL: .kernel "test_two_grf"
; CHECK: addr_add (M1, 32) A0(0)<1> &[[SRC:V[0-9]+]] V{{[0-9]+}}(0,0)<1;1,0>
; CHECK-NEXT: mov (M1, 32)
; CHECK-NOT: {{ }}[[SRC]](
; CHECK-SAME: 0,0)<1> r[A0(0),0]<1,0>:d

; A GRF-aligned result that fits in a single GRF can still be coalesced.

; CHECK-LABEL: .kernel "test_one_grf"
; CHECK: addr_add (M1, 16) A0(0)<1> &[[SRC1:V[0-9]+]] V{{[0-9]+}}(0,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[SRC1]](0,0)<1> r[A0(0),0]<1,0>:d

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <32 x i32> @llvm.genx.rdregioni.v32i32.v32i32.v32i16(<32 x i32>, i32, i32, i32, <32 x i16>, i32)
declare <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.v16i16(<16 x i32>, i32, i32, i32, <16 x i16>, i32)

define dllexport spir_kernel void @test_two_grf(<32 x i32> addrspace(1)* %src, <32 x i16> addrspace(1)* %idx, <32 x i32> addrspace(1)* %dst, i64 %impl.arg.private.base) #0 {
  %data = load <32 x i32>, <32 x i32> addrspace(1)* %src, align 16
  %offsets = load <32 x i16>, <32 x i16> addrspace(1)* %idx, align 16
  %shl = shl <32 x i16> %offsets, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  %gather = tail call <32 x i32> @llvm.genx.rdregioni.v32i32.v32i32.v32i16(<32 x i32> %data, i32 0, i32 1, i32 0, <32 x i16> %shl, i32 undef)
  store <32 x i32> %gather, <32 x i32> addrspace(1)* %dst, align 16
  ret void
}

define dllexport spir_kernel void @test_one_grf(<16 x i32> addrspace(1)* %src, <16 x i16> addrspace(1)* %idx, <16 x i32> addrspace(1)* %dst, i64 %impl.arg.private.base) #0 {
  %data = load <16 x i32>, <16 x i32> addrspace(1)* %src, align 16
  %offsets = load <16 x i16>, <16 x i16> addrspace(1)* %idx, align 16
  %shl = shl <16 x i16> %offsets, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  %gather = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v16i32.v16i16(<16 x i32> %data, i32 0, i32 1, i32 0, <16 x i16> %shl, i32 undef)
  store <16 x i32> %gather, <16 x i32> addrspace(1)* %dst, align 16
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0, !10}
!genx.kernel.internal = !{!5, !11}

!0 = !{void (<32 x i32> addrspace(1)*, <32 x i16> addrspace(1)*, <32 x i32> addrspace(1)*, i64)* @test_two_grf, !"test_two_grf", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 96}
!2 = !{i32 64, i32 72, i32 80, i32 128}
!3 = !{i32 0, i32 0, i32 0}
!4 = !{!"svmptr_t", !"svmptr_t", !"svmptr_t", !""}
!5 = !{void (<32 x i32> addrspace(1)*, <32 x i16> addrspace(1)*, <32 x i32> addrspace(1)*, i64)* @test_two_grf, !6, !7, !8, !9}
!6 = !{i32 0, i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2, i32 3}
!8 = !{}
!9 = !{i32 255, i32 255, i32 255, i32 255}
!10 = !{void (<16 x i32> addrspace(1)*, <16 x i16> addrspace(1)*, <16 x i32> addrspace(1)*, i64)* @test_one_grf, !"test_one_grf", !1, i32 0, !2, !3, !4, i32 0}
!11 = !{void (<16 x i32> addrspace(1)*, <16 x i16> addrspace(1)*, <16 x i32> addrspace(1)*, i64)* @test_one_grf, !6, !7, !8, !9}
