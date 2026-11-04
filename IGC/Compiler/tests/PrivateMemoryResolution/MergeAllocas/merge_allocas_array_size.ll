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

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; Allocation size must account for the constant array size operand of alloca.
; %a allocates 8 x i32 (32 bytes), so it cannot be merged into 4 byte %b.
; Instead %b is merged into %a.

; CHECK-LABEL: @k
; CHECK: [[MERGED:%.*]] = alloca i32, i32 8, align 4
; CHECK-NOT: alloca
; CHECK: store volatile i32 %n, ptr [[MERGED]], align 4
; CHECK: getelementptr inbounds i32, ptr [[MERGED]], i32 7
define spir_kernel void @k(ptr addrspace(1) %out, i32 %n) {
entry:
  %b = alloca i32, align 4
  %a = alloca i32, i32 8, align 4
  br label %phase1

phase1:
  store volatile i32 %n, ptr %b, align 4
  %xb = load volatile i32, ptr %b, align 4
  br label %phase2

phase2:
  %p0 = getelementptr inbounds i32, ptr %a, i32 0
  %p7 = getelementptr inbounds i32, ptr %a, i32 7
  store volatile i32 1, ptr %p0, align 4
  store volatile i32 2, ptr %p7, align 4
  %x7 = load volatile i32, ptr %p7, align 4
  %s = add i32 %x7, %xb
  store i32 %s, ptr addrspace(1) %out, align 4
  ret void
}

; <3 x i8> has a store size of 3 but an alloc size of 4 (v24:32), so
; %v allocates 4 x 4 = 16 bytes. %w allocates 3 x 4 = 12 bytes and is merged
; into %v.

; CHECK-LABEL: @padded_vec
; CHECK: [[MERGED:%.*]] = alloca <3 x i8>, i32 4, align 4
; CHECK-NOT: alloca
; CHECK: store volatile <3 x i8> <i8 1, i8 2, i8 3>, ptr [[MERGED]], align 4
; CHECK: store volatile <3 x i8> <i8 4, i8 5, i8 6>, ptr [[MERGED]], align 4
define spir_kernel void @padded_vec() {
entry:
  %w = alloca <3 x i8>, i32 3, align 4
  %v = alloca <3 x i8>, i32 4, align 4
  br label %phase1

phase1:
  store volatile <3 x i8> <i8 1, i8 2, i8 3>, ptr %w, align 4
  br label %phase2

phase2:
  store volatile <3 x i8> <i8 4, i8 5, i8 6>, ptr %v, align 4
  ret void
}
