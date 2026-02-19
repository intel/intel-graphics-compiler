;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck %s

; Test remateralization of duplicate address calculation instructions comming
; from multiple basic blocks, e.g.:
;   if(cond) {
;       ptr = base + 4;
;       foo(ptr);
;   } else {
;       ptr = base + 4;
;   }
;   return load(ptr);

define i64 @foo(i64 %base, i1 %cond)
{
entry:
  br i1 %cond, label %true-bb, label %false-bb

true-bb:
  %addrTrue = add i64 %base, 4
  %ptrTrue  = inttoptr i64 %addrTrue to i64 addrspace(2)*
  br label %merge-bb

false-bb:
  %addrFalse = add i64 %base, 4
  %ptrFalse  = inttoptr i64 %addrFalse to i64 addrspace(2)*
  br label %merge-bb

merge-bb:
  %addr = phi i64 addrspace(2)* [ %ptrTrue, %true-bb ], [ %ptrFalse, %false-bb ]
  %result = load i64, i64 addrspace(2)* %addr, align 4
  ret i64 %result
  ; CHECK-LABEL: merge-bb:
  ; CHECK: [[ADD:%.*]] = add i64 %base, 4
  ; CHECK-NEXT: [[PTR:%.*]] = inttoptr i64 [[ADD]] to i64 addrspace(2)*
  ; CHECK-NEXT: [[RESULT:%.*]] = load i64, i64 addrspace(2)* [[PTR]], align 4
  ; CHECK-NEXT: ret i64 [[RESULT]]
}
