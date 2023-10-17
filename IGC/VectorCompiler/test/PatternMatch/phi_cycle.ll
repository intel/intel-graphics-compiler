;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; CHECK-NOT: %bad1 = phi i32 [ undef, %entry ], [ %bad2, %loop.end ]
; CHECK-NOT: %bad2 = phi i32 [ %bad1, %true ], [ %mul, %false ]

define i32 @test(i32 %a, i32 %n) {
entry:
  br label %loop

loop:
  %bad1 = phi i32 [ undef, %entry ], [ %bad2, %loop.end ]
  %res = phi i32 [ 0, %entry ], [ %res.next, %loop.end ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop.end ]
  %if.cond = icmp slt i32 %i, 10
  br i1 %if.cond, label %true, label %false

true:
  %add = add i32 %res, %a
  br label %loop.end

false:
  %mul = mul i32 %res, %a
  br label %loop.end

loop.end:
  %bad2 = phi i32 [ %bad1, %true ], [ %mul, %false ]
  %res.next = phi i32 [ %add, %true ], [ %mul, %false ]
  %i.next = add i32 %i, 1
  %loop.cond = icmp slt i32 %i.next, %n
  br i1 %loop.cond, label %loop, label %exit

exit:
  ret i32 %res.next
}
