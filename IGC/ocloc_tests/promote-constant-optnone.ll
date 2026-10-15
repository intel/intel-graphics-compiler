;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; PromoteConstant must leave optnone functions alone, like the legacy pass
; manager's skipFunction does.

; REQUIRES: regkeys, bmg-supported, llvm-16-plus
; RUN: llvm-as %OPAQUE_PTR_FLAG% < %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1, PrintToConsole=1, PrintAfter=PromoteConstant'" 2>&1 | FileCheck %s

; CHECK-LABEL: define {{.*}} @lookup(
; CHECK:       getelementptr inbounds i32, ptr addrspace(2) @table
; CHECK-NEXT:  load i32

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

@table = internal addrspace(2) constant [2 x i32] [i32 3, i32 5], align 4

define spir_func i32 @lookup(i64 %i, i32 %n) #0 {
entry:
  br label %loop
loop:
  %k = phi i32 [ 0, %entry ], [ %k.next, %loop ]
  %acc = phi i32 [ 0, %entry ], [ %sum, %loop ]
  %p = getelementptr inbounds i32, ptr addrspace(2) @table, i64 %i
  %v = load i32, ptr addrspace(2) %p, align 4
  %sum = add i32 %acc, %v
  %k.next = add i32 %k, 1
  %c = icmp slt i32 %k.next, %n
  br i1 %c, label %loop, label %exit
exit:
  ret i32 %sum
}

define spir_kernel void @test(ptr addrspace(1) %out, i64 %i, i32 %n) {
  %r = call spir_func i32 @lookup(i64 %i, i32 %n)
  store i32 %r, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { noinline optnone }
