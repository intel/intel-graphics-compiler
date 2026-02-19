;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey DisableMergingOfMultipleAllocasWithOffset=0 --regkey DisableMergingOfAllocasWithDifferentType=0 --igc-ocl-merge-allocas --igc-private-mem-resolution -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that multiple smaller allocas can be merged as part of larger alloca,
; and that offsets are added correctly.

define spir_kernel void @main(float addrspace(1)* %0, i64 %1, i64 %2, i32 %3, i32 %4) {
; CHECK-LABEL: main
; CHECK-NEXT: alloca [512 x float], align 4
; CHECK-NEXT: bitcast [512 x float]* {{.*}} to i8*
; CHECK-NEXT: bitcast [512 x float]* {{.*}} to [256 x i32]*
; CHECK-NEXT: getelementptr i8, i8* {{.*}}, i32 1024
; CHECK-NEXT: bitcast i8* {{.*}} to [128 x i32]*
; CHECK-NOT: alloca [128 x i32], align 4
; CHECK-NOT: alloca [256 x i32], align 4
  %6 = alloca [128 x i32], align 4
  %7 = alloca [512 x float], align 4
  %8 = alloca [256 x i32], align 4
  br label %9

9:
  %10 = icmp ult i32 %3, %4
  br i1 %10, label %11, label %14

11:
  %12 = getelementptr inbounds [128 x i32], [128 x i32]* %6, i64 0, i64 %1
  %13 = getelementptr inbounds [256 x i32], [256 x i32]* %8, i64 0, i64 %1
  store i32 1, i32* %12
  store i32 2, i32* %13
  br label %16

14:
  %15 = getelementptr inbounds [512 x float], [512 x float]* %7, i64 0, i64 %2
  br label %16

16:
  ret void
}