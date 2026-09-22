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
; Companion of sync_inst_in_accumulator_skip.ll.  There the barrier sits in the
; *accumulator* block, which is rejected while the accumulator is classified,
; before the chain-growing loop ever runs.  Here the barrier sits in a *source*
; block of a later triple, so the accumulator is accepted and the rejection has
; to come from the second synchronisation check, the one inside the loop that
; grows the chain.  Neither triple may be collected, so no candidate is formed
; and the CFG survives untouched.

declare void @llvm.genx.GenISA.threadgroupbarrier()
declare void @sink(i32, i32)

define void @sync_inst_in_source_skip(i1 %c, ptr addrspace(1) %p0,
                                      ptr addrspace(1) %p1) {
; CHECK-LABEL: define void @sync_inst_in_source_skip(
; CHECK-NOT:     .dcm
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    br label %merge0
; CHECK:       merge0:
; CHECK-NEXT:    %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
; CHECK-NEXT:    %u0 = add i32 %v0, 1
; CHECK-NEXT:    br i1 %c, label %body1, label %merge1
; CHECK:       body1:
; CHECK-NEXT:    %l1 = load i32, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    br label %merge1
; CHECK:       merge1:
; CHECK-NEXT:    %v1 = phi i32 [ %l1, %body1 ], [ 0, %merge0 ]
; CHECK-NEXT:    %u1 = add i32 %v1, 1
; CHECK-NEXT:    call void @sink(i32 %u0, i32 %u1)
; CHECK-NEXT:    ret void

entry:
  br i1 %c, label %body0, label %merge0

body0:
  %l0 = load i32, ptr addrspace(1) %p0, align 4
  br label %merge0

merge0:
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  %u0 = add i32 %v0, 1
  br i1 %c, label %body1, label %merge1

body1:
  %l1 = load i32, ptr addrspace(1) %p1, align 4
  call void @llvm.genx.GenISA.threadgroupbarrier()
  br label %merge1

merge1:
  %v1 = phi i32 [ %l1, %body1 ], [ 0, %merge0 ]
  %u1 = add i32 %v1, 1
  call void @sink(i32 %u0, i32 %u1)
  ret void
}
