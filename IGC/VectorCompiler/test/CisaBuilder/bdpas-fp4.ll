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
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null 2>&1 | \
; RUN: FileCheck %s



; CHECK: .decl [[ACC:V[0-9]+]] v_type=G type=f num_elts=128 align=GRF
; CHECK: .decl [[SRC1:V[0-9]+]] v_type=G type=ud num_elts=128 align=wordx32
; CHECK: .decl [[SRC2:V[0-9]+]] v_type=G type=ud num_elts=64
; CHECK: .decl [[SCALE1:V[0-9]+]] v_type=G type=ub num_elts=64
; CHECK: .decl [[SCALE2:V[0-9]+]] v_type=G type=ub num_elts=64

; CHECK: bdpas.e2m1.e2m1.8.8 (M1, 16) [[ACC]].0 [[ACC]].0 [[SRC1]].0 [[SRC2]].0 [[SCALE1]](0,0) [[SCALE2]](0,16)

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@Acc = internal global <128 x float> undef, align 512, !spirv.Decorations !0 #0
@Src1 = internal global <128 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src2 = internal global <64 x i32> undef, align 256, !spirv.Decorations !5 #0
@Src1Scale = internal global <64 x i8> undef, align 64, !spirv.Decorations !7 #0
@Src2Scale = internal global <64 x i8> undef, align 64, !spirv.Decorations !7 #0

declare <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8>, i32, i32, i32, i16, i32) #1
declare <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8>, i32, i32, i32, i16, i32) #1

declare <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float>, <128 x i32>, <64 x i32>, <32 x i8>, <16 x i8>, i32, i32, i32, i32) #1

declare void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float>, <128 x float>*) #2
declare <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>*) #2
declare <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>*) #2
declare <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>*) #2
declare <64 x i8> @llvm.genx.vload.v64i8.p0v64i8(<64 x i8>*) #2

define dllexport spir_kernel void @kernel(i64 %impl.arg.private.base) local_unnamed_addr #3 {
entry:
  %gload = tail call <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>* nonnull @Acc)
  %gload25 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %gload31 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %gload36 = tail call <64 x i8> @llvm.genx.vload.v64i8.p0v64i8(<64 x i8>* nonnull @Src1Scale)
  %rdr.cols10 = tail call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %gload36, i32 32, i32 16, i32 1, i16 0, i32 32)
  %gload40 = tail call <64 x i8> @llvm.genx.vload.v64i8.p0v64i8(<64 x i8>* nonnull @Src2Scale)
  %rdr.cols12 = tail call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> %gload40, i32 32, i32 8, i32 1, i16 16, i32 32)
  %call1.i.i77 = tail call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %gload, <128 x i32> %gload25, <64 x i32> %gload31, <32 x i8> %rdr.cols10, <16 x i8> %rdr.cols12, i32 15, i32 15, i32 8, i32 8)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %call1.i.i77, <128 x float>* nonnull @Acc)
  ret void
}

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nofree nosync nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!spirv.MemoryModel = !{!9}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!10}
!opencl.spir.version = !{!11}
!opencl.ocl.version = !{!10}
!opencl.used.extensions = !{!12}
!opencl.used.optional.core.features = !{!12}
!spirv.Generator = !{!13}
!genx.kernels = !{!14}
!genx.kernel.internal = !{!17}

!0 = !{!1, !2, !3, !4}
!1 = !{i32 21}
!2 = !{i32 44, i32 512}
!3 = !{i32 5624}
!4 = !{i32 5628, i32 0}
!5 = !{!1, !6, !3, !4}
!6 = !{i32 44, i32 256}
!7 = !{!1, !8, !3, !4}
!8 = !{i32 44, i32 64}
!9 = !{i32 2, i32 2}
!10 = !{i32 0, i32 0}
!11 = !{i32 1, i32 2}
!12 = !{}
!13 = !{i16 6, i16 14}
!14 = !{void (i64)* @kernel, !"kernel", !15, i32 0, !16, !12, !12, i32 0}
!15 = !{i32 96}
!16 = !{i32 128}
!17 = !{void (i64)* @kernel, !18, !18, !12, !19}
!18 = !{i32 0}
!19 = !{i32 255}
