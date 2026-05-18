;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; Check that isDpasAccumulator returns false when the phi is passed as arg 1
; (src1) of dpas2 instead of arg 0 (accumulator).
; The getArgOperand(0) == V check inside isDpasAccumulator must reject such phis,
; leaving NumDpasZeroAcc == 0 and preventing loop peeling.
; Verified by checking that the phi nodes still carry zeroinitializer (no peel).

; RUN: %opt_typed_ptrs %use_old_pass_manager% -loop-unroll -vc-peel-loops-dpas-null-acc=true -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -loop-unroll -vc-peel-loops-dpas-null-acc=true -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; Function Attrs: nofree nosync nounwind readnone
declare <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32) #0

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i8 addrspace(1)* %2) local_unnamed_addr #1 !spirv.ParameterDecorations !8 !intel_reqd_sub_group_size !11 {
  %4 = ptrtoint i8 addrspace(1)* %1 to i64
  %5 = ptrtoint i8 addrspace(1)* %2 to i64
  br label %6

6:                                                ; preds = %6, %3
  ; The phi nodes below are zero-initialized but are passed as arg 1 (src1) of
  ; dpas2, NOT as arg 0 (accumulator).  isDpasAccumulator must return false for
  ; them because CI->getArgOperand(0) != phi.  No loop peeling should occur.
  ;
  ; CHECK-DAG: [[PHI0:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],
  ; CHECK-DAG: [[PHI1:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],
  ; CHECK-DAG: [[PHI2:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],
  ; CHECK-DAG: [[PHI3:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],

  %indvars.iv = phi i64 [ 0, %3 ], [ %indvars.iv.next, %6 ]
  %.0140155 = phi i32 [ 0, %3 ], [ %19, %6 ]
  ; These zero-initialized phis are src1 (arg 1), not the accumulator (arg 0).
  %.0143152 = phi <128 x i32> [ zeroinitializer, %3 ], [ %18, %6 ]
  %.0144151 = phi <128 x i32> [ zeroinitializer, %3 ], [ %16, %6 ]
  %.0145150 = phi <128 x i32> [ zeroinitializer, %3 ], [ %14, %6 ]
  %.0146149 = phi <128 x i32> [ zeroinitializer, %3 ], [ %12, %6 ]
  %7 = shl nsw i64 %indvars.iv, 2
  %8 = add i64 %7, %4
  %9 = inttoptr i64 %8 to <64 x i32> addrspace(1)*
  %10 = load <64 x i32>, <64 x i32> addrspace(1)* %9, align 16
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 64
  ; undef is used as the accumulator (arg 0) so the peeling check focuses on
  ; the zero-initialized phis, which are passed as src1 (arg 1).
  %11 = add i64 %7, %5
  %12 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> undef, <128 x i32> %.0146149, <64 x i32> %10, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %13 = inttoptr i64 %11 to <64 x i32> addrspace(1)*
  %14 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> undef, <128 x i32> %.0145150, <64 x i32> %10, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %15 = add i64 %7, %5
  %16 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> undef, <128 x i32> %.0144151, <64 x i32> %10, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %17 = inttoptr i64 %15 to <64 x i32> addrspace(1)*
  %18 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> undef, <128 x i32> %.0143152, <64 x i32> %10, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %19 = add nuw nsw i32 %.0140155, 1
  %exitcond.not = icmp eq i32 %19, 16
  br i1 %exitcond.not, label %20, label %6

20:                                               ; preds = %6
  %.lcssa4 = phi <128 x i32> [ %12, %6 ]
  %.lcssa3 = phi <128 x i32> [ %14, %6 ]
  %.lcssa2 = phi <128 x i32> [ %16, %6 ]
  %.lcssa = phi <128 x i32> [ %18, %6 ]
  %21 = ptrtoint i8 addrspace(1)* %0 to i64
  %22 = bitcast i8 addrspace(1)* %0 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa4, <128 x i32> addrspace(1)* %22, align 16
  %23 = add i64 %21, 512
  %24 = inttoptr i64 %23 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa3, <128 x i32> addrspace(1)* %24, align 16
  %25 = add i64 %21, 1024
  %26 = inttoptr i64 %25 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa2, <128 x i32> addrspace(1)* %26, align 16
  %27 = add i64 %21, 1536
  %28 = inttoptr i64 %27 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa, <128 x i32> addrspace(1)* %28, align 16
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
