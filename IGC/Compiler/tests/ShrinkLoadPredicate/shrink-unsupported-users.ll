;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; A PHI is not walked through, so the load keeps its predicate.
; CHECK-LABEL: define i32 @phi_use(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    br i1 %c, label %then, label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  then:
; CHECK-NEXT:    br label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  exit:
; CHECK-NEXT:    %phi = phi i32 [ %other, %entry ], [ %a, %then ]
; CHECK-NEXT:    ret i32 %phi

define i32 @phi_use(ptr addrspace(1) %p, i1 %q, i1 %c, i32 %other) {
entry:
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  br i1 %c, label %then, label %exit

then:
  br label %exit

exit:
  %phi = phi i32 [ %other, %entry ], [ %a, %then ]
  ret i32 %phi
}

; Load uses as select's condition - use on every lane.
; CHECK-LABEL: define i32 @select_condition_use(
; CHECK-NEXT:    %a = call i1 @llvm.genx.GenISA.PredicatedLoad.i1.p1.i1(ptr addrspace(1) %p, i64 1, i1 %q, i1 false)
; CHECK-NEXT:    %r = select i1 %a, i32 %x, i32 %y
; CHECK-NEXT:    ret i32 %r

define i32 @select_condition_use(ptr addrspace(1) %p, i1 %q, i32 %x, i32 %y) {
  %a = call i1 @llvm.genx.GenISA.PredicatedLoad.i1.p1.i1(ptr addrspace(1) %p, i64 1, i1 %q, i1 false)
  %r = select i1 %a, i32 %x, i32 %y
  ret i32 %r
}

; One conditional use does not help while a store also observes the result
; unconditionally.
; CHECK-LABEL: define void @store_use(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %selected = select i1 %c, i32 %a, i32 %other
; CHECK-NEXT:    store i32 %a, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    store i32 %selected, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void

define void @store_use(ptr addrspace(1) %p, ptr addrspace(1) %out, i1 %q, i1 %c, i32 %other) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %selected = select i1 %c, i32 %a, i32 %other
  store i32 %a, ptr addrspace(1) %out, align 4
  store i32 %selected, ptr addrspace(1) %out, align 4
  ret void
}

; Element precision is dropped: %hi is used on every lane, so the whole vector
; is, even though %lo is only used where %c is false.
; CHECK-LABEL: define <2 x i32> @partial_vector_use(
; CHECK-NEXT:    %a = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1) %p, i64 8, i1 %q, <2 x i32> zeroinitializer)
; CHECK-NEXT:    %lo = extractelement <2 x i32> %a, i32 0
; CHECK-NEXT:    %hi = extractelement <2 x i32> %a, i32 1
; CHECK-NEXT:    %glo = select i1 %c, i32 %lo, i32 %other
; CHECK-NEXT:    %v0 = insertelement <2 x i32> poison, i32 %glo, i32 0
; CHECK-NEXT:    %r = insertelement <2 x i32> %v0, i32 %hi, i32 1
; CHECK-NEXT:    ret <2 x i32> %r

define <2 x i32> @partial_vector_use(ptr addrspace(1) %p, i1 %q, i1 %c, i32 %other) {
  %a = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1) %p, i64 8, i1 %q, <2 x i32> zeroinitializer)
  %lo = extractelement <2 x i32> %a, i32 0
  %hi = extractelement <2 x i32> %a, i32 1
  %glo = select i1 %c, i32 %lo, i32 %other
  %v0 = insertelement <2 x i32> poison, i32 %glo, i32 0
  %r = insertelement <2 x i32> %v0, i32 %hi, i32 1
  ret <2 x i32> %r
}

; Only same-block instructions are hoisted, so a condition defined in a
; successor block cannot be made available at the load.
; CHECK-LABEL: define i32 @other_block_condition(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    br i1 %c, label %use, label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  use:
; CHECK-NEXT:    %cond = icmp ult i32 %n, 8
; CHECK-NEXT:    %r = select i1 %cond, i32 %a, i32 %other
; CHECK-NEXT:    br label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  exit:
; CHECK-NEXT:    %phi = phi i32 [ 0, %entry ], [ %r, %use ]
; CHECK-NEXT:    ret i32 %phi

define i32 @other_block_condition(ptr addrspace(1) %p, i1 %q, i1 %c, i32 %n, i32 %other) {
entry:
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  br i1 %c, label %use, label %exit

use:
  %cond = icmp ult i32 %n, 8
  %r = select i1 %cond, i32 %a, i32 %other
  br label %exit

exit:
  %phi = phi i32 [ 0, %entry ], [ %r, %use ]
  ret i32 %phi
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
declare i1 @llvm.genx.GenISA.PredicatedLoad.i1.p1.i1(ptr addrspace(1), i64, i1, i1) #0
declare <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1), i64, i1, <2 x i32>) #0
attributes #0 = { nounwind willreturn readonly }
