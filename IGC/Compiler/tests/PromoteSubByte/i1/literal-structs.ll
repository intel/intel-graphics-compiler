;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S < %s | FileCheck %s

; Literal structs, such as the { iN, i1 } result of the *.with.overflow
; intrinsics, are promoted to literal structs of the promoted elements.
; Intrinsics are not promoted, so a promoted instruction that uses their result
; converts it element by element first.

declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32)

; CHECK-LABEL: define spir_kernel void @insertvalue_promoted_i1(
; CHECK:         [[CMP:%.*]] = icmp slt i32 %n, 0
; CHECK-NEXT:    [[CMP_I8:%.*]] = zext i1 [[CMP]] to i8
; CHECK-NEXT:    [[R:%.*]] = insertvalue { i32, i8 } { i32 1, i8 0 }, i8 [[CMP_I8]], 1
; CHECK-NEXT:    store { i32, i8 } [[R]], ptr addrspace(1) %out, align 4
define spir_kernel void @insertvalue_promoted_i1(i32 %n, ptr addrspace(1) %out) {
  %cmp = icmp slt i32 %n, 0
  %r = insertvalue { i32, i1 } { i32 1, i1 false }, i1 %cmp, 1
  store { i32, i1 } %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @insertvalue_into_intrinsic_result(
; CHECK:         %ov = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
; CHECK-NEXT:    [[SUM:%.*]] = extractvalue { i32, i1 } %ov, 0
; CHECK-NEXT:    [[A0:%.*]] = insertvalue { i32, i8 } poison, i32 [[SUM]], 0
; CHECK-NEXT:    [[OVF:%.*]] = extractvalue { i32, i1 } %ov, 1
; CHECK-NEXT:    [[OVF_I8:%.*]] = zext i1 [[OVF]] to i8
; CHECK-NEXT:    [[A1:%.*]] = insertvalue { i32, i8 } [[A0]], i8 [[OVF_I8]], 1
; CHECK-NEXT:    [[R:%.*]] = insertvalue { i32, i8 } [[A1]], i32 7, 0
; CHECK-NEXT:    store { i32, i8 } [[R]], ptr addrspace(1) %out, align 4
define spir_kernel void @insertvalue_into_intrinsic_result(i32 %a, i32 %b, ptr addrspace(1) %out) {
  %ov = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %r = insertvalue { i32, i1 } %ov, i32 7, 0
  store { i32, i1 } %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @select_mixed(
; CHECK:         %ov = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i32 %n, 0
; CHECK-NEXT:    [[CMP_I8:%.*]] = zext i1 [[CMP]] to i8
; CHECK-NEXT:    [[OTHER:%.*]] = insertvalue { i32, i8 } zeroinitializer, i8 [[CMP_I8]], 1
; CHECK-NEXT:    [[C:%.*]] = trunc i8 %c to i1
; CHECK-NEXT:    [[SUM:%.*]] = extractvalue { i32, i1 } %ov, 0
; CHECK-NEXT:    [[A0:%.*]] = insertvalue { i32, i8 } poison, i32 [[SUM]], 0
; CHECK-NEXT:    [[OVF:%.*]] = extractvalue { i32, i1 } %ov, 1
; CHECK-NEXT:    [[OVF_I8:%.*]] = zext i1 [[OVF]] to i8
; CHECK-NEXT:    [[A1:%.*]] = insertvalue { i32, i8 } [[A0]], i8 [[OVF_I8]], 1
; CHECK-NEXT:    %s = select i1 [[C]], { i32, i8 } [[A1]], { i32, i8 } [[OTHER]]
; CHECK-NEXT:    store { i32, i8 } %s, ptr addrspace(1) %out, align 4
define spir_kernel void @select_mixed(i1 %c, i32 %a, i32 %b, i32 %n, ptr addrspace(1) %out) {
  %ov = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %cmp = icmp slt i32 %n, 0
  %other = insertvalue { i32, i1 } { i32 0, i1 false }, i1 %cmp, 1
  %s = select i1 %c, { i32, i1 } %ov, { i32, i1 } %other
  store { i32, i1 } %s, ptr addrspace(1) %out, align 4
  ret void
}

; Both operands come straight from the intrinsic; the select still produces the
; promoted struct, and the extractvalue of the flag is truncated back to i1.

; CHECK-LABEL: define spir_kernel void @select_both_unpromoted(
; CHECK:         %ov0 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
; CHECK-NEXT:    %ov1 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %b, i32 %a)
; CHECK:         [[OV0:%.*]] = insertvalue { i32, i8 } {{%.*}}, i8 {{%.*}}, 1
; CHECK:         [[OV1:%.*]] = insertvalue { i32, i8 } {{%.*}}, i8 {{%.*}}, 1
; CHECK-NEXT:    %s = select i1 {{%.*}}, { i32, i8 } [[OV0]], { i32, i8 } [[OV1]]
; CHECK-NEXT:    [[F:%.*]] = extractvalue { i32, i8 } %s, 1
; CHECK-NEXT:    [[F_I1:%.*]] = trunc i8 [[F]] to i1
; CHECK-NEXT:    [[Z:%.*]] = zext i1 [[F_I1]] to i32
; CHECK-NEXT:    store i32 [[Z]], ptr addrspace(1) %out, align 4
define spir_kernel void @select_both_unpromoted(i1 %c, i32 %a, i32 %b, ptr addrspace(1) %out) {
  %ov0 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %ov1 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %b, i32 %a)
  %s = select i1 %c, { i32, i1 } %ov0, { i32, i1 } %ov1
  %f = extractvalue { i32, i1 } %s, 1
  %z = zext i1 %f to i32
  store i32 %z, ptr addrspace(1) %out, align 4
  ret void
}
