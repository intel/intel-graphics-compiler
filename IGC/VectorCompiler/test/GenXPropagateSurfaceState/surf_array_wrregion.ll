;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
target triple = "genx64-unknown-unknown"

%intel.image2d_media_block_rw_t.0 = type opaque

; Function Attrs: nofree nosync nounwind readnone
declare !spirv.ParameterDecorations !17 <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: noinline nounwind
; CHECK: @foo
define dllexport spir_kernel void @foo(%intel.image2d_media_block_rw_t.0 addrspace(1)* %0, %intel.image2d_media_block_rw_t.0 addrspace(1)* %1, %intel.image2d_media_block_rw_t.0 addrspace(1)* %2, %intel.image2d_media_block_rw_t.0 addrspace(1)* %3, %intel.image2d_media_block_rw_t.0 addrspace(1)* %4, %intel.image2d_media_block_rw_t.0 addrspace(1)* %5, %intel.image2d_media_block_rw_t.0 addrspace(1)* %6, %intel.image2d_media_block_rw_t.0 addrspace(1)* %7, i32 %8, i64 %impl.arg.private.base, i64 %impl.arg.indirect.data.buffer, i64 %impl.arg.scratch.buffer) local_unnamed_addr #3 {
; CHECK-TYPED-PTRS: [[SURF64_PTR0:%[^ ]+]] = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %0 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR0:%[^ ]+]] = ptrtoint ptr addrspace(1) %0 to i64
; CHECK: [[SURF32_PTR0:%[^ ]+]] = trunc i64 [[SURF64_PTR0]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR1:%[^ ]+]] = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %1 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR1:%[^ ]+]] = ptrtoint ptr addrspace(1) %1 to i64
; CHECK: [[SURF32_PTR1:%[^ ]+]] = trunc i64 [[SURF64_PTR1]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR2:%[^ ]+]] = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %2 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR2:%[^ ]+]] = ptrtoint ptr addrspace(1) %2 to i64
; CHECK: [[SURF32_PTR2:%[^ ]+]] = trunc i64 [[SURF64_PTR2]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR3:%[^ ]+]] = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %3 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR3:%[^ ]+]] = ptrtoint ptr addrspace(1) %3 to i64
; CHECK: [[SURF32_PTR3:%[^ ]+]] = trunc i64 [[SURF64_PTR3]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR4:%[^ ]+]] = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %4 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR4:%[^ ]+]] = ptrtoint ptr addrspace(1) %4 to i64
; CHECK: [[SURF32_PTR4:%[^ ]+]] = trunc i64 [[SURF64_PTR4]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR5:%[^ ]+]] = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %5 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR5:%[^ ]+]] = ptrtoint ptr addrspace(1) %5 to i64
; CHECK: [[SURF32_PTR5:%[^ ]+]] = trunc i64 [[SURF64_PTR5]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR6:%[^ ]+]] = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %6 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR6:%[^ ]+]] = ptrtoint ptr addrspace(1) %6 to i64
; CHECK: [[SURF32_PTR6:%[^ ]+]] = trunc i64 [[SURF64_PTR6]] to i32

  %10 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %0 to i64
  %11 = trunc i64 %10 to i32
  %12 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %1 to i64
  %13 = trunc i64 %12 to i32
  %14 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %2 to i64
  %15 = trunc i64 %14 to i32
  %16 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %3 to i64
  %17 = trunc i64 %16 to i32
  %18 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %4 to i64
  %19 = trunc i64 %18 to i32
  %20 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %5 to i64
  %21 = trunc i64 %20 to i32
  %22 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %6 to i64
  %23 = trunc i64 %22 to i32
  %24 = ptrtoint %intel.image2d_media_block_rw_t.0 addrspace(1)* %7 to i64
  %25 = trunc i64 %24 to i32

; CHECK: [[INS0_64:%[^ ]+]] = insertelement <1 x i64> undef, i64 [[SURF64_PTR0]], i64 0
; CHECK: [[INS0_32:%[^ ]+]] = insertelement <1 x i32> undef, i32 [[SURF32_PTR0]], i64 0
; CHECK: [[WRRGN0_64:%[^ ]+]] = call <7 x i64> @llvm.genx.wrregioni.v7i64.v1i64.i16.i1(<7 x i64> undef, <1 x i64> [[INS0_64]], i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: [[WRRGN0_32:%[^ ]+]] = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> undef, <1 x i32> [[INS0_32]], i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)

  %sev.cast.6 = insertelement <1 x i32> undef, i32 %11, i64 0
  %26 = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> undef, <1 x i32> %sev.cast.6, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)

; CHECK: [[INS1_64:%[^ ]+]] = insertelement <1 x i64> undef, i64 [[SURF64_PTR1]], i64 0
; CHECK: [[INS1_32:%[^ ]+]] = insertelement <1 x i32> undef, i32 [[SURF32_PTR1]], i64 0
; CHECK: [[WRRGN1_64:%[^ ]+]] = call <7 x i64> @llvm.genx.wrregioni.v7i64.v1i64.i16.i1(<7 x i64> [[WRRGN0_64]], <1 x i64> [[INS1_64]], i32 0, i32 1, i32 0, i16 8, i32 undef, i1 true)
; CHECK: [[WRRGN1_32:%[^ ]+]] = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[WRRGN0_32]], <1 x i32> [[INS1_32]], i32 0, i32 1, i32 0, i16 4, i32 undef, i1 true)
  %sev.cast.5 = insertelement <1 x i32> undef, i32 %13, i64 0
  %27 = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> %26, <1 x i32> %sev.cast.5, i32 0, i32 1, i32 0, i16 4, i32 undef, i1 true)

; CHECK: [[INS2_64:%[^ ]+]] = insertelement <1 x i64> undef, i64 [[SURF64_PTR2]], i64 0
; CHECK: [[INS2_32:%[^ ]+]] = insertelement <1 x i32> undef, i32 [[SURF32_PTR2]], i64 0
; CHECK: [[WRRGN2_64:%[^ ]+]] = call <7 x i64> @llvm.genx.wrregioni.v7i64.v1i64.i16.i1(<7 x i64> [[WRRGN1_64]], <1 x i64> [[INS2_64]], i32 0, i32 1, i32 0, i16 16, i32 undef, i1 true)
; CHECK: [[WRRGN2_32:%[^ ]+]] = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[WRRGN1_32]], <1 x i32> [[INS2_32]], i32 0, i32 1, i32 0, i16 8, i32 undef, i1 true)
  %sev.cast.4 = insertelement <1 x i32> undef, i32 %15, i64 0
  %28 = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> %27, <1 x i32> %sev.cast.4, i32 0, i32 1, i32 0, i16 8, i32 undef, i1 true)

; CHECK: [[INS3_64:%[^ ]+]] = insertelement <1 x i64> undef, i64 [[SURF64_PTR3]], i64 0
; CHECK: [[INS3_32:%[^ ]+]] = insertelement <1 x i32> undef, i32 [[SURF32_PTR3]], i64 0
; CHECK: [[WRRGN3_64:%[^ ]+]] = call <7 x i64> @llvm.genx.wrregioni.v7i64.v1i64.i16.i1(<7 x i64> [[WRRGN2_64]], <1 x i64> [[INS3_64]], i32 0, i32 1, i32 0, i16 24, i32 undef, i1 true)
; CHECK: [[WRRGN3_32:%[^ ]+]] = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[WRRGN2_32]], <1 x i32> [[INS3_32]], i32 0, i32 1, i32 0, i16 12, i32 undef, i1 true)
  %sev.cast.3 = insertelement <1 x i32> undef, i32 %17, i64 0
  %29 = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> %28, <1 x i32> %sev.cast.3, i32 0, i32 1, i32 0, i16 12, i32 undef, i1 true)

; CHECK: [[INS4_64:%[^ ]+]] = insertelement <1 x i64> undef, i64 [[SURF64_PTR4]], i64 0
; CHECK: [[INS4_32:%[^ ]+]] = insertelement <1 x i32> undef, i32 [[SURF32_PTR4]], i64 0
; CHECK: [[WRRGN4_64:%[^ ]+]] = call <7 x i64> @llvm.genx.wrregioni.v7i64.v1i64.i16.i1(<7 x i64> [[WRRGN3_64]], <1 x i64> [[INS4_64]], i32 0, i32 1, i32 0, i16 32, i32 undef, i1 true)
; CHECK: [[WRRGN4_32:%[^ ]+]] = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[WRRGN3_32]], <1 x i32> [[INS4_32]], i32 0, i32 1, i32 0, i16 16, i32 undef, i1 true)
  %sev.cast.2 = insertelement <1 x i32> undef, i32 %19, i64 0
  %30 = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> %29, <1 x i32> %sev.cast.2, i32 0, i32 1, i32 0, i16 16, i32 undef, i1 true)

; CHECK: [[INS5_64:%[^ ]+]] = insertelement <1 x i64> undef, i64 [[SURF64_PTR5]], i64 0
; CHECK: [[INS5_32:%[^ ]+]] = insertelement <1 x i32> undef, i32 [[SURF32_PTR5]], i64 0
; CHECK: [[WRRGN5_64:%[^ ]+]] = call <7 x i64> @llvm.genx.wrregioni.v7i64.v1i64.i16.i1(<7 x i64> [[WRRGN4_64]], <1 x i64> [[INS5_64]], i32 0, i32 1, i32 0, i16 40, i32 undef, i1 true)
; CHECK: [[WRRGN5_32:%[^ ]+]] = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[WRRGN4_32]], <1 x i32> [[INS5_32]], i32 0, i32 1, i32 0, i16 20, i32 undef, i1 true)
  %sev.cast.1 = insertelement <1 x i32> undef, i32 %21, i64 0
  %31 = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> %30, <1 x i32> %sev.cast.1, i32 0, i32 1, i32 0, i16 20, i32 undef, i1 true)

; CHECK: [[INS6_64:%[^ ]+]] = insertelement <1 x i64> undef, i64 [[SURF64_PTR6]], i64 0
; CHECK: [[INS6_32:%[^ ]+]] = insertelement <1 x i32> undef, i32 [[SURF32_PTR6]], i64 0
; CHECK: [[WRRGN6_64:%[^ ]+]] = call <7 x i64> @llvm.genx.wrregioni.v7i64.v1i64.i16.i1(<7 x i64> [[WRRGN5_64]], <1 x i64> [[INS6_64]], i32 0, i32 1, i32 0, i16 48, i32 undef, i1 true)
; CHECK: [[WRRGN6_32:%[^ ]+]] = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[WRRGN5_32]], <1 x i32> [[INS6_32]], i32 0, i32 1, i32 0, i16 24, i32 undef, i1 true)
  %sev.cast. = insertelement <1 x i32> undef, i32 %23, i64 0
  %32 = tail call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> %31, <1 x i32> %sev.cast., i32 0, i32 1, i32 0, i16 24, i32 undef, i1 true)

; CHECK: [[ARR_IND:%[^ ]+]] = trunc i32 %8 to i16
; CHECK: [[ARR_IND32:%[^ ]+]] = shl i16 [[ARR_IND]], 2
; CHECK: [[ARR_IND64:%[^ ]+]] = mul i16 [[ARR_IND32]], 2
  %33 = trunc i32 %8 to i16
  %34 = shl i16 %33, 2

; CHECK: [[RDREG64:%[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v7i64.i16(<7 x i64> [[WRRGN6_64]], i32 0, i32 1, i32 1, i16 [[ARR_IND64]], i32 0)
; CHECK: [[RDREG32:%[^ ]+]] = tail call i32 @llvm.genx.rdregioni.i32.v7i32.i16(<7 x i32> [[WRRGN6_32]], i32 0, i32 1, i32 1, i16 [[ARR_IND32]], i32 0)
  %sev.cast.759.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v7i32.i16(<7 x i32> %32, i32 0, i32 1, i32 1, i16 %34, i32 0)

; CHECK: [[TRNK:%[^ ]+]] = trunc i64 [[RDREG64]] to i32
; CHECK: call <8 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v8i32.v3i8(<3 x i8> zeroinitializer, i32 [[TRNK]], i32 2, i32 4, i32 8, i32 4)
  %35 = call <8 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v8i32.v3i8(<3 x i8> zeroinitializer, i32 %sev.cast.759.regioncollapsed, i32 2, i32 4, i32 8, i32 4)
  call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v3i8.v8i32(<3 x i8> zeroinitializer, i32 %25, i32 2, i32 4, i32 4, i32 2, <8 x i32> %35)
  ret void
}

; Function Attrs: nofree nosync nounwind readnone
declare !genx_intrinsic_id !20 i32 @llvm.genx.rdregioni.i32.v7i32.i16(<7 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !21 <8 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v8i32.v3i8(<3 x i8>, i32, i32, i32, i32, i32) #4

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !22 void @llvm.vc.internal.lsc.store.2d.tgm.bti.v3i8.v8i32(<3 x i8>, i32, i32, i32, i32, i32, <8 x i32>) #5

attributes #0 = { nofree nosync nounwind readnone }
attributes #1 = { nofree nounwind readonly }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #4 = { nounwind readonly }
attributes #5 = { nounwind writeonly }
attributes #6 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #7 = { nounwind readnone }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!genx.kernel.internal = !{!11}
!llvm.ident = !{!15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15}
!llvm.module.flags = !{!16}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (%intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, i32, i64, i64, i64)* @foo, !"foo", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 0, i32 96, i32 144, i32 152}
!8 = !{i32 200, i32 208, i32 216, i32 224, i32 232, i32 240, i32 248, i32 256, i32 264, i32 192, i32 128, i32 136}
!9 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!10 = !{!"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !""}
!11 = !{void (%intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, %intel.image2d_media_block_rw_t.0 addrspace(1)*, i32, i64, i64, i64)* @foo, !12, !13, !4, !14, i32 0}
!12 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!13 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11}
!14 = !{i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 -1, i32 255, i32 255, i32 255}
!15 = !{!"clang version 14.0.5"}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{!4, !18, !4, !4, !4, !4, !4, !4}
!18 = !{!19}
!19 = !{i32 6085}
!20 = !{i32 10986}
!21 = !{i32 11214}
!22 = !{i32 11249}
!23 = !{i32 10888}
!24 = !{i32 10989}
!25 = !{i32 11191}
!26 = !{i32 10775}
!27 = !{i32 10781}
!28 = !{i32 10773}
!29 = !{i32 10984}
!30 = !{i32 10788}
!31 = !{i32 10985}
!32 = !{i32 11192}
!33 = !{i32 11280}
!34 = !{i32 11205}
!35 = !{i32 11013}
!36 = !{i32 11014}
!37 = !{i32 11190}
!38 = !{i32 11048}
!39 = !{i32 10769}
!40 = !{i32 10789}
!41 = !{i32 10767}
!42 = !{i32 10890}
!43 = !{i32 11035}
!44 = !{i32 11269}
!45 = !{i32 11232}
!46 = !{i32 11210}
!47 = !{i32 11152}
!48 = !{i32 11150}
!49 = !{i32 11234}
!50 = !{i32 11212}
