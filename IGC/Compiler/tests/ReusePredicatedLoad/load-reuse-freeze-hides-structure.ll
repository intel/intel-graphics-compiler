;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-22-plus
; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; LLVM 14-17 do not prove (x+y >=u x) => (x+y >=u y), which @add_structure_noundef needs.

; The implication is proved again after freezing, and that proof can fail:
; freezing a shared value replaces it in the structure the first proof matched.
; The freezes are then removed again, and the function is unchanged.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; For %sum = add %x, %y, `icmp uge %sum, %x` implies `icmp uge %sum, %y`. %m
; and %q share %sum and %x, and both are frozen. The two freezes are chosen
; independently, so %sum.fr need not equal %x.fr + %y, and the `add` no longer
; relates the two comparisons. The proof fails, and both freezes are removed.
;
; TODO: Missed optimization: freezing %x and %y instead of %sum would keep the
; `add` visible (%sum = add %x.fr, %y.fr is then well defined), and %m would
; still imply %q.
; CHECK-LABEL: define i32 @add_structure(
; CHECK-NEXT:    %sum = add i32 %x, %y
; CHECK-NEXT:    %q = icmp uge i32 %sum, %y
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
; CHECK-NEXT:    %m = icmp uge i32 %sum, %x
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
; CHECK-NEXT:    %s = add i32 %a, %b
; CHECK-NEXT:    ret i32 %s

define i32 @add_structure(ptr addrspace(1) %p, i32 %x, i32 %y) {
  %sum = add i32 %x, %y
  %q = icmp uge i32 %sum, %y
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %m = icmp uge i32 %sum, %x
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

; With noundef %x and %y nothing is frozen, and %b is reused.
; CHECK-LABEL: define i32 @add_structure_noundef(
; CHECK-NEXT:    %sum = add i32 %x, %y
; CHECK-NEXT:    %q = icmp uge i32 %sum, %y
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
; CHECK-NEXT:    %m = icmp uge i32 %sum, %x
; CHECK-NEXT:    %[[SEL:[0-9]+]] = select i1 %m, i32 %a, i32 13
; CHECK-NEXT:    %s = add i32 %a, %[[SEL]]
; CHECK-NEXT:    ret i32 %s

define i32 @add_structure_noundef(ptr addrspace(1) %p, i32 noundef %x, i32 noundef %y) {
  %sum = add i32 %x, %y
  %q = icmp uge i32 %sum, %y
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %m = icmp uge i32 %sum, %x
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
