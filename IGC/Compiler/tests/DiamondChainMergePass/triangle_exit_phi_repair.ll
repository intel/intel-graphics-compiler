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
;
; Merging this triangle chain gives the exit PHI two chain predecessors in
; place of merge1. SSA repair must add the missing incoming entry and retain
; the unrelated predecessor's value. The input already supplies poison on the
; chain path, so the repaired entry preserves its semantics. The store in
; merge0 forces regular merging rather than the PHI-only whole-chain shortcut.

define i32 @triangle_exit_phi_repair(i1 %enter, i1 %c, ptr addrspace(1) %out) {
; CHECK-LABEL: define i32 @triangle_exit_phi_repair(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %enter, label %chain, label %extra
; CHECK:       extra:
; CHECK-NEXT:    br label %exit
; CHECK:       chain:
; CHECK-NEXT:    br i1 %c, label %body0, label %merge0
; CHECK:       body0:
; CHECK-NEXT:    store i32 1, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    store i32 2, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    store i32 3, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    br label %exit
; CHECK:       merge0:
; CHECK-NEXT:    store i32 2, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    br label %exit
; CHECK:       exit:
; CHECK-NEXT:    %result = phi i32 [ poison, %body0 ], [ 42, %extra ], [ poison, %merge0 ]
; CHECK-NEXT:    ret i32 %result
entry:
  br i1 %enter, label %chain, label %extra

extra:
  br label %exit

chain:
  br i1 %c, label %body0, label %merge0

body0:
  store i32 1, ptr addrspace(1) %out, align 4
  br label %merge0

merge0:
  store i32 2, ptr addrspace(1) %out, align 4
  br i1 %c, label %body1, label %merge1

body1:
  store i32 3, ptr addrspace(1) %out, align 4
  br label %merge1

merge1:
  br label %exit

exit:
  %result = phi i32 [ poison, %merge1 ], [ 42, %extra ]
  ret i32 %result
}
