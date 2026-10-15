;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-type-legalizer -verify -S < %s | FileCheck %s --check-prefixes=CHECK,%if llvm-18-plus %{CHECK-LLVM18%} %else %{CHECK-PRE18%}
; ------------------------------------------------
; Legalization: ptrtoint to an illegal integer width
; ------------------------------------------------
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

@slm = external addrspace(3) global [0 x i8]

define i1 @test_global_i3() {
; Before LLVM 18, the global-pointer operations fold into constant expressions.
; CHECK-LABEL: define i1 @test_global_i3(
; CHECK-PRE18:   [[R:%.*]] = and i1 icmp slt (i8 ashr (i8 shl (i8 and (i8 ptrtoint (ptr addrspace(3) @slm to i8), i8 7), i8 5), i8 5), i8 2),
; CHECK-PRE18-SAME: icmp eq (i8 and (i8 ptrtoint (ptr addrspace(3) @slm to i8), i8 7), i8 1)
; CHECK-LLVM18:  [[AND:%.*]] = and i8 ptrtoint (ptr addrspace(3) @slm to i8), 7
; CHECK-LLVM18:  [[SHL:%.*]] = shl i8 [[AND]], 5
; CHECK-LLVM18:  [[ASHR:%.*]] = ashr i8 [[SHL]], 5
; CHECK-LLVM18:  [[SLT:%.*]] = icmp slt i8 [[ASHR]], 2
; CHECK-LLVM18:  [[MASK:%.*]] = and i8 [[AND]], 7
; CHECK-LLVM18:  [[EQ:%.*]] = icmp eq i8 [[MASK]], 1
; CHECK-LLVM18:  [[R:%.*]] = and i1 [[SLT]], [[EQ]]
; CHECK:         ret i1 [[R]]
;
  %v = ptrtoint ptr addrspace(3) @slm to i3
  %slt = icmp slt i3 %v, 2
  %eq = icmp eq i3 %v, 1
  %r = and i1 %slt, %eq
  ret i1 %r
}

define i32 @test_arg_i3(ptr addrspace(1) %p) {
; CHECK-LABEL: define i32 @test_arg_i3(
; CHECK:         [[P2I:%.*]] = ptrtoint ptr addrspace(1) %p to i8
; CHECK:         [[AND:%.*]] = and i8 [[P2I]], 7
; CHECK:         [[MASK:%.*]] = and i8 [[AND]], 7
; CHECK:         [[Z:%.*]] = zext i8 [[MASK]] to i32
; CHECK:         ret i32 [[Z]]
;
  %v = ptrtoint ptr addrspace(1) %p to i3
  %z = zext i3 %v to i32
  ret i32 %z
}

define i64 @test_arg_i40(ptr addrspace(1) %p) {
; CHECK-LABEL: define i64 @test_arg_i40(
; CHECK:         [[P2I:%.*]] = ptrtoint ptr addrspace(1) %p to i64
; CHECK:         [[AND:%.*]] = and i64 [[P2I]], 1099511627775
; CHECK:         [[Z:%.*]] = and i64 [[AND]], 1099511627775
; CHECK:         ret i64 [[Z]]
;
  %v = ptrtoint ptr addrspace(1) %p to i40
  %z = zext i40 %v to i64
  ret i64 %z
}
