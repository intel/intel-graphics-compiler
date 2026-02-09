;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
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



target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i64>, i16, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i32>)

; CHECK: .decl [[PRED:P[0-9]+]] v_type=P num_elts=16

define spir_kernel void @test(i16 %mask, <16 x i64> %index, <16 x i32> %src1, <16 x i32> %src2, <16 x i32> %passthru) local_unnamed_addr #0 {
  %pred = bitcast i16 %mask to <16 x i1>

  ; CHECK: ([[PRED]]) lsc_atomic_bfadd.ugm (M1, 16)  %null:d16u32  flat[[[INDEX:V[0-9]+]]]:a64  [[SRC:V[0-9]+]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfsub.ugm (M1, 16)  %null:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfmin.ugm (M1, 16)  %null:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfmax.ugm (M1, 16)  %null:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfcas.ugm (M1, 16)  %null:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  [[SRC2:V[0-9]+]]

  %1 = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 33, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 34, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %3 = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 35, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %4 = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 36, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %5 = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 37, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> %src2, <16 x i32> %passthru)

  ; CHECK: ([[PRED]]) lsc_atomic_bfadd.ugm (M1, 16)  V{{[0-9]+}}:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfsub.ugm (M1, 16)  V{{[0-9]+}}:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfmin.ugm (M1, 16)  V{{[0-9]+}}:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfmax.ugm (M1, 16)  V{{[0-9]+}}:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  %null
  ; CHECK: ([[PRED]]) lsc_atomic_bfcas.ugm (M1, 16)  V{{[0-9]+}}:d16u32  flat[[[INDEX]]]:a64  [[SRC]]  [[SRC2]]

  %bfadd = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 33, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %bfsub = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 34, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %bfmin = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 35, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %bfmax = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 36, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)
  %bfcas = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 37, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> %src2, <16 x i32> %passthru)

  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1> %pred, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %bfadd)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1> %pred, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %bfsub)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1> %pred, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %bfmin)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1> %pred, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %bfmax)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16i64(<16 x i1> %pred, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %index, i16 1, i32 0, <16 x i32> %bfcas)

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
!2 = !{i32 1, i32 1}
!3 = !{i16 6, i16 14}
!4 = !{void (i16, <16 x i64>, <16 x i32>, <16 x i32>, <16 x i32>)* @test, !"test", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 64, i32 68, i32 196, i32 260, i32 324}
!7 = !{!"image2d_t"}
!8 = !{void (i16, <16 x i64>, <16 x i32>, <16 x i32>, <16 x i32>)* @test, null, null, null, null}
