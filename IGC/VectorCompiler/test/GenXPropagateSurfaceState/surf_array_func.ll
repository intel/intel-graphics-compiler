;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%intel.buffer_rw_t = type opaque

; Function Attrs: nofree nosync nounwind readnone
declare !genx_intrinsic_id !17 i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) #4

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !18 <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1>, i8, i8, i8, <3 x i8>, i32, i32, i16, i32, <8 x i32>) #5

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !19 void @llvm.vc.internal.lsc.store.bti.v1i1.v3i8.i32.v8i32(<1 x i1>, i8, i8, i8, <3 x i8>, i32, i32, i16, i32, <8 x i32>) #6

; Function Attrs: mustprogress nofree noinline nounwind readonly willreturn
define internal spir_func <8 x i32> @_Z3baru2CMvb8_15cm_surfaceindexi(<8 x i32> %0) unnamed_addr #2 {
; CHECK: [[RDREG:%[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %0, i32 0, i32 1, i32 1, i16 40, i32 0)
; CHECK: [[TRUNC:%[^ ]+]] = trunc i64 [[RDREG]] to i32
; CHECK: call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 [[TRUNC]], i32 32, i16 1, i32 0, <8 x i32> undef)
  %sev.cast.14.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %0, i32 0, i32 1, i32 1, i16 20, i32 0)
  %2 = call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %sev.cast.14.regioncollapsed, i32 32, i16 1, i32 0, <8 x i32> undef)
  ret <8 x i32> %2
}

declare i64 @llvm.vc.internal.optimization.fence.i64(i64)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @foo(%intel.buffer_rw_t addrspace(1)* %0, %intel.buffer_rw_t addrspace(1)* %1, %intel.buffer_rw_t addrspace(1)* %2, i64 %impl.arg.private.base, i64 %impl.arg.indirect.data.buffer, i64 %impl.arg.scratch.buffer) local_unnamed_addr #3 {
; CHECK-TYPED-PTRS: [[SURF64_PTR0:%[^ ]+]] = ptrtoint %intel.buffer_rw_t addrspace(1)* %0 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR0:%[^ ]+]] = ptrtoint ptr addrspace(1) %0 to i64
; CHECK: [[FENCE_PTR0:%[^ ]+]] = call i64 @llvm.vc.internal.optimization.fence.i64(i64 [[SURF64_PTR0]])
; CHECK: [[SURF32_PTR0:%[^ ]+]] = trunc i64 [[FENCE_PTR0]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR1:%[^ ]+]] = ptrtoint %intel.buffer_rw_t addrspace(1)* %1 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR1:%[^ ]+]] = ptrtoint ptr addrspace(1) %1 to i64
; CHECK: [[SURF32_PTR1:%[^ ]+]] = trunc i64 [[SURF64_PTR1]] to i32
  %4 = ptrtoint %intel.buffer_rw_t addrspace(1)* %0 to i64
  %fence = call i64 @llvm.vc.internal.optimization.fence.i64(i64 %4)
  %5 = trunc i64 %fence to i32
  %6 = ptrtoint %intel.buffer_rw_t addrspace(1)* %1 to i64
  %7 = trunc i64 %6 to i32
  %8 = ptrtoint %intel.buffer_rw_t addrspace(1)* %2 to i64
  %9 = trunc i64 %8 to i32
; CHECK: [[INS0_64:%[^ ]+]] = insertelement <8 x i64> undef, i64 [[FENCE_PTR0]], i64 0
; CHECK: [[INS0_32:%[^ ]+]] = insertelement <8 x i32> undef, i32 [[SURF32_PTR0]], i64 0
; CHECK: [[INS1_64:%[^ ]+]] = insertelement <8 x i64> [[INS0_64]], i64 [[SURF64_PTR1]], i64 1
; CHECK: [[INS1_32:%[^ ]+]] = insertelement <8 x i32> [[INS0_32]], i32 [[SURF32_PTR1]], i64 1
; CHECK: [[INS2_64:%[^ ]+]] = insertelement <8 x i64> [[INS1_64]], i64 [[FENCE_PTR0]], i64 2
; CHECK: [[INS2_32:%[^ ]+]] = insertelement <8 x i32> [[INS1_32]], i32 [[SURF32_PTR0]], i64 2
; CHECK: [[INS3_64:%[^ ]+]] = insertelement <8 x i64> [[INS2_64]], i64 [[SURF64_PTR1]], i64 3
; CHECK: [[INS3_32:%[^ ]+]] = insertelement <8 x i32> [[INS2_32]], i32 [[SURF32_PTR1]], i64 3
; CHECK: [[INS4_64:%[^ ]+]] = insertelement <8 x i64> [[INS3_64]], i64 [[FENCE_PTR0]], i64 4
; CHECK: [[INS4_32:%[^ ]+]] = insertelement <8 x i32> [[INS3_32]], i32 [[SURF32_PTR0]], i64 4
; CHECK: [[INS5_64:%[^ ]+]] = insertelement <8 x i64> [[INS4_64]], i64 [[SURF64_PTR1]], i64 5
; CHECK: [[INS5_32:%[^ ]+]] = insertelement <8 x i32> [[INS4_32]], i32 [[SURF32_PTR1]], i64 5
; CHECK: [[INS6_64:%[^ ]+]] = insertelement <8 x i64> [[INS5_64]], i64 [[FENCE_PTR0]], i64 6
; CHECK: [[INS6_32:%[^ ]+]] = insertelement <8 x i32> [[INS5_32]], i32 [[SURF32_PTR0]], i64 6
; CHECK: [[INS7_64:%[^ ]+]] = insertelement <8 x i64> [[INS6_64]], i64 [[SURF64_PTR1]], i64 7
; CHECK: [[INS7_32:%[^ ]+]] = insertelement <8 x i32> [[INS6_32]], i32 [[SURF32_PTR1]], i64 7
  %10 = insertelement <8 x i32> undef, i32 %5, i64 0
  %11 = insertelement <8 x i32> %10, i32 %7, i64 1
  %12 = insertelement <8 x i32> %11, i32 %5, i64 2
  %13 = insertelement <8 x i32> %12, i32 %7, i64 3
  %14 = insertelement <8 x i32> %13, i32 %5, i64 4
  %15 = insertelement <8 x i32> %14, i32 %7, i64 5
  %16 = insertelement <8 x i32> %15, i32 %5, i64 6
  %17 = insertelement <8 x i32> %16, i32 %7, i64 7
; CHECK: call <8 x i32> @_Z3baru2CMvb8_15cm_surfaceindexi(<8 x i64> [[INS7_64]])
  %18 = tail call spir_func <8 x i32> @_Z3baru2CMvb8_15cm_surfaceindexi(<8 x i32> %17) #9
  call void @llvm.vc.internal.lsc.store.bti.v1i1.v3i8.i32.v8i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %9, i32 16, i16 1, i32 0, <8 x i32> %18)
  ret void
}

attributes #0 = { nofree nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { mustprogress nofree noinline nounwind readonly willreturn }
attributes #3 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #4 = { nofree nosync nounwind readnone }
attributes #5 = { nounwind readonly }
attributes #6 = { nounwind writeonly }
attributes #7 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #8 = { nounwind readnone }
attributes #9 = { noinline nounwind }

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
!6 = !{void (%intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, i64, i64, i64)* @foo, !"foo", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 2, i32 2, i32 2, i32 96, i32 144, i32 152}
!8 = !{i32 200, i32 208, i32 216, i32 192, i32 128, i32 136}
!9 = !{i32 0, i32 0, i32 0}
!10 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write"}
!11 = !{void (%intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, i64, i64, i64)* @foo, !12, !13, !4, !14}
!12 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!13 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5}
!14 = !{i32 1, i32 2, i32 3, i32 255, i32 255, i32 255}
!15 = !{!"clang version 14.0.5"}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{i32 10986}
!18 = !{i32 11220}
!19 = !{i32 11248}
!20 = !{i32 10888}
!21 = !{i32 10989}
!22 = !{i32 11191}
!23 = !{i32 10775}
!24 = !{i32 10773}
!25 = !{i32 10984}
!26 = !{i32 10788}
!27 = !{i32 10985}
!28 = !{i32 11192}
!29 = !{i32 10994}
!30 = !{i32 10890}
!31 = !{i32 11013}
!32 = !{i32 11014}
!33 = !{i32 10781}
!34 = !{i32 11190}
!35 = !{i32 11048}
!36 = !{i32 10769}
!37 = !{i32 10789}
!38 = !{i32 10767}
!39 = !{i32 11035}
!40 = !{i32 11259}
!41 = !{i32 11228}
!42 = !{i32 11208}
!43 = !{i32 11152}
!44 = !{i32 11150}
!45 = !{i32 11230}
!46 = !{i32 11210}
