;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeHPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

; COM: dpas src2 (s8 x s8, systolic depth 8) must be 32-byte aligned: a source
; COM: region at a 32-byte offset is used directly, one at an 8-byte offset is
; COM: copied first.

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <32 x i32> @llvm.genx.oword.ld.v32i32(i32, i32, i32)
declare <64 x i32> @llvm.genx.oword.ld.v64i32(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)
declare <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.dpas2.v8i32.v8i32.v64i32.v8i32(<8 x i32>, <64 x i32>, <8 x i32>, i32, i32, i32, i32, i32, i32)

; CHECK-LABEL: .kernel "dpas_src2_aligned"
; CHECK-NOT: mov (M1, 8)
; CHECK: dpas.s8.s8.8.1 (M1, 8) {{V[0-9]+}}.0 {{V[0-9]+}}.0 {{V[0-9]+}}.0 {{V[0-9]+}}(1,0)
define dllexport spir_kernel void @dpas_src2_aligned(i32 %0, i32 %1) local_unnamed_addr #0 {
  %acc = tail call <32 x i32> @llvm.genx.oword.ld.v32i32(i32 0, i32 %0, i32 0)
  %src1 = tail call <64 x i32> @llvm.genx.oword.ld.v64i32(i32 0, i32 %0, i32 8)
  %src = tail call <32 x i32> @llvm.genx.oword.ld.v32i32(i32 0, i32 %0, i32 24)
  %a = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %acc, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %s2 = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %src, i32 0, i32 8, i32 1, i16 32, i32 undef)
  %r = tail call <8 x i32> @llvm.genx.dpas2.v8i32.v8i32.v64i32.v8i32(<8 x i32> %a, <64 x i32> %src1, <8 x i32> %s2, i32 8, i32 8, i32 8, i32 1, i32 1, i32 1)
  tail call void @llvm.genx.oword.st.v8i32(i32 1, i32 %1, <8 x i32> %r)
  ret void
}

; CHECK-LABEL: .kernel "dpas_src2_unaligned"
; CHECK: mov (M1, 8) [[TMP:V[0-9]+]](0,0)<1> {{V[0-9]+}}(0,2)<1;1,0>
; CHECK: dpas.s8.s8.8.1 (M1, 8) {{V[0-9]+}}.0 {{V[0-9]+}}.0 {{V[0-9]+}}.0 [[TMP]](0,0)
define dllexport spir_kernel void @dpas_src2_unaligned(i32 %0, i32 %1) local_unnamed_addr #0 {
  %acc = tail call <32 x i32> @llvm.genx.oword.ld.v32i32(i32 0, i32 %0, i32 0)
  %src1 = tail call <64 x i32> @llvm.genx.oword.ld.v64i32(i32 0, i32 %0, i32 8)
  %src = tail call <32 x i32> @llvm.genx.oword.ld.v32i32(i32 0, i32 %0, i32 24)
  %a = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %acc, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %s2 = tail call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> %src, i32 0, i32 8, i32 1, i16 8, i32 undef)
  %r = tail call <8 x i32> @llvm.genx.dpas2.v8i32.v8i32.v64i32.v8i32(<8 x i32> %a, <64 x i32> %src1, <8 x i32> %s2, i32 8, i32 8, i32 8, i32 1, i32 1, i32 1)
  tail call void @llvm.genx.oword.st.v8i32(i32 1, i32 %1, <8 x i32> %r)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!10, !11}
!genx.kernel.internal = !{!20, !21}

!1 = !{i32 2, i32 2}
!2 = !{i32 32, i32 36}
!3 = !{}
!10 = !{void (i32, i32)* @dpas_src2_aligned, !"dpas_src2_aligned", !1, i32 0, !2, !3, !3, i32 0}
!11 = !{void (i32, i32)* @dpas_src2_unaligned, !"dpas_src2_unaligned", !1, i32 0, !2, !3, !3, i32 0}
!20 = !{void (i32, i32)* @dpas_src2_aligned, null, null, null, null}
!21 = !{void (i32, i32)* @dpas_src2_unaligned, null, null, null, null}
