;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; Visiting %u promotes the PHI node %p, which promotes its back-edge value %v further down the
; block. The replacement of %v must not be promoted again when the visitor reaches it, or
; cleanUp() erases it and leaves its user %w with an undefined operand.

; CHECK-LABEL: define void @loop_carried_i1(
; CHECK:       loop:
; CHECK:         [[P:%.*]] = phi i8 [ 1, %entry ], [ [[V8:%.*]], %loop ]
; CHECK:         [[U:%.*]] = icmp ne i8 [[P]], 1
; CHECK:         [[U8:%.*]] = zext i1 [[U]] to i8
; CHECK:         [[V:%.*]] = icmp ne i8 %x, 0
; CHECK:         [[V8]] = zext i1 [[V]] to i8
; CHECK:         [[W:%.*]] = icmp ne i8 [[V8]], [[U8]]
; CHECK-NOT:     undef
; CHECK-NOT:     poison
; CHECK:         ret void

define void @loop_carried_i1(ptr %out, ptr %in, i64 %n) {
entry:
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %i.next, %loop ]
  %p = phi i1 [ true, %entry ], [ %v, %loop ]
  %u = icmp ne i1 %p, true
  %ptr = getelementptr i8, ptr %in, i64 %i
  %x = load i8, ptr %ptr
  %v = icmp ne i8 %x, 0
  %w = icmp ne i1 %v, %u
  %i.next = add i64 %i, 1
  %done = icmp eq i64 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  %r = zext i1 %w to i8
  store i8 %r, ptr %out
  ret void
}
