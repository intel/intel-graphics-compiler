;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-merge-allocas --igc-private-mem-resolution -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that array allocas are not merged with regular allocas.

define spir_kernel void @main(float addrspace(1)* %0, i64 %1, i64 %2, i32 %3, i32 %4) {
; CHECK-LABEL: main
; CHECK-NEXT: alloca [128 x float], align 4
; CHECK-NEXT: alloca float, align 4
  %6 = alloca [128 x float], align 4
  %7 = alloca float, align 4
  br label %8

8:
  %9 = icmp ult i32 %3, %4
  br i1 %9, label %10, label %12

10:
  %11 = getelementptr inbounds [128 x float], [128 x float]* %6, i64 0, i64 %1
  br label %14

12:
  %13 = load float, float* %7, align 4
  br label %14

14:
  ret void
}