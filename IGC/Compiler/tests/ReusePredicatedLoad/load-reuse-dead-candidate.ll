;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; A candidate no user reads is not optimized and left for the dead-instruction cleanup.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @dead_candidate(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
; CHECK-NEXT:    ret i32 %b

define i32 @dead_candidate(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  ret i32 %b
}

; A live candidate is reused.

; CHECK-LABEL: define i32 @live_candidate(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %q, i32 %a, i32 13
; CHECK-NEXT:    %s = add i32 %a, %[[V1]]
; CHECK-NEXT:    ret i32 %s

define i32 @live_candidate(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
