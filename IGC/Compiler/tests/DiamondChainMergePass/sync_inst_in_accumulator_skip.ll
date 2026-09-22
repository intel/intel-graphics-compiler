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
; Candidate discovery must reject a chain whose *accumulator* block holds a
; synchronisation intrinsic: the pass prepends the whole chain body into the
; accumulator, which would move code across the barrier.  The CFG here is the
; same fully-mergeable triangle chain as triton_side_effect_merge.ll, so the
; workgroup barrier in %body0 is the only reason nothing is merged.

declare void @llvm.genx.GenISA.threadgroupbarrier()
declare void @sink(i32, i32, i32)

define void @sync_inst_in_accumulator_skip(i1 %c, ptr addrspace(1) %p0,
                                           ptr addrspace(1) %p1,
                                           ptr addrspace(1) %p2) {
; CHECK-LABEL: define void @sync_inst_in_accumulator_skip(
; CHECK-NOT:     .dcm
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %p0, align 4
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NEXT:    br label %merge0
; CHECK-NOT:     load
; CHECK:       merge0:
; CHECK-NEXT:    %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
; CHECK-NEXT:    %u0 = add i32 %v0, 1
; CHECK-NEXT:    br i1 %c, label %body1, label %merge1
; CHECK:       body1:
; CHECK-NEXT:    %l1 = load i32, ptr addrspace(1) %p1, align 4
; CHECK-NEXT:    br label %merge1
; CHECK:       merge1:
; CHECK-NEXT:    %v1 = phi i32 [ %l1, %body1 ], [ 0, %merge0 ]
; CHECK-NEXT:    %u1 = add i32 %v1, 1
; CHECK-NEXT:    br i1 %c, label %body2, label %merge2
; CHECK:       body2:
; CHECK-NEXT:    %l2 = load i32, ptr addrspace(1) %p2, align 4
; CHECK-NEXT:    br label %merge2
; CHECK:       merge2:
; CHECK-NEXT:    %v2 = phi i32 [ %l2, %body2 ], [ 0, %merge1 ]
; CHECK-NEXT:    %u2 = add i32 %v2, 1
; CHECK-NEXT:    call void @sink(i32 %u0, i32 %u1, i32 %u2)
; CHECK-NEXT:    ret void

entry:
  br i1 %c, label %body0, label %merge0

body0:
  %l0 = load i32, ptr addrspace(1) %p0, align 4
  call void @llvm.genx.GenISA.threadgroupbarrier()
  br label %merge0

merge0:
  %v0 = phi i32 [ %l0, %body0 ], [ 0, %entry ]
  %u0 = add i32 %v0, 1
  br i1 %c, label %body1, label %merge1

body1:
  %l1 = load i32, ptr addrspace(1) %p1, align 4
  br label %merge1

merge1:
  %v1 = phi i32 [ %l1, %body1 ], [ 0, %merge0 ]
  %u1 = add i32 %v1, 1
  br i1 %c, label %body2, label %merge2

body2:
  %l2 = load i32, ptr addrspace(1) %p2, align 4
  br label %merge2

merge2:
  %v2 = phi i32 [ %l2, %body2 ], [ 0, %merge1 ]
  %u2 = add i32 %v2, 1
  call void @sink(i32 %u0, i32 %u1, i32 %u2)
  ret void
}
