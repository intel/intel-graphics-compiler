;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableMergingOfAllocasWithDifferentType=0 --igc-ocl-merge-allocas -S %s | FileCheck %s
; ------------------------------------------------
; MergeAllocasOCL
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; <3 x i8> has a store size of 3 but an alloc size of 4 (v24:32), so %v
; allocates 4 x 4 = 16 bytes and 13 byte %a is merged into %v. Without
; padding (3 x 4 = 12 bytes) %a would be chosen as the merged alloca and %v
; would not fit in it.

; CHECK-LABEL: @padded_vec_different_type
; CHECK: [[MERGED:%.*]] = alloca <3 x i8>, i32 4, align 4
; CHECK-NOT: alloca
; CHECK: store volatile i8 1, ptr [[MERGED]], align 1
; CHECK: store volatile <3 x i8> <i8 4, i8 5, i8 6>, ptr [[MERGED]], align 4
define spir_kernel void @padded_vec_different_type() {
entry:
  %a = alloca [13 x i8], align 1
  %v = alloca <3 x i8>, i32 4, align 4
  br label %phase1

phase1:
  store volatile i8 1, ptr %a, align 1
  br label %phase2

phase2:
  store volatile <3 x i8> <i8 4, i8 5, i8 6>, ptr %v, align 4
  ret void
}
