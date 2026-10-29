;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

; Test remateralization of address calculation that is a PHI instruction with
; a single incomming value.

define i64 @foo(i64 %base, i1 %cond)
{
entry:
  %addr = add i64 %base, 4
  %ptr  = inttoptr i64 %addr to i64 addrspace(2)*
  br label %exit

exit:
  %addrPhi = phi i64 addrspace(2)* [ %ptr, %entry ]
  %result = load i64, i64 addrspace(2)* %addrPhi, align 4
  ret i64 %result
  ; CHECK-LABEL: exit:
  ; CHECK: [[ADD:%.*]] = add i64 %base, 4
  ; CHECK-TYPED-NEXT: [[PTR:%.*]] = inttoptr i64 [[ADD]] to i64 addrspace(2)*
  ; CHECK-OPAQUE-NEXT: [[PTR:%.*]] = inttoptr i64 [[ADD]] to ptr addrspace(2)
  ; CHECK-TYPED-NEXT: [[RESULT:%.*]] = load i64, i64 addrspace(2)* [[PTR]], align 4
  ; CHECK-OPAQUE-NEXT: [[RESULT:%.*]] = load i64, ptr addrspace(2) [[PTR]], align 4
  ; CHECK-NEXT: ret i64 [[RESULT]]
}
