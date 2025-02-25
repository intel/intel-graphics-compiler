;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -loop-unroll -vc-peel-loops-dpas-null-acc=true -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -loop-unroll -vc-peel-loops-dpas-null-acc=true -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; Function Attrs: nofree nosync nounwind readnone
declare <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nofree nosync nounwind readnone
declare <128 x i32> @llvm.genx.rdregioni.v128i32.v512i32.i16(<512 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nofree nosync nounwind readnone
declare <512 x i32> @llvm.genx.wrregioni.v512i32.v128i32.i16.i1(<512 x i32>, <128 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nofree nosync nounwind readnone
declare <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32) #0

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i8 addrspace(1)* %2) local_unnamed_addr #1 !spirv.ParameterDecorations !8 !intel_reqd_sub_group_size !11 {
  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>
  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>
  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>
  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>
  %4 = ptrtoint i8 addrspace(1)* %1 to i64
  %5 = ptrtoint i8 addrspace(1)* %2 to i64
  br label %6

6:                                                ; preds = %6, %3
  ; CHECK: phi <512 x i32>

  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>
  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>
  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>
  ; CHECK: call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>

  %indvars.iv159 = phi i64 [ 0, %3 ], [ %indvars.iv.next160, %6 ]
  %indvars.iv = phi i64 [ 0, %3 ], [ %indvars.iv.next, %6 ]
  %.0140155 = phi i32 [ 0, %3 ], [ %23, %6 ]
  %phiacc = phi <512 x i32> [ zeroinitializer, %3 ], [ %dst3, %6 ]
  %7 = shl nsw i64 %indvars.iv159, 2
  %8 = add i64 %7, %4
  %9 = inttoptr i64 %8 to <256 x i32> addrspace(1)*
  %10 = load <256 x i32>, <256 x i32> addrspace(1)* %9, align 16
  %11 = shl nsw i64 %indvars.iv, 2
  %12 = add i64 %11, %5
  %13 = inttoptr i64 %12 to <128 x i32> addrspace(1)*
  %14 = load <128 x i32>, <128 x i32> addrspace(1)* %13, align 16
  %indvars.iv.next160 = add nuw nsw i64 %indvars.iv159, 256
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 128

  %15 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 0, i32 8)
  %acc0 = call <128 x i32> @llvm.genx.rdregioni.v128i32.v512i32.i16(<512 x i32> %phiacc, i32 1, i32 1, i32 0, i16 0, i32 0)
  %16 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %acc0, <128 x i32> %14, <64 x i32> %15, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %dst0 = call <512 x i32> @llvm.genx.wrregioni.v512i32.v128i32.i16.i1(<512 x i32> %phiacc, <128 x i32> %16, i32 1, i32 1, i32 0, i16 0, i32 0, i1 true)

  %17 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 256, i32 8)
  %acc1 = call <128 x i32> @llvm.genx.rdregioni.v128i32.v512i32.i16(<512 x i32> %dst0, i32 1, i32 1, i32 0, i16 512, i32 0)
  %18 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %acc1, <128 x i32> %14, <64 x i32> %15, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %dst1 = call <512 x i32> @llvm.genx.wrregioni.v512i32.v128i32.i16.i1(<512 x i32> %dst0, <128 x i32> %18, i32 1, i32 1, i32 0, i16 512, i32 0, i1 true)

  %19 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 512, i32 8)
  %acc2 = call <128 x i32> @llvm.genx.rdregioni.v128i32.v512i32.i16(<512 x i32> %dst1, i32 1, i32 1, i32 0, i16 1024, i32 0)
  %20 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %acc2, <128 x i32> %14, <64 x i32> %15, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %dst2 = call <512 x i32> @llvm.genx.wrregioni.v512i32.v128i32.i16.i1(<512 x i32> %dst1, <128 x i32> %20, i32 1, i32 1, i32 0, i16 1024, i32 0, i1 true)

  %21 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 768, i32 8)
  %acc3 = call <128 x i32> @llvm.genx.rdregioni.v128i32.v512i32.i16(<512 x i32> %dst2, i32 1, i32 1, i32 0, i16 1536, i32 0)
  %22 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %acc3, <128 x i32> %14, <64 x i32> %15, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %dst3 = call <512 x i32> @llvm.genx.wrregioni.v512i32.v128i32.i16.i1(<512 x i32> %dst2, <128 x i32> %22, i32 1, i32 1, i32 0, i16 1536, i32 0, i1 true)

  %23 = add nuw nsw i32 %.0140155, 1
  %exitcond.not = icmp eq i32 %23, 16
  br i1 %exitcond.not, label %24, label %6

24:                                               ; preds = %6
  %res = phi <512 x i32> [ %dst3, %6 ]
  %25 = ptrtoint i8 addrspace(1)* %0 to i64
  %26 = bitcast i8 addrspace(1)* %0 to <512 x i32> addrspace(1)*
  store <512 x i32> %res, <512 x i32> addrspace(1)* %26, align 16
  ret void
}

attributes #0 = { nofree nosync nounwind readnone "target-cpu"="XeHPC" }
attributes #1 = { noinline nounwind "CMGenxMain" "oclrt"="1" "target-cpu"="XeHPC" }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*)* @kernel, !"kernel", !6, i32 0, i32 0, !6, !7, i32 0}
!6 = !{i32 0, i32 0, i32 0}
!7 = !{!"svmptr_t", !"svmptr_t", !"svmptr_t"}
!8 = !{!9, !9, !9}
!9 = !{!10}
!10 = !{i32 5625, i32 0}
!11 = !{i32 1}
