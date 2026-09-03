;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -igc-advmemopt -S \
; RUN:   --regkey=AdvMemOptAggressiveHoist=0 < %s | FileCheck %s --check-prefix=DEFAULT
;
; RUN: igc_opt --opaque-pointers -igc-advmemopt -S \
; RUN:   --regkey=AdvMemOptAggressiveHoist=1 < %s | FileCheck %s --check-prefix=HOIST

; Test: AdvMemOpt::collectOperandInst() and a load whose address depends on a phi.
;
; No diamond here, so the default walk already reaches %bb1 and considers %l1 for
; hoisting into %bb. Operand collection is what refuses: %gep's index is %iv, a
; phi, and collectOperandInst() bails on any phi before the dominance check.
; But %iv's parent %bb is the leading block itself, so the address is already
; available there. AdvMemOptAggressiveHoist runs the check instead of bailing,
; and %gep and %l1 both hoist - the case whenever addresses come from an IV.

define spir_kernel void @phiaddr(i32 %a, ptr addrspace(1) %b, ptr addrspace(1) %b2) {
entry:
  %c0 = icmp slt i32 %a, 13
  br i1 %c0, label %bb, label %end

bb:
  %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
  %l0 = load i32, ptr addrspace(1) %b, align 4
  br label %bb1

bb1:
  %gep = getelementptr inbounds i32, ptr addrspace(1) %b2, i32 %iv
  %l1 = load i32, ptr addrspace(1) %gep, align 4
  %ivnext = add i32 %iv, %l0
  %t = add i32 %l1, %ivnext
  %cc = icmp slt i32 %ivnext, %a
  br i1 %cc, label %bb, label %end

end:
  %r = phi i32 [ %a, %entry ], [ %t, %bb1 ]
  store i32 %r, ptr addrspace(1) %b, align 4
  ret void
}

; The phi operand blocks collection: the address and the load stay put.
;
; DEFAULT-LABEL: bb:
; DEFAULT-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
; DEFAULT-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; DEFAULT-NEXT:    br label %bb1
;
; DEFAULT-LABEL: bb1:
; DEFAULT-NEXT:    %gep = getelementptr inbounds i32, ptr addrspace(1) %b2, i32 %iv
; DEFAULT-NEXT:    %l1 = load i32, ptr addrspace(1) %gep, align 4

; %bb dominates the leading block, so the phi is accepted and both move up.
;
; HOIST-LABEL: bb:
; HOIST-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
; HOIST-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; HOIST-NEXT:    %gep = getelementptr inbounds i32, ptr addrspace(1) %b2, i32 %iv
; HOIST-NEXT:    %l1 = load i32, ptr addrspace(1) %gep, align 4
; HOIST-NEXT:    br label %bb1
;
; HOIST-LABEL: bb1:
; HOIST-NEXT:    %ivnext = add i32 %iv, %l0

!igc.functions = !{!0}

!0 = !{ptr @phiaddr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
