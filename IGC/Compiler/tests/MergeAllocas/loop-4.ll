;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -S --platformbmg --igc-ocl-merge-allocas %s | FileCheck %s

; Alloca whose pointer is carried to the next iteration must stay live over the whole loop.

; Carried via header PHI: %a and %b must not merge.
; CHECK-LABEL: @phi_carried(
; CHECK: %a = alloca [16 x i32]
; CHECK: %b = alloca [16 x i32]
define void @phi_carried() {
entry:
  %a = alloca [16 x i32]
  br label %loop

loop:
  %p = phi ptr [ %a, %entry ], [ %b, %loop ]
  store i32 0, ptr %p
  %b = alloca [16 x i32]
  br label %loop
}

; Carried via memory from inner loop: live over outer loop too, must not merge with %x.
; CHECK-LABEL: @memory_carried(
; CHECK: %a = alloca [16 x i32]
; CHECK: %x = alloca [16 x i32]
define void @memory_carried(ptr %q, i1 %c, i1 %d) {
entry:
  %slot = alloca ptr
  store ptr %q, ptr %slot
  br label %outer

outer:
  br label %inner

inner:
  %p = load ptr, ptr %slot
  %v = load i32, ptr %p
  %a = alloca [16 x i32]
  %gep = getelementptr [16 x i32], ptr %a, i32 0, i32 1
  store i32 %v, ptr %gep
  store ptr %gep, ptr %slot
  br i1 %c, label %outer.latch, label %inner

outer.latch:
  %x = alloca [16 x i32]
  store i32 1, ptr %x
  br i1 %d, label %exit, label %outer

exit:
  ret void
}

; Carried over inner loop only: %a merges with %x in outer loop.
; CHECK-LABEL: @inner_carried(
; CHECK: = alloca [16 x i32]
; CHECK-NOT: alloca
; CHECK: ret void
define void @inner_carried(ptr %q, i1 %c, i1 %d) {
entry:
  br label %outer

outer:
  br label %inner

inner:
  %p = phi ptr [ %q, %outer ], [ %a, %inner ]
  %v = load i32, ptr %p
  %a = alloca [16 x i32]
  store i32 %v, ptr %a
  br i1 %c, label %outer.latch, label %inner

outer.latch:
  %x = alloca [16 x i32]
  store i32 1, ptr %x
  br i1 %d, label %exit, label %outer

exit:
  ret void
}
