;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; Regression: path-specific PHI coming from the previous accumulator merge must
; be folded using the cumulative value map instead of being replaced with
; poison when the source triple is a triangle.
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

define spir_kernel void @dst_merge_with_triangle_source(i32 %x, ptr addrspace(1) %out) #0 {
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %dst.true, label %dst.false

dst.true:
  %t0 = add i32 %x, 1
  br label %dst.merge

dst.false:
  %f0 = add i32 %x, 2
  br label %dst.merge

dst.merge:
  %m0 = phi i32 [ %t0, %dst.true ], [ %f0, %dst.false ]
  br i1 %cmp, label %src.true, label %src.false

src.true:
  %t1 = add i32 %m0, 3
  br label %src.false

src.false:
  %m1 = phi i32 [ %m0, %dst.merge ], [ %t1, %src.true ]
  store i32 %m1, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @dst_merge_with_triangle_source(
; CHECK:       entry:
; CHECK:       dst.true:
; CHECK:         [[T0:%.*]] = add i32 %x, 1
; CHECK:         [[T1:%.*]] = add i32 [[T0]], 3
; CHECK:         store i32 [[T1]], ptr addrspace(1) %out, align 4
; CHECK:         ret void
; CHECK:       dst.false:
; CHECK:         [[F0:%.*]] = add i32 %x, 2
; CHECK:         store i32 [[F0]], ptr addrspace(1) %out, align 4
; CHECK:         ret void
; CHECK-NOT: poison

attributes #0 = { convergent mustprogress noinline nounwind optnone }
