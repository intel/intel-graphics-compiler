;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

; Condition depends on load; load can't be hoisted before store.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @hoist_load_across_store(
; CHECK-NEXT:    %slot = alloca i32, align 4
; CHECK-NEXT:    store i32 0, ptr %slot, align 4
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    store i32 1, ptr %slot, align 4
; CHECK-NEXT:    %cv = load i32, ptr %slot, align 4
; CHECK-NEXT:    %A = icmp eq i32 %cv, 1
; CHECK-NEXT:    %r = select i1 %A, i32 %a, i32 %other
; CHECK-NEXT:    ret i32 %r

define i32 @hoist_load_across_store(ptr addrspace(1) %p, i1 %S, i32 %other) {
  %slot = alloca i32, align 4
  store i32 0, ptr %slot, align 4
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  store i32 1, ptr %slot, align 4
  %cv = load i32, ptr %slot, align 4
  %A = icmp eq i32 %cv, 1
  %r = select i1 %A, i32 %a, i32 %other
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
