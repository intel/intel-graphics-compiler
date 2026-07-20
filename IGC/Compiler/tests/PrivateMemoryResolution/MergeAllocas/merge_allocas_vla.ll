;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers --igc-ocl-merge-allocas -S %s | FileCheck %s
; ------------------------------------------------
; MergeAllocasOCL
; ------------------------------------------------

; VLAs don't have predictable sizes. Cannot be merged with fixed size allocas.

; Reduced repro from IGC-16609: the VLA %2 must not become the merge container
; (no %MergedAlloca), and both allocas must survive so the IR stays valid.
; This may be optimized by folding the zext inst into an immediate value.

; CHECK-LABEL: @vla_not_merged
; CHECK-NOT: MergedAlloca
; CHECK: %[[CNT:.*]] = zext i32 0 to i64
; CHECK: alloca [32 x i16], i64 %[[CNT]], align 64
; CHECK: alloca [32 x i16], i64 1, align 64
define spir_kernel void @vla_not_merged() {
cond-add-join:
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %cond-add-join
  %0 = or i32 0, 0
  %1 = zext i32 0 to i64
  %2 = alloca [32 x i16], i64 %1, align 64
  %3 = alloca [32 x i16], i64 1, align 64
  br label %._crit_edge
}

; CHECK-LABEL: @vla_and_fixed_used
; CHECK-NOT: MergedAlloca
; CHECK: %[[SZ:.*]] = zext i32 %n to i64
; CHECK: alloca [32 x i16], i64 %[[SZ]], align 64
; CHECK: alloca [32 x i16], align 64
define spir_kernel void @vla_and_fixed_used(i32 %c, i32 %n) {
entry:
  %sz = zext i32 %n to i64
  %vla = alloca [32 x i16], i64 %sz, align 64
  %fixed = alloca [32 x i16], align 64
  %cmp = icmp ne i32 %c, 0
  br i1 %cmp, label %vla.bb, label %fixed.bb

vla.bb:                                           ; preds = %entry
  store i16 7, ptr %vla, align 2
  br label %exit

fixed.bb:                                         ; preds = %entry
  store i16 9, ptr %fixed, align 2
  br label %exit

exit:                                             ; preds = %fixed.bb, %vla.bb
  ret void
}
