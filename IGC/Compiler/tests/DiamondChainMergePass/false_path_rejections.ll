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

; The short-circuit safety checks must inspect the false path after the true
; path succeeds. Keep barriers, volatile accesses, and escaping definitions on
; their original paths.

declare void @llvm.genx.GenISA.threadgroupbarrier()

define void @false_accumulator_barrier(i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @false_accumulator_barrier(
; CHECK:       false0:
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    br label %merge0
; CHECK:       merge0:
; CHECK-NEXT:    br i1 %c, label %true1, label %false1
; CHECK:       true1:
; CHECK-NEXT:    store i32 1, ptr addrspace(1) %out, align 4
; CHECK:       false1:
; CHECK-NEXT:    store i32 2, ptr addrspace(1) %out, align 4
entry:
  br i1 %c, label %true0, label %false0
true0:
  br label %merge0
false0:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  br label %merge0
merge0:
  br i1 %c, label %true1, label %false1
true1:
  store i32 1, ptr addrspace(1) %out, align 4
  ret void
false1:
  store i32 2, ptr addrspace(1) %out, align 4
  ret void
}

define void @false_source_barrier(i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @false_source_barrier(
; CHECK:       merge0:
; CHECK-NEXT:    br i1 %c, label %true1, label %false1
; CHECK:       true1:
; CHECK-NEXT:    store i32 1, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
; CHECK:       false1:
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    store i32 2, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
entry:
  br i1 %c, label %true0, label %false0
true0:
  br label %merge0
false0:
  br label %merge0
merge0:
  br i1 %c, label %true1, label %false1
true1:
  store i32 1, ptr addrspace(1) %out, align 4
  ret void
false1:
  call void @llvm.genx.GenISA.threadgroupbarrier()
  store i32 2, ptr addrspace(1) %out, align 4
  ret void
}

define void @false_source_volatile(i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @false_source_volatile(
; CHECK:       merge0:
; CHECK-NEXT:    br i1 %c, label %true1, label %false1
; CHECK:       true1:
; CHECK-NEXT:    store i32 1, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
; CHECK:       false1:
; CHECK-NEXT:    store volatile i32 2, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
entry:
  br i1 %c, label %true0, label %false0
true0:
  br label %merge0
false0:
  br label %merge0
merge0:
  br i1 %c, label %true1, label %false1
true1:
  store i32 1, ptr addrspace(1) %out, align 4
  ret void
false1:
  store volatile i32 2, ptr addrspace(1) %out, align 4
  ret void
}

define void @false_source_escaping_def(i1 %c, i32 %x, ptr addrspace(1) %out) {
; CHECK-LABEL: define void @false_source_escaping_def(
; CHECK:       merge0:
; CHECK-NEXT:    br i1 %c, label %true1, label %false1
; CHECK:       false1:
; CHECK-NEXT:    %value = add i32 %x, 1
; CHECK-NEXT:    br label %outside
; CHECK:       outside:
; CHECK-NEXT:    store i32 %value, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
entry:
  br i1 %c, label %true0, label %false0
true0:
  br label %merge0
false0:
  br label %merge0
merge0:
  br i1 %c, label %true1, label %false1
true1:
  ret void
false1:
  %value = add i32 %x, 1
  br label %outside
outside:
  store i32 %value, ptr addrspace(1) %out, align 4
  ret void
}
