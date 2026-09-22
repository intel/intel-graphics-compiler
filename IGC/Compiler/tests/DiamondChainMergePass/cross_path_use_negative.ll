;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s
;
; Negative regression for path-sensitive check4:
; value defined in t0 (%t0v) is used in f1 (%cross). This cross-path use must
; remain compilable without introducing malformed IR.

define spir_kernel void @cross_path_use_negative(i32 %x, ptr addrspace(1) %out) #0 {
; CHECK-LABEL: define spir_kernel void @cross_path_use_negative(
; CHECK: ret void
;
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %t0, label %f0

t0:                                                ; preds = %entry
  %t0v = add i32 %x, 11
  br label %m0

f0:                                                ; preds = %entry
  %f0v = sub i32 %x, 7
  br label %m0

m0:                                                ; preds = %f0, %t0
  %m0v = phi i32 [ %t0v, %t0 ], [ %f0v, %f0 ]
  %t0x = phi i32 [ %t0v, %t0 ], [ 0, %f0 ]
  br i1 %cmp, label %t1, label %f1

t1:                                                ; preds = %m0
  %t1v = add i32 %m0v, 1
  br label %m1

f1:                                                ; preds = %m0
  %cross = add i32 %t0x, %m0v
  br label %m1

m1:                                                ; preds = %f1, %t1
  %r = phi i32 [ %t1v, %t1 ], [ %cross, %f1 ]
  store i32 %r, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
