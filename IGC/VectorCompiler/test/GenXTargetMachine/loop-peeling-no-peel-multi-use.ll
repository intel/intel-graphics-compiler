;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; Check that isDpasAccumulator returns false when the phi node has more than one
; use.  Even though each phi is passed as arg 0 (accumulator) of dpas2 calls,
; each phi feeds two dpas2 calls so hasOneUse() returns false.
; This leaves NumDpasZeroAcc == 0 and prevents loop peeling.
; Verified by checking that the phi nodes still carry zeroinitializer (no peel).

; RUN: %opt_typed_ptrs %use_old_pass_manager% -loop-unroll -vc-peel-loops-dpas-null-acc=true -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -loop-unroll -vc-peel-loops-dpas-null-acc=true -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; Function Attrs: nofree nosync nounwind readnone
declare <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nofree nosync nounwind readnone
declare <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32) #0

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i8 addrspace(1)* %2) local_unnamed_addr #1 !spirv.ParameterDecorations !8 !intel_reqd_sub_group_size !11 {
  %4 = ptrtoint i8 addrspace(1)* %1 to i64
  %5 = ptrtoint i8 addrspace(1)* %2 to i64
  br label %6

6:                                                ; preds = %6, %3
  ; Each zero-initialized phi is used as acc (arg 0) in TWO dpas2 calls, so
  ; hasOneUse() == false.  isDpasAccumulator must return false for all of them,
  ; leaving NumDpasZeroAcc == 0 and preventing loop peeling.
  ;
  ; CHECK-DAG: [[PHI0:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],
  ; CHECK-DAG: [[PHI1:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],
  ; CHECK-DAG: [[PHI2:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],
  ; CHECK-DAG: [[PHI3:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ],

  %indvars.iv159 = phi i64 [ 0, %3 ], [ %indvars.iv.next160, %6 ]
  %indvars.iv = phi i64 [ 0, %3 ], [ %indvars.iv.next, %6 ]
  %.0140155 = phi i32 [ 0, %3 ], [ %33, %6 ]
  ; Each of these phi nodes has zeroinitializer on entry.
  %.0143152 = phi <128 x i32> [ zeroinitializer, %3 ], [ %30, %6 ]
  %.0144151 = phi <128 x i32> [ zeroinitializer, %3 ], [ %26, %6 ]
  %.0145150 = phi <128 x i32> [ zeroinitializer, %3 ], [ %22, %6 ]
  %.0146149 = phi <128 x i32> [ zeroinitializer, %3 ], [ %18, %6 ]
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

  ; phi %.0146149 used TWICE as acc (arg 0) -- hasOneUse() is false.
  %15 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 0, i32 8)
  %16 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0146149, <128 x i32> %14, <64 x i32> %15, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %17 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 128, i32 8)
  %18 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0146149, <128 x i32> %14, <64 x i32> %17, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)

  ; phi %.0145150 used TWICE as acc (arg 0) -- hasOneUse() is false.
  %19 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 256, i32 8)
  %20 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0145150, <128 x i32> %14, <64 x i32> %19, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %21 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 384, i32 8)
  %22 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0145150, <128 x i32> %14, <64 x i32> %21, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)

  ; phi %.0144151 used TWICE as acc (arg 0) -- hasOneUse() is false.
  %23 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 512, i32 8)
  %24 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0144151, <128 x i32> %14, <64 x i32> %23, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %25 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 640, i32 8)
  %26 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0144151, <128 x i32> %14, <64 x i32> %25, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)

  ; phi %.0143152 used TWICE as acc (arg 0) -- hasOneUse() is false.
  %27 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 768, i32 8)
  %28 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0143152, <128 x i32> %14, <64 x i32> %27, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)
  %29 = call <64 x i32> @llvm.genx.rdregioni.v64i32.v256i32.i16(<256 x i32> %10, i32 8, i32 8, i32 1, i16 896, i32 8)
  %30 = call <128 x i32> @llvm.genx.dpas2.v128i32.v128i32.v128i32.v64i32(<128 x i32> %.0143152, <128 x i32> %14, <64 x i32> %29, i32 8, i32 8, i32 8, i32 8, i32 1, i32 1)

  %31 = add <128 x i32> %16, %22
  %32 = add <128 x i32> %24, %28
  %33 = add nuw nsw i32 %.0140155, 1
  %exitcond.not = icmp eq i32 %33, 16
  br i1 %exitcond.not, label %34, label %6

34:                                               ; preds = %6
  %.lcssa4 = phi <128 x i32> [ %18, %6 ]
  %.lcssa3 = phi <128 x i32> [ %26, %6 ]
  %.lcssa2 = phi <128 x i32> [ %30, %6 ]
  %.lcssa1 = phi <128 x i32> [ %31, %6 ]
  %.lcssa = phi <128 x i32> [ %32, %6 ]
  %35 = ptrtoint i8 addrspace(1)* %0 to i64
  %36 = bitcast i8 addrspace(1)* %0 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa4, <128 x i32> addrspace(1)* %36, align 16
  %37 = add i64 %35, 512
  %38 = inttoptr i64 %37 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa3, <128 x i32> addrspace(1)* %38, align 16
  %39 = add i64 %35, 1024
  %40 = inttoptr i64 %39 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa2, <128 x i32> addrspace(1)* %40, align 16
  %41 = add i64 %35, 1536
  %42 = inttoptr i64 %41 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa1, <128 x i32> addrspace(1)* %42, align 16
  %43 = add i64 %35, 2048
  %44 = inttoptr i64 %43 to <128 x i32> addrspace(1)*
  store <128 x i32> %.lcssa, <128 x i32> addrspace(1)* %44, align 16
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
