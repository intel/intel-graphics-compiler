;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s --check-prefixes=CHECK,DEFAULT
; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --igc-predicated-load-max-mask-depth=5 --verify -S %s | FileCheck %s --check-prefixes=CHECK,DEEP

; The component that proves the implication, %q, sits five `and` levels below the
; later load's predicate %m5. The default mask depth of 4 stops the walk one
; level short of it, so no reuse happens; raising the cap by one finds it and
; %b becomes a select over %a.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @deep_mask(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %m1 = and i1 %q, %c1
; CHECK-NEXT:    %m2 = and i1 %m1, %c2
; CHECK-NEXT:    %m3 = and i1 %m2, %c3
; CHECK-NEXT:    %m4 = and i1 %m3, %c4
; CHECK-NEXT:    %m5 = and i1 %m4, %c5
; DEFAULT-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m5, i32 37)
; DEFAULT-NEXT:    %r = add i32 %a, %b
; DEEP-NEXT:    %[[SEL:[0-9]+]] = select i1 %m5, i32 %a, i32 37
; DEEP-NEXT:    %r = add i32 %a, %[[SEL]]
; CHECK-NEXT:    ret i32 %r

define i32 @deep_mask(ptr addrspace(1) %p, i1 noundef %q, i1 %c1, i1 %c2, i1 %c3, i1 %c4, i1 %c5) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %m1 = and i1 %q, %c1
  %m2 = and i1 %m1, %c2
  %m3 = and i1 %m2, %c3
  %m4 = and i1 %m3, %c4
  %m5 = and i1 %m4, %c5
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m5, i32 37)
  %r = add i32 %a, %b
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
