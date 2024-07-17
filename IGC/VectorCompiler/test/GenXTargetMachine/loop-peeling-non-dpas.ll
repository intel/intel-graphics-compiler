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

define void @kernel(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i8 addrspace(1)* %2) {
  %4 = ptrtoint i8 addrspace(1)* %1 to i64
  %5 = ptrtoint i8 addrspace(1)* %2 to i64
  br label %6

6:                                                ; preds = %6, %3
  ; CHECK-DAG: [[PHI0:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ], [ [[ACC0:[^ ]+]], %{{[^ ]+}} ]
  ; CHECK-DAG: [[PHI1:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ], [ [[ACC1:[^ ]+]], %{{[^ ]+}} ]
  ; CHECK-DAG: [[PHI2:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ], [ [[ACC2:[^ ]+]], %{{[^ ]+}} ]
  ; CHECK-DAG: [[PHI3:%[^ ]+]] = phi <128 x i32> [ zeroinitializer, %{{[^ ]+}} ], [ [[ACC3:[^ ]+]], %{{[^ ]+}} ]

  ; CHECK-DAG: [[ACC0]] = add <128 x i32> [[PHI0]],
  ; CHECK-DAG: [[ACC1]] = add <128 x i32> [[PHI1]],
  ; CHECK-DAG: [[ACC2]] = add <128 x i32> [[PHI2]],
  ; CHECK-DAG: [[ACC3]] = add <128 x i32> [[PHI3]],

  %indvars.iv159 = phi i64 [ 0, %3 ], [ %indvars.iv.next160, %6 ]
  %indvars.iv = phi i64 [ 0, %3 ], [ %indvars.iv.next, %6 ]
  %.0140155 = phi i32 [ 0, %3 ], [ %19, %6 ]
  %.0143152 = phi <128 x i32> [ zeroinitializer, %3 ], [ %18, %6 ]
  %.0144151 = phi <128 x i32> [ zeroinitializer, %3 ], [ %17, %6 ]
  %.0145150 = phi <128 x i32> [ zeroinitializer, %3 ], [ %16, %6 ]
  %.0146149 = phi <128 x i32> [ zeroinitializer, %3 ], [ %15, %6 ]
  %7 = shl nsw i64 %indvars.iv159, 2
  %8 = add i64 %7, %4
  %9 = inttoptr i64 %8 to <128 x i32> addrspace(1)*
  %10 = load <128 x i32>, <128 x i32> addrspace(1)* %9, align 16
  %11 = shl nsw i64 %indvars.iv, 2
  %12 = add i64 %11, %5
  %13 = inttoptr i64 %12 to <128 x i32> addrspace(1)*
  %14 = load <128 x i32>, <128 x i32> addrspace(1)* %13, align 16
  %indvars.iv.next160 = add nuw nsw i64 %indvars.iv159, 256
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 128
  %15 = add <128 x i32> %.0146149, %10
  %16 = add <128 x i32> %.0145150, %14
  %17 = add <128 x i32> %.0144151, %10
  %18 = add <128 x i32> %.0143152, %14
  %19 = add nuw nsw i32 %.0140155, 1
  %exitcond.not = icmp eq i32 %19, 16
  br i1 %exitcond.not, label %20, label %6

20:                                               ; preds = %6
  %.lcssa4 = phi <128 x i32> [ %15, %6 ]
  %.lcssa3 = phi <128 x i32> [ %16, %6 ]
  %.lcssa2 = phi <128 x i32> [ %17, %6 ]
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
