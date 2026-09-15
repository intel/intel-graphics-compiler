;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; After shrinking, the load returns its fallback value on every lane where the
; use condition rejects it. A select on that condition whose other incoming value
; is the same fallback value then equals the load, so it is replaced by the load.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; IfTrue: the predicate shrinks to `%S & %A`.
; CHECK-LABEL: define i32 @fallback_equals_false_value(
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %A
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    ret i32 %a

define i32 @fallback_equals_false_value(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %r = select i1 %A, i32 %a, i32 19
  ret i32 %r
}

; IfFalse: the predicate shrinks to `%S & !%A`.
; CHECK-LABEL: define i32 @fallback_equals_true_value(
; CHECK-NEXT:    %predload.usepred.not = xor i1 %A, true
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %predload.usepred.not
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    ret i32 %a

define i32 @fallback_equals_true_value(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %r = select i1 %A, i32 19, i32 %a
  ret i32 %r
}

; A condition that may be undef is frozen first. The select and the mask read the
; same %A.fr, so the select still equals the load.
; CHECK-LABEL: define i32 @undef_condition(
; CHECK:         %A.fr = freeze i1 %A
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %A.fr
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    ret i32 %a

define i32 @undef_condition(ptr addrspace(1) %p, i1 noundef %S, i32 %u) {
  %A = icmp ult i32 %u, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %r = select i1 %A, i32 %a, i32 19
  ret i32 %r
}

; The other incoming value is not the fallback value: the select stays.
; CHECK-LABEL: define i32 @other_value_differs(
; CHECK:         %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    %r = select i1 %A, i32 %a, i32 20
; CHECK-NEXT:    ret i32 %r

define i32 @other_value_differs(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %r = select i1 %A, i32 %a, i32 20
  ret i32 %r
}

; The select reads the load through a bitcast. Only selects that read the load
; directly are handled, so the select stays.
; CHECK-LABEL: define float @through_bitcast(
; CHECK:         %f = bitcast i32 %a to float
; CHECK-NEXT:    %r = select i1 %A, float %f, float 0.000000e+00
; CHECK-NEXT:    ret float %r

define float @through_bitcast(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %f = bitcast i32 %a to float
  %r = select i1 %A, float %f, float 0.0
  ret float %r
}

; Nested select chain. The predicate is shrunk by the condition of %inner, whose
; other incoming value is not the fallback value. %outer has the fallback value
; as its other incoming value, but another condition: both selects stay.
; CHECK-LABEL: define i32 @nested_select_chain(
; CHECK:         %predload.shrunk = and i1 %S, %A
; CHECK:         %inner = select i1 %A, i32 %a, i32 %x
; CHECK-NEXT:    %outer = select i1 %B, i32 %inner, i32 19
; CHECK-NEXT:    ret i32 %outer

define i32 @nested_select_chain(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A, i1 noundef %B, i32 %x) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %inner = select i1 %A, i32 %a, i32 %x
  %outer = select i1 %B, i32 %inner, i32 19
  ret i32 %outer
}

; Two loads share one mask: the second load reuses the mask built for the first.
; The other incoming value of each select is the fallback value of its load, so
; both selects are replaced by their loads, whichever load built the mask.
; CHECK-LABEL: define i32 @shared_mask(
; CHECK-NEXT:    %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %A
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk, i32 23)
; CHECK-NEXT:    %r = add i32 %a, %b
; CHECK-NEXT:    ret i32 %r

define i32 @shared_mask(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A) {
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %S, i32 23)
  %sa = select i1 %A, i32 %a, i32 19
  %sb = select i1 %A, i32 %b, i32 23
  %r = add i32 %sa, %sb
  ret i32 %r
}

; Same with a condition that may be undef. The first load freezes it, and the
; second load finds the mask built on the same %A.fr its select reads.
; CHECK-LABEL: define i32 @shared_mask_undef_condition(
; CHECK:         %A.fr = freeze i1 %A
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %A.fr
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk, i32 23)
; CHECK-NEXT:    %r = add i32 %a, %b
; CHECK-NEXT:    ret i32 %r

define i32 @shared_mask_undef_condition(ptr addrspace(1) %p, i1 noundef %S, i32 %u) {
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %A = icmp ult i32 %u, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %S, i32 23)
  %sa = select i1 %A, i32 %a, i32 19
  %sb = select i1 %A, i32 %b, i32 23
  %r = add i32 %sa, %sb
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
