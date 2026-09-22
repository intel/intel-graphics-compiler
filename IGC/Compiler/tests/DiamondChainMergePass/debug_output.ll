;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge

define spir_kernel void @debug_output(i32 %x, ptr addrspace(1) %out) #0 {
entry:
  %cmp = icmp sgt i32 %x, 0
  br i1 %cmp, label %dst.true, label %dst.false

dst.true:                                          ; preds = %entry
  %t0 = add i32 %x, 1
  br label %dst.false

dst.false:                                         ; preds = %dst.true, %entry
  %m0 = phi i32 [ %x, %entry ], [ %t0, %dst.true ]
  br i1 %cmp, label %src.true, label %src.false

src.true:                                          ; preds = %dst.false
  %t1 = add i32 %m0, 2
  br label %src.false

src.false:                                         ; preds = %src.true, %dst.false
  %m1 = phi i32 [ %m0, %dst.false ], [ %t1, %src.true ]
  store i32 %m1, ptr addrspace(1) %out, align 4
  ret void
}

attributes #0 = { convergent mustprogress noinline nounwind optnone }
