;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-gen-specific-pattern -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; (x == -1) is the carry-out of (x + C) only for C == 1; other constants must
; not be folded into uaddc.

define void @addc_const_one(ptr addrspace(1) %a, ptr addrspace(1) %o) {
  %x = load i32, ptr addrspace(1) %a, align 4
  %add = add i32 %x, 1
  %cmp = icmp eq i32 %x, -1
  %z = zext i1 %cmp to i32
  store i32 %add, ptr addrspace(1) %o, align 4
  %o1 = getelementptr inbounds i32, ptr addrspace(1) %o, i64 1
  store i32 %z, ptr addrspace(1) %o1, align 4
  ret void
}

; CHECK-LABEL: @addc_const_one
; CHECK: call <2 x i32> @llvm.genx.GenISA.uaddc.v2i32.i32(i32 %x, i32 1)

define void @addc_const_zero(ptr addrspace(1) %a, ptr addrspace(1) %o) {
  %x = load i32, ptr addrspace(1) %a, align 4
  %add = add i32 %x, 0
  %cmp = icmp eq i32 %x, -1
  %z = zext i1 %cmp to i32
  store i32 %add, ptr addrspace(1) %o, align 4
  %o1 = getelementptr inbounds i32, ptr addrspace(1) %o, i64 1
  store i32 %z, ptr addrspace(1) %o1, align 4
  ret void
}

; CHECK-LABEL: @addc_const_zero
; CHECK-NOT: uaddc
; CHECK: %add = add i32 %x, 0
; CHECK: icmp eq i32 %x, -1
; CHECK-NOT: uaddc
; CHECK: ret void

define void @addc_const_two(ptr addrspace(1) %a, ptr addrspace(1) %o) {
  %x = load i32, ptr addrspace(1) %a, align 4
  %add = add i32 %x, 2
  %cmp = icmp eq i32 %x, -1
  %z = zext i1 %cmp to i32
  store i32 %add, ptr addrspace(1) %o, align 4
  %o1 = getelementptr inbounds i32, ptr addrspace(1) %o, i64 1
  store i32 %z, ptr addrspace(1) %o1, align 4
  ret void
}

; CHECK-LABEL: @addc_const_two
; CHECK-NOT: uaddc
; CHECK: %add = add i32 %x, 2
; CHECK: icmp eq i32 %x, -1
; CHECK-NOT: uaddc
; CHECK: ret void
