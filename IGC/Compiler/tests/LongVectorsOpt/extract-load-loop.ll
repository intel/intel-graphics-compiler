;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; An extract whose index is computed in a loop loads its element at the extract, as long as nothing can write the
; loaded memory between the long load and any iteration of the loop. Otherwise it reads from a private copy of the
; vector.

; CHECK-LABEL: @sum_in_loop(
; CHECK-NOT:     load <64 x float>
; CHECK:       entry:
; CHECK-NEXT:    br label %loop
; CHECK:       loop:
; CHECK:         [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %src, i32 0, i32 %i
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK-NEXT:    %s.next = fadd float %s, [[E]]
; CHECK:       exit:
; CHECK-NEXT:    store float %s.next, ptr addrspace(1) %dst, align 4
define void @sum_in_loop(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %v = load <64 x float>, ptr addrspace(1) %src, align 4
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %s = phi float [ 0.0, %entry ], [ %s.next, %loop ]
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %loop, label %exit
exit:
  store float %s.next, ptr addrspace(1) %dst, align 4
  ret void
}

; Alias analysis proves that a store to a local variable doesn't write the loaded memory.
; CHECK-LABEL: @private_store_in_loop(
; CHECK-NOT:     load <64 x float>
; CHECK:       loop:
; CHECK:         store float %a, ptr %tmp, align 4
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %src, i32 0, i32 %i
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
define float @private_store_in_loop(ptr addrspace(1) %src, float %a) {
entry:
  %tmp = alloca float, align 4
  %v = load <64 x float>, ptr addrspace(1) %src, align 4
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %s = phi float [ 0.0, %entry ], [ %s.next, %loop ]
  store float %a, ptr %tmp, align 4
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %loop, label %exit
exit:
  ret float %s.next
}

; A store that alias analysis proves can't write the loaded memory.
; CHECK-LABEL: @noalias_store_in_loop(
; CHECK-NOT:     load <64 x float>
; CHECK:       loop:
; CHECK:         [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %src, i32 0, i32 %i
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK:         store float {{.*}}, ptr addrspace(1) %dst, align 4
define void @noalias_store_in_loop(ptr addrspace(1) noalias %src, ptr addrspace(1) noalias %dst) {
entry:
  %v = load <64 x float>, ptr addrspace(1) %src, align 4
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %s = phi float [ 0.0, %entry ], [ %s.next, %loop ]
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  store float %s.next, ptr addrspace(1) %dst, align 4
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %loop, label %exit
exit:
  ret void
}

; A write after the loop that loads the vector again doesn't matter: the long load runs again before the extract.
; CHECK-LABEL: @nested_loop(
; CHECK-NOT:     load <64 x float>
; CHECK:       inner:
; CHECK:         [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %src, i32 0, i32 %i
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
define void @nested_loop(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  br label %outer
outer:
  %k = phi i32 [ 0, %entry ], [ %k.next, %outer.latch ]
  %v = load <64 x float>, ptr addrspace(1) %src, align 4
  br label %inner
inner:
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  %s = phi float [ 0.0, %outer ], [ %s.next, %inner ]
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %inner, label %outer.latch
outer.latch:
  %p = getelementptr float, ptr addrspace(1) %dst, i32 %k
  store float %s.next, ptr addrspace(1) %p, align 4
  %k.next = add i32 %k, 1
  %oc = icmp ult i32 %k.next, 4
  br i1 %oc, label %outer, label %exit
exit:
  ret void
}

; A global store in the loop may write the loaded memory, even after the extract: the next iteration reads it, so
; the extract reads from a copy.
; CHECK-LABEL: @global_store_in_loop(
; CHECK:       entry:
; CHECK-NEXT:    [[COPY:%.*]] = alloca <64 x float>
; CHECK:       loop:
; CHECK:         getelementptr <64 x float>, ptr [[COPY]], i32 0
define float @global_store_in_loop(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %v = load <64 x float>, ptr addrspace(1) %src, align 4
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  %s = phi float [ 0.0, %entry ], [ %s.next, %latch ]
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  br label %latch
latch:
  store float %s.next, ptr addrspace(1) %dst, align 4
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %loop, label %exit
exit:
  ret float %s.next
}

; The same for a store between the long load and the loop.
; CHECK-LABEL: @store_before_loop(
; CHECK:       entry:
; CHECK-NEXT:    [[COPY:%.*]] = alloca <64 x float>
; CHECK:       loop:
; CHECK:         getelementptr <64 x float>, ptr [[COPY]], i32 0
define float @store_before_loop(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %v = load <64 x float>, ptr addrspace(1) %src, align 4
  store float 0.0, ptr addrspace(1) %dst, align 4
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %s = phi float [ 0.0, %entry ], [ %s.next, %loop ]
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %loop, label %exit
exit:
  ret float %s.next
}

; A call may write any memory, so the extract reads from a copy.
; CHECK-LABEL: @call_in_loop(
; CHECK:       entry:
; CHECK-NEXT:    [[COPY:%.*]] = alloca <64 x float>
; CHECK:       loop:
; CHECK:         getelementptr <64 x float>, ptr [[COPY]], i32 0
declare void @f()
define float @call_in_loop(ptr addrspace(1) %src) {
entry:
  %v = load <64 x float>, ptr addrspace(1) %src, align 4
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %s = phi float [ 0.0, %entry ], [ %s.next, %loop ]
  call void @f()
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %loop, label %exit
exit:
  ret float %s.next
}

; Only stateless loads are checked for writes, so an extract from an ldrawvector in a loop reads from a copy.
; CHECK-LABEL: @ldraw_in_loop(
; CHECK:       entry:
; CHECK-NEXT:    [[COPY:%.*]] = alloca <64 x float>
; CHECK:       loop:
; CHECK:         getelementptr <64 x float>, ptr [[COPY]], i32 0
define float @ldraw_in_loop(ptr addrspace(2490368) %buf, i32 %off) {
entry:
  %v = call <64 x float> @llvm.genx.GenISA.ldrawvector.indexed.v64f32.p2490368(ptr addrspace(2490368) %buf, i32 %off, i32 16, i1 false)
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %s = phi float [ 0.0, %entry ], [ %s.next, %loop ]
  %e = extractelement <64 x float> %v, i32 %i
  %s.next = fadd float %s, %e
  %i.next = add i32 %i, 1
  %c = icmp ult i32 %i.next, 64
  br i1 %c, label %loop, label %exit
exit:
  ret float %s.next
}

declare <64 x float> @llvm.genx.GenISA.ldrawvector.indexed.v64f32.p2490368(ptr addrspace(2490368), i32, i32, i1)
