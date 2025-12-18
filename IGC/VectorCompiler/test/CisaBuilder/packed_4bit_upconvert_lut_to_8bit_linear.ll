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

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32>, <16 x i16>) #1

declare <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32) #1
declare <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16>, <16 x i16>, i32, i32, i32, i16, i32, i1) #1

declare <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <16 x i32>)

; CHECK-DAG: .decl [[SRC:V[0-9]+]] v_type=G type=w num_elts=32 align=GRF
; CHECK-DAG: .decl [[SVNNI:V[0-9]+]] v_type=G type=w num_elts=32 align=GRF
; CHECK-DAG: .decl [[VNNI:V[0-9]+]] v_type=G type=uw num_elts=32 alias=<[[SVNNI]], 0>

define dllexport spir_kernel void @kernel(i64 %0, i64 %1) local_unnamed_addr #0 {
  %lut = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %0, i16 1, i32 0, <16 x i32> undef)
  %src = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %1, i16 1, i32 0, <16 x i32> undef)

  %src.i16 = bitcast <16 x i32> %src to <32 x i16>

  %src.i16.0 = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %src.i16, i32 1, i32 1, i32 0, i16 0, i32 undef)
  %src.i16.1 = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %src.i16, i32 1, i32 1, i32 0, i16 32, i32 undef)

  ; CHECK: mov (M1, 16) [[SVNNI]](0,0)<2> [[SRC]](0,0)<1;1,0>
  ; CHECK: mov (M1, 16) [[SVNNI]](0,1)<2> [[SRC]](0,16)<1;1,0>
  %vnni.0 = call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> undef, <16 x i16> %src.i16.0, i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
  %vnni = call <32 x i16> @llvm.genx.wrregioni.v32i16.v16i16.i16.i1(<32 x i16> %vnni.0, <16 x i16> %src.i16.1, i32 2, i32 1, i32 0, i16 2, i32 undef, i1 true)

  %src.vnni.i16.0 = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %vnni, i32 2, i32 1, i32 0, i16 0, i32 undef)
  %src.vnni.i16.1 = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %vnni, i32 2, i32 1, i32 0, i16 2, i32 undef)

  ; CHECK: shfl_idx4 (M1, 16) V{{[0-9]+}}.0 [[LUT:V[0-9]+]](0,0)<1;1,0> [[VNNI]](0,0)<2;1,0>
  ; CHECK: shfl_idx4 (M1, 16) V{{[0-9]+}}.0 [[LUT]](0,0)<1;1,0> [[VNNI]](0,1)<2;1,0>
  %res.0 = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %src.vnni.i16.0)
  %res.1 = call <16 x i32> @llvm.vc.internal.packed.4bit.upconvert.lut.v16i32.v16i16(<16 x i32> %lut, <16 x i16> %src.vnni.i16.1)

  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %1, i16 1, i32 0, <16 x i32> %res.0)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %1, i16 1, i32 64, <16 x i32> %res.1)

  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }
attributes #1 = { nounwind readnone }

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
!4 = !{void (i64, i64)* @kernel, !"kernel", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0}
!6 = !{i32 64, i32 72}
!7 = !{!"svmptr_t", !"svmptr_t"}
!8 = !{void (i64, i64)* @kernel, null, null, null, null}
