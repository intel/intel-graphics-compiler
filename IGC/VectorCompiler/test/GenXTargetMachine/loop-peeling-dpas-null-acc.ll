;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -loop-unroll -vc-peel-loops-dpas-null-acc=true -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32) #0

define dllexport spir_kernel void @kernel(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i8 addrspace(1)* %2) #1 !spirv.ParameterDecorations !8 !intel_reqd_sub_group_size !11 {
; CHECK: [[PEEL:%[^ ]+]] = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> zeroinitializer, <128 x i32> %{{[^,]+}}, <64 x i32> %{{[^,]+}}, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %4 = ptrtoint i8 addrspace(1)* %1 to i64
  %5 = ptrtoint i8 addrspace(1)* %2 to i64
  br label %6

; CHECK: [[PHI:%[^ ]+]] = phi <128 x i32> [ [[PEEL]], %{{[^ ]+}} ],  [ [[ACC:%[^ ]+]], %{{[^ ]+}} ]
; CHECK: [[ACC]] = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> [[PHI]], <128 x i32> %{{[^,]+}}, <64 x i32> %{{[^,]+}}, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
6:                                                ; preds = %6, %3
  %indvars.iv60 = phi i64 [ 0, %3 ], [ %indvars.iv.next61, %6 ]
  %indvars.iv = phi i64 [ 0, %3 ], [ %indvars.iv.next, %6 ]
  %.05259 = phi <128 x i32> [ zeroinitializer, %3 ], [ %15, %6 ]
  %.05358 = phi i32 [ 0, %3 ], [ %16, %6 ]
  %7 = shl nsw i64 %indvars.iv60, 2
  %8 = add i64 %7, %4
  %9 = inttoptr i64 %8 to <64 x i32> addrspace(1)*
  %10 = load <64 x i32>, <64 x i32> addrspace(1)* %9, align 16
  %11 = shl nsw i64 %indvars.iv, 2
  %12 = add i64 %11, %5
  %13 = inttoptr i64 %12 to <128 x i32> addrspace(1)*
  %14 = load <128 x i32>, <128 x i32> addrspace(1)* %13, align 16
  %indvars.iv.next61 = add nuw nsw i64 %indvars.iv60, 64
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 128
  %15 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.05259, <128 x i32> %14, <64 x i32> %10, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %16 = add nuw nsw i32 %.05358, 1
  %exitcond.not = icmp eq i32 %16, 16
  br i1 %exitcond.not, label %17, label %6

17:                                               ; preds = %6
  %18 = bitcast i8 addrspace(1)* %0 to <128 x i32> addrspace(1)*
  store <128 x i32> %15, <128 x i32> addrspace(1)* %18, align 16
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
