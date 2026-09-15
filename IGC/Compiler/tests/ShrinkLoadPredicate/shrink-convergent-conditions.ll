;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; A convergent call depends on the set of active lanes, so two of them
; count as one condition only inside a single block.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @convergent_conditions(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %c1 = call noundef i1 @vote(i1 %x)
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 true, i32 13)
; CHECK-NEXT:    br i1 %branch, label %then, label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  then:
; CHECK-NEXT:    %c2 = call noundef i1 @vote(i1 %x)
; CHECK-NEXT:    %s2 = select i1 %c2, i32 %a, i32 0
; CHECK-NEXT:    store i32 %s2, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    br label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  exit:
; CHECK-NEXT:    %s1 = select i1 %c1, i32 %a, i32 0
; CHECK-NEXT:    ret i32 %s1

define i32 @convergent_conditions(ptr addrspace(1) %p, ptr addrspace(1) %out, i1 noundef %x, i1 noundef %branch) {
entry:
  %c1 = call noundef i1 @vote(i1 %x)
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 true, i32 13)
  br i1 %branch, label %then, label %exit

then:
  %c2 = call noundef i1 @vote(i1 %x)
  %s2 = select i1 %c2, i32 %a, i32 0
  store i32 %s2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  %s1 = select i1 %c1, i32 %a, i32 0
  ret i32 %s1
}

declare noundef i1 @vote(i1) convergent nounwind willreturn readnone
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
