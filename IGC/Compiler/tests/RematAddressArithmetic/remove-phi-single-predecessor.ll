;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck %s

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
  ; CHECK-NEXT: [[PTR:%.*]] = inttoptr i64 [[ADD]] to i64 addrspace(2)*
  ; CHECK-NEXT: [[RESULT:%.*]] = load i64, i64 addrspace(2)* [[PTR]], align 4
  ; CHECK-NEXT: ret i64 [[RESULT]]
}
