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



; CHECK: .decl [[ACCSTORAGE:V[0-9]+]] v_type=G type=f num_elts=128 align=GRF
; CHECK: .decl [[SRC1:V[0-9]+]] v_type=G type=d num_elts=128 align=GRF
; CHECK: .decl [[SRC2:V[0-9]+]] v_type=G type=d num_elts=64 align=GRF
; CHECK: .decl [[ACC:V[0-9]+]] v_type=G type=f num_elts=128 align=wordx32 alias=<[[ACCSTORAGE]], 0>

; CHECK: dpas.e2m1.e2m1.8.8 (M1, 16) [[ACC]].0 [[ACC]].0 [[SRC1]].0 [[SRC2]](0,0)

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@Acc = internal global <128 x float> undef, align 512, !spirv.Decorations !0 #0
@Src1 = internal global <128 x i32> undef, align 512, !spirv.Decorations !0 #0
@Src2 = internal global <64 x i32> undef, align 256, !spirv.Decorations !5 #0

declare <128 x float> @llvm.genx.dpas.nosrc0.v128f32.v128i32.v64i32(<128 x i32>, <64 x i32>, i32) #1
declare <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32) #1

declare void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float>, <128 x float>*) #2
declare <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>*) #2
declare <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>*) #2
declare <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>*) #2

define dllexport spir_kernel void @kernel(i64 %impl.arg.private.base) local_unnamed_addr #3 {
entry:
  %gload5 = tail call <128 x float> @llvm.genx.vload.v128f32.p0v128f32(<128 x float>* nonnull @Acc)
  %gload9 = tail call <128 x i32> @llvm.genx.vload.v128i32.p0v128i32(<128 x i32>* nonnull @Src1)
  %gload13 = tail call <64 x i32> @llvm.genx.vload.v64i32.p0v64i32(<64 x i32>* nonnull @Src2)
  %call1.i.i = tail call <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float> %gload5, <128 x i32> %gload9, <64 x i32> %gload13, i32 15, i32 15, i32 8, i32 8, i32 0, i32 0)
  tail call void @llvm.genx.vstore.v128f32.p0v128f32(<128 x float> %call1.i.i, <128 x float>* nonnull @Acc)
  ret void
}

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nofree nosync nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!spirv.MemoryModel = !{!7}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!8}
!opencl.spir.version = !{!9}
!opencl.ocl.version = !{!8}
!opencl.used.extensions = !{!10}
!opencl.used.optional.core.features = !{!10}
!spirv.Generator = !{!11}
!genx.kernels = !{!12}
!genx.kernel.internal = !{!15}

!0 = !{!1, !2, !3, !4}
!1 = !{i32 21}
!2 = !{i32 44, i32 512}
!3 = !{i32 5624}
!4 = !{i32 5628, i32 0}
!5 = !{!1, !6, !3, !4}
!6 = !{i32 44, i32 256}
!7 = !{i32 2, i32 2}
!8 = !{i32 0, i32 0}
!9 = !{i32 1, i32 2}
!10 = !{}
!11 = !{i16 6, i16 14}
!12 = !{void (i64)* @kernel, !"kernel", !13, i32 0, !14, !10, !10, i32 0}
!13 = !{i32 96}
!14 = !{i32 128}
!15 = !{void (i64)* @kernel, !16, !16, !10, !17}
!16 = !{i32 0}
!17 = !{i32 255}
