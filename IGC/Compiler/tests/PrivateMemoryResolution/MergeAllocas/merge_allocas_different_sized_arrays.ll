;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-ocl-merge-allocas --igc-private-mem-resolution -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that different sized allocas are merged and that only largest one is kept.

define spir_kernel void @main(float addrspace(1)* %0, i64 %1, i64 %2, i32 %3, i32 %4) {
; CHECK-LABEL: main
; CHECK-NEXT: alloca [256 x float], align 4
; CHECK-NEXT: bitcast [256 x float]* {{.*}} to [128 x float]*
; CHECK-NOT: alloca [128 x float], align 4
  %6 = alloca [128 x float], align 4
  %7 = alloca [256 x float], align 4
  br label %8

8:
  %9 = icmp ult i32 %3, %4
  br i1 %9, label %10, label %12

10:
  %11 = getelementptr inbounds [128 x float], [128 x float]* %6, i64 0, i64 %1
  br label %14

12:
  %13 = getelementptr inbounds [256 x float], [256 x float]* %7, i64 0, i64 %2
  br label %14

14:
  ret void
}