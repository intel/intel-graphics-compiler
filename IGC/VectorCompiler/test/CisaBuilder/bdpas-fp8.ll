;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null 2>&1 | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null 2>&1 | \
; RUN: FileCheck %s

; CHECK: .decl [[ACC:V[0-9]+]] v_type=G type=f num_elts=128 align=GRF
; CHECK: .decl [[SRC1:V[0-9]+]] v_type=G type=ud num_elts=128 align=wordx32
; CHECK: .decl [[SRC2:V[0-9]+]] v_type=G type=ud num_elts=64
; CHECK: .decl [[SCALE1:V[0-9]+]] v_type=G type=ub num_elts=16
; CHECK: .decl [[SCALE2:V[0-9]+]] v_type=G type=ub num_elts=8

; CHECK: bdpas.bf8.bf8.8.8 (M1, 16) [[ACC]].0 %null.0 [[SRC1]].0 [[SRC2]].0 [[SCALE1]](0,0) [[SCALE2]](0,0)
; CHECK: bdpas.bf8.hf8.8.8 (M1, 16) [[ACC]].0 %null.0 [[SRC1]].0 [[SRC2]].0 %null(0,0) [[SCALE2]](0,0)
; CHECK: bdpas.hf8.bf8.8.8 (M1, 16) [[ACC]].0 %null.0 [[SRC1]].0 [[SRC2]].0 [[SCALE1]](0,0) %null(0,0)
; CHECK: bdpas.hf8.hf8.8.8 (M1, 16) [[ACC]].0 [[ACC]].0 [[SRC1]].0 [[SRC2]].0 [[SCALE1]](0,0) [[SCALE2]](0,0)
; CHECK: bdpas.bf8.bf8.8.8 (M1, 16) [[ACC]].0 [[ACC]].0 [[SRC1]].0 [[SRC2]].0 %null(0,0) [[SCALE2]](0,0)
; CHECK: bdpas.bf8.bf8.8.8 (M1, 16) [[ACC]].0 [[ACC]].0 [[SRC1]].0 [[SRC2]].0 [[SCALE1]](0,0) %null(0,0)

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@Acc = internal global <128 x float> undef, align 512, !spirv.Decorations !0 #0
@Src1 = internal global <128 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src2 = internal global <64 x i32> undef, align 256, !spirv.Decorations !5 #0
@Src1Scale = internal global <16 x i8> undef, align 16, !spirv.Decorations !7 #0
@Src2Scale = internal global <8 x i8> undef, align 8, !spirv.Decorations !9 #0

declare <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float>, <128 x i32>, <64 x i32>, <16 x i8>, <8 x i8>, i32, i32, i32, i32) #1

declare void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float>, <128 x float>*) #2
declare <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>*) #2
declare <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>*) #2
declare <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>*) #2
declare <16 x i8> @llvm.genx.vload.v16i8.p0v16i8(<16 x i8>*) #2
declare <8 x i8> @llvm.genx.vload.v8i8.p0v8i8(<8 x i8>*) #2

define dllexport spir_kernel void @kernel(i64 %impl.arg.private.base) local_unnamed_addr #3 {
  %1 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %2 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %3 = tail call <16 x i8> @llvm.genx.vload.v16i8.p0v16i8(<16 x i8>* nonnull @Src1Scale)
  %4 = tail call <8 x i8> @llvm.genx.vload.v8i8.p0v8i8(<8 x i8>* nonnull @Src2Scale)
  %5 = tail call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> zeroinitializer, <128 x i32> %1, <64 x i32> %2, <16 x i8> %3, <8 x i8> %4, i32 11, i32 11, i32 8, i32 8)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %5, <128 x float>* nonnull @Acc)
  %6 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %7 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %8 = tail call <8 x i8> @llvm.genx.vload.v8i8.p0v8i8(<8 x i8>* nonnull @Src2Scale)
  %9 = tail call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> zeroinitializer, <128 x i32> %6, <64 x i32> %7, <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, <8 x i8> %8, i32 11, i32 14, i32 8, i32 8)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %9, <128 x float>* nonnull @Acc)
  %10 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %11 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %12 = tail call <16 x i8> @llvm.genx.vload.v16i8.p0v16i8(<16 x i8>* nonnull @Src1Scale)
  %13 = tail call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> zeroinitializer, <128 x i32> %10, <64 x i32> %11, <16 x i8> %12, <8 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, i32 14, i32 11, i32 8, i32 8)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %13, <128 x float>* nonnull @Acc)
  %14 = tail call <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>* nonnull @Acc)
  %15 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %16 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %17 = tail call <16 x i8> @llvm.genx.vload.v16i8.p0v16i8(<16 x i8>* nonnull @Src1Scale)
  %18 = tail call <8 x i8> @llvm.genx.vload.v8i8.p0v8i8(<8 x i8>* nonnull @Src2Scale)
  %19 = tail call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %14, <128 x i32> %15, <64 x i32> %16, <16 x i8> %17, <8 x i8> %18, i32 14, i32 14, i32 8, i32 8)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %19, <128 x float>* nonnull @Acc)
  %20 = tail call <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>* nonnull @Acc)
  %21 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %22 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %23 = tail call <8 x i8> @llvm.genx.vload.v8i8.p0v8i8(<8 x i8>* nonnull @Src2Scale)
  %24 = tail call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %20, <128 x i32> %21, <64 x i32> %22, <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, <8 x i8> %23, i32 11, i32 11, i32 8, i32 8)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %24, <128 x float>* nonnull @Acc)
  %25 = tail call <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>* nonnull @Acc)
  %26 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %27 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %28 = tail call <16 x i8> @llvm.genx.vload.v16i8.p0v16i8(<16 x i8>* nonnull @Src1Scale)
  %29 = tail call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v16i8.v8i8(<128 x float> %25, <128 x i32> %26, <64 x i32> %27, <16 x i8> %28, <8 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, i32 11, i32 11, i32 8, i32 8)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %29, <128 x float>* nonnull @Acc)
  ret void
}

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nofree nosync nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!spirv.MemoryModel = !{!11}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!12}
!opencl.spir.version = !{!13}
!opencl.ocl.version = !{!12}
!opencl.used.extensions = !{!14}
!opencl.used.optional.core.features = !{!14}
!spirv.Generator = !{!15}
!genx.kernels = !{!16}
!genx.kernel.internal = !{!19}

!0 = !{!1, !2, !3, !4}
!1 = !{i32 21}
!2 = !{i32 44, i32 512}
!3 = !{i32 5624}
!4 = !{i32 5628, i32 0}
!5 = !{!1, !6, !3, !4}
!6 = !{i32 44, i32 256}
!7 = !{!1, !8, !3, !4}
!8 = !{i32 44, i32 16}
!9 = !{!1, !10, !3, !4}
!10 = !{i32 44, i32 8}
!11 = !{i32 2, i32 2}
!12 = !{i32 0, i32 0}
!13 = !{i32 1, i32 2}
!14 = !{}
!15 = !{i16 6, i16 14}
!16 = !{void (i64)* @kernel, !"kernel", !17, i32 0, !18, !14, !14, i32 0}
!17 = !{i32 96}
!18 = !{i32 128}
!19 = !{void (i64)* @kernel, !20, !20, !14, !21}
!20 = !{i32 0}
!21 = !{i32 255}
