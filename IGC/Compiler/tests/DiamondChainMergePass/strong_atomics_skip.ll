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
; Companion of weak_atomics_merge.ll: check 5 (noVolatileOrAtomic) rejects an
; atomic whose ordering is stronger than monotonic, per instruction kind. Each
; chain below is shaped exactly like the merging chains in that file and only
; differs in the ordering, so the whole candidate must be dropped and the CFG
; must survive untouched.

define void @strong_atomic_load(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @strong_atomic_load(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    br label %merge0
; CHECK:       merge0:
; CHECK-NEXT:    %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
; CHECK-NEXT:    store i32 %v0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    br i1 %c, label %body1, label %merge1
; CHECK:       body1:
; CHECK-NEXT:    %a1 = load atomic i32, ptr addrspace(1) %p1 acquire, align 4
; CHECK-NEXT:    br label %merge1
; CHECK:       merge1:
; CHECK-NEXT:    %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
; CHECK-NEXT:    store i32 %v1, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
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
  %a1 = load atomic i32, ptr addrspace(1) %p1 acquire, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}

define void @strong_atomic_store(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @strong_atomic_store(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    br label %merge0
; CHECK:       merge0:
; CHECK-NEXT:    %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
; CHECK-NEXT:    store i32 %v0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    br i1 %c, label %body1, label %merge1
; CHECK:       body1:
; CHECK-NEXT:    store atomic i32 1, ptr addrspace(1) %p1 release, align 4
; CHECK-NEXT:    br label %merge1
; CHECK:       merge1:
; CHECK-NEXT:    %v1 = phi i32 [ 1, %body1 ], [ 0, %merge0 ]
; CHECK-NEXT:    store i32 %v1, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
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
  store atomic i32 1, ptr addrspace(1) %p1 release, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ 1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}

define void @strong_atomicrmw(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @strong_atomicrmw(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    br label %merge0
; CHECK:       merge0:
; CHECK-NEXT:    %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
; CHECK-NEXT:    store i32 %v0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    br i1 %c, label %body1, label %merge1
; CHECK:       body1:
; CHECK-NEXT:    %a1 = atomicrmw add ptr addrspace(1) %p1, i32 1 seq_cst, align 4
; CHECK-NEXT:    br label %merge1
; CHECK:       merge1:
; CHECK-NEXT:    %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
; CHECK-NEXT:    store i32 %v1, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
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
  %a1 = atomicrmw add ptr addrspace(1) %p1, i32 1 seq_cst, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}

define void @strong_cmpxchg(i1 %c, ptr addrspace(1) %p0, ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @strong_cmpxchg(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    br label %merge0
; CHECK:       merge0:
; CHECK-NEXT:    %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
; CHECK-NEXT:    store i32 %v0, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    br i1 %c, label %body1, label %merge1
; CHECK:       body1:
; CHECK-NEXT:    %x1 = cmpxchg ptr addrspace(1) %p1, i32 0, i32 1 acquire monotonic, align 4
; CHECK-NEXT:    %a1 = extractvalue { i32, i1 } %x1, 0
; CHECK-NEXT:    br label %merge1
; CHECK:       merge1:
; CHECK-NEXT:    %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
; CHECK-NEXT:    store i32 %v1, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    ret void
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
  %x1 = cmpxchg ptr addrspace(1) %p1, i32 0, i32 1 acquire monotonic, align 4
  %a1 = extractvalue { i32, i1 } %x1, 0
  br label %merge1

merge1:
  %v1 = phi i32 [ %a1, %body1 ], [ 0, %merge0 ]
  store i32 %v1, ptr addrspace(1) %p1, align 4
  ret void
}
