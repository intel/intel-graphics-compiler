;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s
;
; Regression: preserve path-local remaps for the first edge in each diamond
; chain when the same condition is revisited in a nested triangle chain.

define spir_kernel void @small_new_repro(ptr addrspace(1) %in, ptr addrspace(1) %out, i1 %c0, i1 %c1) {
; CHECK-LABEL: define spir_kernel void @small_new_repro(
; CHECK:       %{{.*}}.dcm{{[0-9]*}} = phi i32
; CHECK:       %{{.*}}.dcm{{[0-9]*}} = phi i32
; CHECK:       store i32 %{{.*}}.dcm{{[0-9]*}}, ptr addrspace(1) %o0, align 4
; CHECK:       store i32 %{{.*}}.dcm{{[0-9]*}}, ptr addrspace(1) %o1, align 4
; CHECK-NOT: poison
entry:
  %i0 = getelementptr inbounds i32, ptr addrspace(1) %in, i32 0
  %i1 = getelementptr inbounds i32, ptr addrspace(1) %in, i32 1
  %i2 = getelementptr inbounds i32, ptr addrspace(1) %in, i32 2
  %i3 = getelementptr inbounds i32, ptr addrspace(1) %in, i32 3
  %o0 = getelementptr inbounds i32, ptr addrspace(1) %out, i32 0
  %o1 = getelementptr inbounds i32, ptr addrspace(1) %out, i32 1
  %o2 = getelementptr inbounds i32, ptr addrspace(1) %out, i32 2
  %o3 = getelementptr inbounds i32, ptr addrspace(1) %out, i32 3
  br i1 %c0, label %t0, label %m0

t0:
  %l0 = load i32, ptr addrspace(1) %i0, align 4
  br label %m0

m0:
  %v0 = phi i32 [ %l0, %t0 ], [ 0, %entry ]
  %a0 = add i32 %v0, 1
  br i1 %c0, label %t1, label %m1

t1:
  %l1 = load i32, ptr addrspace(1) %i1, align 4
  br label %m1

m1:
  %v1 = phi i32 [ %l1, %t1 ], [ 0, %m0 ]
  %a1 = add i32 %v1, 2
  br i1 %c1, label %t2, label %m2

t2:
  %l2 = load i32, ptr addrspace(1) %i2, align 4
  br label %m2

m2:
  %v2 = phi i32 [ %l2, %t2 ], [ 0, %m1 ]
  %a2 = add i32 %v2, 3
  br i1 %c1, label %t3, label %m3

t3:
  %l3 = load i32, ptr addrspace(1) %i3, align 4
  br label %m3

m3:
  %v3 = phi i32 [ %l3, %t3 ], [ 0, %m2 ]
  %a3 = add i32 %v3, 4
  store i32 %a0, ptr addrspace(1) %o0, align 4
  store i32 %a1, ptr addrspace(1) %o1, align 4
  store i32 %a2, ptr addrspace(1) %o2, align 4
  store i32 %a3, ptr addrspace(1) %o3, align 4
  ret void
}