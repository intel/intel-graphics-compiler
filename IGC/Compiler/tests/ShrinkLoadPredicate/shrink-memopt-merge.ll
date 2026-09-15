;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-memopt -S %s | FileCheck %s

; Adjacent loads under one shrunk mask still merge into a single memory
; operation. Loads with different use predicates need different lane masks
; and therefore stay separate.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @merge_after_shrinking(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond
; CHECK-NEXT:    %[[V0:[0-9]+]] = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, <2 x i32> zeroinitializer)
; CHECK-NEXT:    %[[V1:[0-9]+]] = extractelement <2 x i32> %[[V0]], i32 0
; CHECK-NEXT:    %[[V2:[0-9]+]] = extractelement <2 x i32> %[[V0]], i32 1
; CHECK-NEXT:    %sa = select i1 %cond, i32 %[[V1]], i32 %other
; CHECK-NEXT:    %sb = select i1 %cond, i32 %[[V2]], i32 %other
; CHECK-NEXT:    %r = add i32 %sa, %sb
; CHECK-NEXT:    ret i32 %r

define i32 @merge_after_shrinking(ptr addrspace(1) %p, i1 %q, i1 noundef %cond, i32 %other) {
entry:
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %q, i32 0)
  %sa = select i1 %cond, i32 %a, i32 %other
  %sb = select i1 %cond, i32 %b, i32 %other
  %r = add i32 %sa, %sb
  ret i32 %r
}

; CHECK-LABEL: define i32 @no_merge_different_conditions(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %g0
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %predload.shrunk1 = and i1 %q, %g1
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk1, i32 0)
; CHECK-NEXT:    %sa = select i1 %g0, i32 %a, i32 %other
; CHECK-NEXT:    %sb = select i1 %g1, i32 %b, i32 %other
; CHECK-NEXT:    %r = add i32 %sa, %sb
; CHECK-NEXT:    ret i32 %r

define i32 @no_merge_different_conditions(ptr addrspace(1) %p, i1 %q, i1 noundef %g0, i1 noundef %g1, i32 %other) {
entry:
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %q, i32 0)
  %sa = select i1 %g0, i32 %a, i32 %other
  %sb = select i1 %g1, i32 %b, i32 %other
  %r = add i32 %sa, %sb
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }

!igc.functions = !{!0, !1}
!0 = !{ptr @merge_after_shrinking, !2}
!1 = !{ptr @no_merge_different_conditions, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
