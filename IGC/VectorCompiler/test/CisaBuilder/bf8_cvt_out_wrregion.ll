;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

; COM: This test reproduces a regression where a qf.cvt (bf8) result baled
; COM: directly into a wrregion write (a common pattern when a wide
; COM: conversion gets split into several sub-width calls that get
; COM: recombined into one larger vector) silently lost its explicit VISA
; COM: type and was declared with the plain container type (ub/b) instead
; COM: of the real bf8 micro-float type (createDestination's DstDesc.WrRegion
; COM: branch). A related fix for a similar "lone" wrregion copy sub-case
; COM: only applies to some configurations and is covered by a separate,
; COM: dedicated test, so this test intentionally does not check the exact
; COM: form of the non-fused sub-width copies, only the fused case below.
; COM: [[OTHERDST]] below is an unchecked placeholder: depending on
; COM: configuration, the non-fused copy's destination may end up with the
; COM: same declared type as [[DST]], so it must be consumed here to keep
; COM: [[DST]] correctly bound to the fused case's actual destination.

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK-DAG: .decl [[SRC:V[^ ]+]] v_type=G type=hf num_elts=64 alias
; CHECK-DAG: .decl [[TMP0:V[^ ]+]] v_type=G type=bf8 num_elts=16 alias
; CHECK-DAG: .decl [[TMP1:V[^ ]+]] v_type=G type=bf8 num_elts=16 alias
; CHECK-DAG: .decl [[TMP2:V[^ ]+]] v_type=G type=bf8 num_elts=16 alias
; CHECK-DAG: .decl [[OTHERDST:V[^ ]+]] v_type=G type={{[a-z0-9]+}} num_elts=64 alias
; CHECK-DAG: .decl [[DST:V[^ ]+]] v_type=G type=bf8 num_elts=64 alias

; CHECK: mov (M1_NM, 16) [[TMP0]](0,0)<1> [[SRC]](0,0)<1;1,0>
; CHECK: mov (M1_NM, 16) [[TMP1]](0,0)<1> [[SRC]](0,16)<1;1,0>
; CHECK: mov (M1_NM, 16) [[TMP2]](0,0)<1> [[SRC]](1,0)<1;1,0>
; CHECK: mov (M1_NM, 16) [[DST]](0,48)<1> [[SRC]](1,16)<1;1,0>



; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <64 x half> @llvm.genx.oword.ld.v64f16(i32, i32, i32)
declare void @llvm.genx.oword.st.v64i8(i32, i32, <64 x i8>)

declare <16 x half> @llvm.genx.rdregionf.v16f16.v64f16.i16(<64 x half>, i32, i32, i32, i16, i32)
declare <16 x i8> @llvm.genx.qf.cvt.v16i8.v16f16(<16 x half>)
declare <64 x i8> @llvm.genx.wrregioni.v64i8.v16i8.i16.i1(<64 x i8>, <16 x i8>, i32, i32, i32, i16, i32, i1)

define dllexport spir_kernel void @bf8_cvtKernel_out_wrregion(i32 %0, i32 %1) local_unnamed_addr #0 {
  %vec = tail call <64 x half> @llvm.genx.oword.ld.v64f16(i32 0, i32 %0, i32 0)

  %vec.0 = tail call <16 x half> @llvm.genx.rdregionf.v16f16.v64f16.i16(<64 x half> %vec, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %vec.1 = tail call <16 x half> @llvm.genx.rdregionf.v16f16.v64f16.i16(<64 x half> %vec, i32 0, i32 16, i32 1, i16 32, i32 undef)
  %vec.2 = tail call <16 x half> @llvm.genx.rdregionf.v16f16.v64f16.i16(<64 x half> %vec, i32 0, i32 16, i32 1, i16 64, i32 undef)
  %vec.3 = tail call <16 x half> @llvm.genx.rdregionf.v16f16.v64f16.i16(<64 x half> %vec, i32 0, i32 16, i32 1, i16 96, i32 undef)

  %cvt.0 = tail call <16 x i8> @llvm.genx.qf.cvt.v16i8.v16f16(<16 x half> %vec.0)
  %cvt.1 = tail call <16 x i8> @llvm.genx.qf.cvt.v16i8.v16f16(<16 x half> %vec.1)
  %cvt.2 = tail call <16 x i8> @llvm.genx.qf.cvt.v16i8.v16f16(<16 x half> %vec.2)
  %cvt.3 = tail call <16 x i8> @llvm.genx.qf.cvt.v16i8.v16f16(<16 x half> %vec.3)

  %res.0 = tail call <64 x i8> @llvm.genx.wrregioni.v64i8.v16i8.i16.i1(<64 x i8> undef, <16 x i8> %cvt.0, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %res.1 = tail call <64 x i8> @llvm.genx.wrregioni.v64i8.v16i8.i16.i1(<64 x i8> %res.0, <16 x i8> %cvt.1, i32 0, i32 16, i32 1, i16 16, i32 undef, i1 true)
  %res.2 = tail call <64 x i8> @llvm.genx.wrregioni.v64i8.v16i8.i16.i1(<64 x i8> %res.1, <16 x i8> %cvt.2, i32 0, i32 16, i32 1, i16 32, i32 undef, i1 true)
  %res.3 = tail call <64 x i8> @llvm.genx.wrregioni.v64i8.v16i8.i16.i1(<64 x i8> %res.2, <16 x i8> %cvt.3, i32 0, i32 16, i32 1, i16 48, i32 undef, i1 true)

  tail call void @llvm.genx.oword.st.v64i8(i32 %1, i32 0, <64 x i8> %res.3)
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
!4 = !{void (i32, i32)* @bf8_cvtKernel_out_wrregion, !"bf8_cvtKernel_out_wrregion", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @bf8_cvtKernel_out_wrregion, null, null, null, null}
