;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @testkernel1(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp eq i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  %selres = select i1 %cmp, i8 0, i8 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel1
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp ne i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: %selres = select i1 [[CONDNEW]], i8 1, i8 0

define spir_kernel void @testkernel2(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp ugt i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  %selres = select i1 %cmp, i8 0, i8 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel2
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp ule i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: %selres = select i1 [[CONDNEW]], i8 1, i8 0

define spir_kernel void @testkernel3(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp uge i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  %selres = select i1 %cmp, i8 0, i8 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel3
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp ult i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: %selres = select i1 [[CONDNEW]], i8 1, i8 0

define spir_kernel void @testkernel4(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp ult i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  %selres = select i1 %cmp, i8 0, i8 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel4
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp uge i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: %selres = select i1 [[CONDNEW]], i8 1, i8 0

define spir_kernel void @testkernel5(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp ule i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  br i1 %cmp, label %bb1, label %bb2
bb1:
  br label %bb3
bb2:
  br label %bb3
bb3:
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel5
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp ugt i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: br i1 [[CONDNEW]], label %bb2, label %bb1

define spir_kernel void @testkernel6(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp sgt i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  br i1 %cmp, label %bb1, label %bb2
bb1:
  br label %bb3
bb2:
  br label %bb3
bb3:
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel6
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp sle i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: br i1 [[CONDNEW]], label %bb2, label %bb1

define spir_kernel void @testkernel7(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp sge i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  br i1 %cmp, label %bb1, label %bb2
bb1:
  br label %bb3
bb2:
  br label %bb3
bb3:
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel7
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp slt i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: br i1 [[CONDNEW]], label %bb2, label %bb1

define spir_kernel void @testkernel8(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp slt i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  br i1 %cmp, label %bb1, label %bb2
bb1:
  br label %bb3
bb2:
  br label %bb3
bb3:
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel8
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp sge i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %and.cond = and i1 %andop1, [[CONDNEW]]
; CHECK: br i1 [[CONDNEW]], label %bb2, label %bb1

define spir_kernel void @testkernel9(i32 %cmpop1, i32 %cmpop2) #0 {
0:
  %cmp = icmp sle i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel9
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp sgt i32 %cmpop1, %cmpop2
; CHECK-NOT: xor

define spir_kernel void @testkernel10(i32 %cmpop1, i32 %cmpop2) #0 {
0:
  %cmp = icmp eq i32 %cmpop1, %cmpop2
  %cmp.not1 = xor i1 %cmp, 3
  %cmp.not2 = xor i1 %cmp, 3
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel10
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp eq i32 %cmpop1, %cmpop2
; CHECK: %cmp.not1 = xor i1 [[CONDNEW]], true
; CHECK: %cmp.not2 = xor i1 [[CONDNEW]], true

define spir_kernel void @testkernel11(i32 %cmpop1, i32 %cmpop2) #0 {
0:
  %cmp = icmp uge i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 true, %cmp
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel11
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp ult i32 %cmpop1, %cmpop2
; CHECK-NOT: xor

define spir_kernel void @testkernel12(i32 %cmpop1, i32 %cmpop2, i1 %andop1) #0 {
0:
  %cmp = icmp ult i32 %cmpop1, %cmpop2
  %cmp.not = xor i1 %cmp, true
  %and.cond = and i1 %andop1, %cmp.not
  %selres1 = select i1 %cmp, i8 0, i8 1
  %selres2 = select i1 %cmp, i8 0, i8 1
  br i1 %cmp, label %bb1, label %bb2
bb1:
  br i1 %cmp, label %bb2, label %bb3
bb2:
  br i1 %cmp, label %bb1, label %bb3
bb3:
  ret void
}

; CHECK-LABEL: define spir_kernel void @testkernel12
; CHECK: [[CONDNEW:%[a-zA-Z0-9]+]] = icmp uge i32 %cmpop1, %cmpop2
; CHECK-NOT: xor
; CHECK: %selres1 = select i1 [[CONDNEW]], i8 1, i8 0
; CHECK: %selres2 = select i1 [[CONDNEW]], i8 1, i8 0
; CHECK: br i1 [[CONDNEW]], label %bb2, label %bb1
; CHECK: br i1 [[CONDNEW]], label %bb3, label %bb2
; CHECK: br i1 [[CONDNEW]], label %bb3, label %bb1
