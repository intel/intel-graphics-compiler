;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
target triple = "genx64-unknown-unknown"

%intel.buffer_rw_t = type opaque

; Function Attrs: nofree nounwind readonly
declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32) #0

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>) #1

; CHECK: @foo
define dllexport spir_kernel void @foo(%intel.buffer_rw_t addrspace(1)* %0, %intel.buffer_rw_t addrspace(1)* %1, %intel.buffer_rw_t addrspace(1)* %2, i32 %3, i64 %impl.arg.private.base, i64 %impl.arg.indirect.data.buffer, i64 %impl.arg.scratch.buffer) local_unnamed_addr #2 {
; CHECK-TYPED-PTRS: [[SURF64_PTR0:%[^ ]+]] = ptrtoint %intel.buffer_rw_t addrspace(1)* %0 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR0:%[^ ]+]] = ptrtoint ptr addrspace(1) %0 to i64
; CHECK: [[SURF32_PTR0:%[^ ]+]] = trunc i64 [[SURF64_PTR0]] to i32
; CHECK-TYPED-PTRS: [[SURF64_PTR1:%[^ ]+]] = ptrtoint %intel.buffer_rw_t addrspace(1)* %1 to i64
; CHECK-OPAQUE-PTRS: [[SURF64_PTR1:%[^ ]+]] = ptrtoint ptr addrspace(1) %1 to i64
; CHECK: [[SURF32_PTR1:%[^ ]+]] = trunc i64 [[SURF64_PTR1]] to i32
  %5 = ptrtoint %intel.buffer_rw_t addrspace(1)* %0 to i64
  %6 = trunc i64 %5 to i32
  %7 = ptrtoint %intel.buffer_rw_t addrspace(1)* %1 to i64
  %8 = trunc i64 %7 to i32
  %9 = ptrtoint %intel.buffer_rw_t addrspace(1)* %2 to i64
  %10 = trunc i64 %9 to i32

; CHECK: [[INS0_64:%[^ ]+]] = insertelement <8 x i64> undef, i64 [[SURF64_PTR0]], i64 0
; CHECK: [[INS0_32:%[^ ]+]] = insertelement <8 x i32> undef, i32 [[SURF32_PTR0]], i64 0
; CHECK: [[INS1_64:%[^ ]+]] = insertelement <8 x i64> [[INS0_64]], i64 [[SURF64_PTR1]], i64 1
; CHECK: [[INS1_32:%[^ ]+]] = insertelement <8 x i32> [[INS0_32]], i32 [[SURF32_PTR1]], i64 1
; CHECK: [[INS2_64:%[^ ]+]] = insertelement <8 x i64> [[INS1_64]], i64 [[SURF64_PTR0]], i64 2
; CHECK: [[INS2_32:%[^ ]+]] = insertelement <8 x i32> [[INS1_32]], i32 [[SURF32_PTR0]], i64 2
; CHECK: [[INS3_64:%[^ ]+]] = insertelement <8 x i64> [[INS2_64]], i64 [[SURF64_PTR1]], i64 3
; CHECK: [[INS3_32:%[^ ]+]] = insertelement <8 x i32> [[INS2_32]], i32 [[SURF32_PTR1]], i64 3
; CHECK: [[INS4_64:%[^ ]+]] = insertelement <8 x i64> [[INS3_64]], i64 [[SURF64_PTR0]], i64 4
; CHECK: [[INS4_32:%[^ ]+]] = insertelement <8 x i32> [[INS3_32]], i32 [[SURF32_PTR0]], i64 4
; CHECK: [[INS5_64:%[^ ]+]] = insertelement <8 x i64> [[INS4_64]], i64 [[SURF64_PTR1]], i64 5
; CHECK: [[INS5_32:%[^ ]+]] = insertelement <8 x i32> [[INS4_32]], i32 [[SURF32_PTR1]], i64 5
; CHECK: [[INS6_64:%[^ ]+]] = insertelement <8 x i64> [[INS5_64]], i64 [[SURF64_PTR0]], i64 6
; CHECK: [[INS6_32:%[^ ]+]] = insertelement <8 x i32> [[INS5_32]], i32 [[SURF32_PTR0]], i64 6
; CHECK: [[INS7_64:%[^ ]+]] = insertelement <8 x i64> [[INS6_64]], i64 [[SURF64_PTR1]], i64 7
; CHECK: [[INS7_32:%[^ ]+]] = insertelement <8 x i32> [[INS6_32]], i32 [[SURF32_PTR1]], i64 7

  %11 = insertelement <8 x i32> undef, i32 %6, i64 0
  %12 = insertelement <8 x i32> %11, i32 %8, i64 1
  %13 = insertelement <8 x i32> %12, i32 %6, i64 2
  %14 = insertelement <8 x i32> %13, i32 %8, i64 3
  %15 = insertelement <8 x i32> %14, i32 %6, i64 4
  %16 = insertelement <8 x i32> %15, i32 %8, i64 5
  %17 = insertelement <8 x i32> %16, i32 %6, i64 6
  %18 = insertelement <8 x i32> %17, i32 %8, i64 7
  %19 = icmp sgt i32 %3, 0
  br i1 %19, label %..lr.ph_crit_edge, label %.._crit_edge_crit_edge

.._crit_edge_crit_edge:                           ; preds = %4
  br label %._crit_edge

..lr.ph_crit_edge:                                ; preds = %4
  br label %.lr.ph

.lr.ph:                                           ; preds = %.lr.ph..lr.ph_crit_edge, %..lr.ph_crit_edge
  %.031 = phi i32 [ %25, %.lr.ph..lr.ph_crit_edge ], [ 0, %..lr.ph_crit_edge ]
  %20 = trunc i32 %.031 to i16
  %21 = shl i16 %20, 2
; CHECK: [[MUL_DST:%[^ ]+]] = mul i16 [[MUL_SRC:%[^ ]+]], 2
; CHECK: [[SURF64_PTR:%[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> [[INS7_64]], i32 0, i32 1, i32 1, i16 [[MUL_DST]], i32 0)
; CHECK: tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> [[INS7_32]], i32 0, i32 1, i32 1, i16 [[MUL_SRC]], i32 0)
  %sev.cast.32.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %18, i32 0, i32 1, i32 1, i16 %21, i32 0)
  %22 = shl i32 %.031, 5
; CHECK: [[SURF32_PTR:%[^ ]+]] = trunc i64 [[SURF64_PTR]] to i32
; CHECK: call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 [[SURF32_PTR]], i32 %{{.+}}, i16 1, i32 0, <8 x i32> undef)
  %23 = call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %sev.cast.32.regioncollapsed, i32 %22, i16 1, i32 0, <8 x i32> undef)
  %24 = shl i32 %.031, 4
  call void @llvm.vc.internal.lsc.store.bti.v1i1.v3i8.i32.v8i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %10, i32 %24, i16 1, i32 0, <8 x i32> %23)
  %25 = add nuw nsw i32 %.031, 1, !spirv.Decorations !14
  %exitcond.not = icmp eq i32 %25, %3
  br i1 %exitcond.not, label %.lr.ph.._crit_edge_crit_edge, label %.lr.ph..lr.ph_crit_edge

.lr.ph..lr.ph_crit_edge:                          ; preds = %.lr.ph
  br label %.lr.ph

.lr.ph.._crit_edge_crit_edge:                     ; preds = %.lr.ph
  br label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph.._crit_edge_crit_edge, %.._crit_edge_crit_edge
  ret void
}

; Function Attrs: nofree nosync nounwind readnone
declare !genx_intrinsic_id !16 i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) #3

; Function Attrs: nounwind readonly
declare !internal_intrinsic_id !17 <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1>, i8, i8, i8, <3 x i8>, i32, i32, i16, i32, <8 x i32>) #4

; Function Attrs: nounwind writeonly
declare !internal_intrinsic_id !18 void @llvm.vc.internal.lsc.store.bti.v1i1.v3i8.i32.v8i32(<1 x i1>, i8, i8, i8, <3 x i8>, i32, i32, i16, i32, <8 x i32>) #5

attributes #0 = { nofree nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #3 = { nofree nosync nounwind readnone }
attributes #4 = { nounwind readonly }
attributes #5 = { nounwind writeonly }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!genx.kernel.internal = !{!10}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (%intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, i32, i64, i64, i64)* @foo, !"foo", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 2, i32 2, i32 2, i32 0, i32 96, i32 144, i32 152}
!7 = !{i32 200, i32 208, i32 216, i32 224, i32 192, i32 128, i32 136}
!8 = !{i32 0, i32 0, i32 0, i32 0}
!9 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !""}
!10 = !{void (%intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, %intel.buffer_rw_t addrspace(1)*, i32, i64, i64, i64)* @foo, !11, !12, !3, !13}
!11 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!12 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
!13 = !{i32 1, i32 2, i32 3, i32 -1, i32 255, i32 255, i32 255}
!14 = !{!15}
!15 = !{i32 4469}
!16 = !{i32 10986}
!17 = !{i32 11220}
!18 = !{i32 11248}
