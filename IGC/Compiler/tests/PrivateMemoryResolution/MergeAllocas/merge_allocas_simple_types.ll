;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --regkey DisableMergingOfAllocasWithDifferentType=0 --igc-ocl-merge-allocas --igc-private-mem-resolution -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that allocas with different simple types are correctly merged.

define spir_kernel void @main(float addrspace(1)* %0, i64 %1, i64 %2, i32 %3, i32 %4) {
; CHECK-LABEL: main
; CHECK-NEXT: alloca i64, align 4
; CHECK-NEXT: bitcast i64* {{.*}} to float*
; CHECK-NOT: alloca float, align 4
  %6 = alloca float, align 4
  %7 = alloca i64, align 4
  br label %8

8:
  %9 = icmp ult i32 %3, %4
  br i1 %9, label %10, label %11

10:
  store float 5.000000e-01, float* %6, align 4
  br label %12

11:
  store i64 200, i64* %7, align 4
  br label %12

12:
  ret void
}