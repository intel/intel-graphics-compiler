;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

; The final merge block is cloned into both accumulator paths. An external
; PHI must therefore get two incoming entries carrying the original constant,
; while preserving the value arriving from outside the chain.

define i32 @diamond_exit_phi_split(i1 %enter, i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: define i32 @diamond_exit_phi_split(
; CHECK:       chain:
; CHECK-NEXT:    br i1 %c, label %true0, label %false0
; CHECK:       true0:
; CHECK-NEXT:    store i32 1, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    store i32 3, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    br label %exit
; CHECK:       false0:
; CHECK-NEXT:    store i32 2, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    store i32 4, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    br label %exit
; CHECK:       exit:
; CHECK-NEXT:    %result = phi i32 [ 7, %true0 ], [ 42, %extra ], [ 7, %false0 ]
; CHECK-NEXT:    ret i32 %result
entry:
  br i1 %enter, label %chain, label %extra

extra:
  br label %exit

chain:
  br i1 %c, label %true0, label %false0

true0:
  store i32 1, ptr addrspace(1) %out, align 4
  br label %merge0

false0:
  store i32 2, ptr addrspace(1) %out, align 4
  br label %merge0

merge0:
  br i1 %c, label %true1, label %false1

true1:
  store i32 3, ptr addrspace(1) %out, align 4
  br label %merge1

false1:
  store i32 4, ptr addrspace(1) %out, align 4
  br label %merge1

merge1:
  br label %exit

exit:
  %result = phi i32 [ 7, %merge1 ], [ 42, %extra ]
  ret i32 %result
}
