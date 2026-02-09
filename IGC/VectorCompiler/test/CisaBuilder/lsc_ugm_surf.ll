;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s



target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.vc.internal.lsc.atomic.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, i8, <16 x i32>, i16, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.surf.v16i1.v2i8.v16i32.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, i8, <16 x i32>, i16, i32, <16 x i32>)

declare <16 x i32> @llvm.vc.internal.lsc.load.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, i8, <16 x i32>, i16, i32, <16 x i32>)

declare <64 x i32> @llvm.vc.internal.lsc.load.quad.surf.v64i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, i8, <16 x i32>, i16, i32, <64 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.surf.v16i1.v2i8.v16i32.v64i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, i8, <16 x i32>, i16, i32, <64 x i32>)

declare void @llvm.vc.internal.lsc.prefetch.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, i8, <16 x i32>, i16, i32)

declare void @llvm.vc.internal.lsc.prefetch.quad.surf.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, i8, <16 x i32>, i16, i32)

; CHECK: .decl [[PRED:P[0-9]+]] v_type=P num_elts=16

define spir_kernel void @test(i16 %mask, i64 %surf, <16 x i32> %index, <16 x i32> %src1, <16 x i32> %src2, <16 x i32> %passthru) local_unnamed_addr #0 {
  %pred = bitcast i16 %mask to <16 x i1>

  ; CHECK: ([[PRED]]) lsc_atomic_fadd.ugm (M1, 16)  [[DST:V[0-9]+]]:d32  surf([[SURF:V[0-9]+]])[[[INDEX:V[0-9]+]]]:a32u
  %atomicfadd = call <16 x i32> @llvm.vc.internal.lsc.atomic.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 19, i8 4, i8 3, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0, <16 x i32> %src1, <16 x i32> undef, <16 x i32> %passthru)

  ; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16)  surf([[SURF]])[[[INDEX]]]:a32u  [[DST]]:d32
  call void @llvm.vc.internal.lsc.store.surf.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0, <16 x i32> %atomicfadd)

  ; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16)  [[DST:V[0-9]+]]:d32  surf([[SURF]])[[[INDEX]]]:a32u
  %load = call <16 x i32> @llvm.vc.internal.lsc.load.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0, <16 x i32> %passthru)

  ; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16)  surf([[SURF]])[[[INDEX]]]:a32u  [[DST]]:d32
  call void @llvm.vc.internal.lsc.store.surf.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0, <16 x i32> %load)

  ; CHECK: ([[PRED]]) lsc_load_quad.ugm (M1, 16)  [[DST:V[0-9]+]]:d32.xyzw  surf([[SURF]])[[[INDEX]]]:a32u
  %loadquad = call <64 x i32> @llvm.vc.internal.lsc.load.quad.surf.v64i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 15, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0, <64 x i32> zeroinitializer)

  ; CHECK: ([[PRED]]) lsc_store_quad.ugm (M1, 16)  surf([[SURF]])[[[INDEX]]]:a32u  [[DST]]:d32.xyzw
  call void @llvm.vc.internal.lsc.store.quad.surf.v16i1.v2i8.v16i32.v64i32(<16 x i1> %pred, i8 4, i8 3, i8 15, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0, <64 x i32> %loadquad)

  ; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16)  %null:d32  surf([[SURF]])[[[INDEX]]]:a32u
  call void @llvm.vc.internal.lsc.prefetch.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0)

  ; CHECK: ([[PRED]]) lsc_load_quad.ugm (M1, 16)  %null:d32.xyzw  surf([[SURF]])[[[INDEX]]]:a32u
  call void @llvm.vc.internal.lsc.prefetch.quad.surf.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 15, <2 x i8> zeroinitializer, i64 %surf, i8 0, <16 x i32> %index, i16 1, i32 0)

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
!4 = !{void (i16, i64, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)* @test, !"test", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 64, i32 68, i32 76, i32 204, i32 268, i32 332}
!7 = !{!"image2d_t"}
!8 = !{void (i16, i64, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)* @test, null, null, null, null}
