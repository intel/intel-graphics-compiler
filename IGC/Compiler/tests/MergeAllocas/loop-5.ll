;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -S --platformbmg --igc-ocl-merge-allocas %s | FileCheck %s

; %a is written in the inner loop body and read after the inner loop, so it is live across the inner loop header,
; where %b is used. The allocas must not be merged, even though the loops are nested.

define void @test(ptr %out) {
; CHECK-LABEL: @test(
; CHECK: %a = alloca i32
; CHECK: %b = alloca i32
; CHECK-NOT: MergedAlloca
entry:
  %a = alloca i32
  %b = alloca i32
  br label %outer.header

outer.header:
  %i = phi i32 [ 0, %entry ], [ %inext, %inner.exit ]
  br label %inner.header

inner.header:
  %j = phi i32 [ 0, %outer.header ], [ %jnext, %inner.body ]
  store i32 20, ptr %b
  %bv = load i32, ptr %b
  %out1 = getelementptr inbounds i32, ptr %out, i64 1
  store i32 %bv, ptr %out1
  %jc = icmp slt i32 %j, 2
  br i1 %jc, label %inner.body, label %inner.exit

inner.body:
  store i32 9, ptr %a
  %jnext = add i32 %j, 1
  br label %inner.header

inner.exit:
  %av = load i32, ptr %a
  store i32 %av, ptr %out
  %inext = add i32 %i, 1
  %ic = icmp slt i32 %inext, 2
  br i1 %ic, label %outer.header, label %outer.exit

outer.exit:
  ret void
}
