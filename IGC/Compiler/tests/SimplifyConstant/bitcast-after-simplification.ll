;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -SimplifyConstant -S < %s 2>&1 | FileCheck %s

; This test checks that SimplifyConstant pass emits a bitcast following
; simplification.
;
; Before the transition to opaque pointers, the pass accepted the following pattern:
; @g = dso_local unnamed_addr addrspace(2) constant [2 x i32] [i32 1065353216, i32 -1082130432], align 4
; %gep = getelementptr inbounds [2 x i32], [2 x i32] addrspace(2)* @g, i64 0, i64 %a
; %bc = bitcast i32 addrspace(2)* %30 to float addrspace(2)*
; %load = load float, float addrspace(2)* %bc, align 4
;
; In opaque pointer mode, the pattern changes to:
; @g = dso_local unnamed_addr addrspace(2) constant [2 x i32] [i32 1065353216, i32 -1082130432], align 4
; %gep = getelementptr inbounds [2 x i32], ptr addrspace(2) @g, i64 0, i64 %a
; %load = load float, ptr addrspace(2) %gep, align 4
;
; After simplification, the load expects a different primitive type and given
; that the types are bitwise compatible, the pass can emit a bitcast.

; CHECK: [[SELECT:%.*]] = select i1 {{%.*}}, i32 -1082130432, i32 1065353216
; CHECK: [[BC:%.*]] = bitcast i32 [[SELECT]] to float
; CHECK: store float [[BC]], ptr {{%.*}}, align 4

@g = dso_local unnamed_addr addrspace(2) constant [2 x i32] [i32 1065353216, i32 -1082130432], align 4

define spir_kernel void @test_bitcast_after_simplification(i64 %a, ptr %b) {
  %gep = getelementptr inbounds [2 x i32], ptr addrspace(2) @g, i64 0, i64 %a
  %load = load float, ptr addrspace(2) %gep, align 4
  store float %load, ptr %b, align 4
  ret void
}
