;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s
;
; Check 5 (noVolatileOrAtomic) classifies atomics by instruction kind and only
; rejects orderings stronger than monotonic. Monotonic load/store/rmw/cmpxchg
; must therefore NOT block the merge. No other test in this directory contains
; an atomic instruction, so the per-kind arms of hasVolatileOrAtomic() are never
; entered; every chain here flattens with the atomic carried along unchanged.

define void @weak_atomic_load(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @weak_atomic_load(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %[[L0:.*]] = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    store i32 %[[L0]], ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    %[[A:.*]] = load atomic i32, ptr addrspace(1) %p1 monotonic, align 4
; CHECK-NEXT:    store i32 %[[A]], ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK:       merge0:
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK-NOT:     body1:
; CHECK-NOT:     merge1:
entry:
  br i1 %c, label %body0, label %merge0

body0:
  %l0 = load i32, ptr addrspace(1) %p0, align 4
  br label %merge0

merge0:
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  store i32 %v0, ptr addrspace(1) %p1, align 4
  br i1 %c, label %body1, label %merge1

body1:
  %a1 = load atomic i32, ptr addrspace(1) %p1 monotonic, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}

define void @weak_atomic_store(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @weak_atomic_store(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %[[L0:.*]] = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    store i32 %[[L0]], ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    store atomic i32 1, ptr addrspace(1) %p1 monotonic, align 4
; CHECK-NEXT:    store i32 1, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK:       merge0:
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK-NOT:     body1:
; CHECK-NOT:     merge1:
entry:
  br i1 %c, label %body0, label %merge0

body0:
  %l0 = load i32, ptr addrspace(1) %p0, align 4
  br label %merge0

merge0:
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  store i32 %v0, ptr addrspace(1) %p1, align 4
  br i1 %c, label %body1, label %merge1

body1:
  store atomic i32 1, ptr addrspace(1) %p1 monotonic, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ 1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}

define void @weak_atomicrmw(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @weak_atomicrmw(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %[[L0:.*]] = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    store i32 %[[L0]], ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    %[[A:.*]] = atomicrmw add ptr addrspace(1) %p1, i32 1 monotonic, align 4
; CHECK-NEXT:    store i32 %[[A]], ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK:       merge0:
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK-NOT:     body1:
; CHECK-NOT:     merge1:
entry:
  br i1 %c, label %body0, label %merge0

body0:
  %l0 = load i32, ptr addrspace(1) %p0, align 4
  br label %merge0

merge0:
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  store i32 %v0, ptr addrspace(1) %p1, align 4
  br i1 %c, label %body1, label %merge1

body1:
  %a1 = atomicrmw add ptr addrspace(1) %p1, i32 1 monotonic, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}

define void @weak_cmpxchg(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @weak_cmpxchg(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %[[L0:.*]] = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    store i32 %[[L0]], ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    %[[X:.*]] = cmpxchg ptr addrspace(1) %p1, i32 0, i32 1 monotonic monotonic, align 4
; CHECK-NEXT:    %[[E:.*]] = extractvalue { i32, i1 } %[[X]], 0
; CHECK-NEXT:    store i32 %[[E]], ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK:       merge0:
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    store i32 0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
; CHECK-NOT:     body1:
; CHECK-NOT:     merge1:
entry:
  br i1 %c, label %body0, label %merge0

body0:
  %l0 = load i32, ptr addrspace(1) %p0, align 4
  br label %merge0

merge0:
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  store i32 %v0, ptr addrspace(1) %p1, align 4
  br i1 %c, label %body1, label %merge1

body1:
  %x1 = cmpxchg ptr addrspace(1) %p1, i32 0, i32 1 monotonic monotonic, align 4
  %a1 = extractvalue { i32, i1 } %x1, 0
  br label %merge1

merge1:
  %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}
