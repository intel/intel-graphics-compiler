;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-hoist-congruent-phi -inputcs --regkey CodeSinkingMinSize=10 -S < %s | FileCheck %s
; ------------------------------------------------
; CodeSinking
; ------------------------------------------------
; Check that instructions are hoisted before their uses
;

define float @test(i1 %cond) {
; CHECK-LABEL: entry:
; %loadB and %subB must be hoisted before %mulA2
; CHECK-NEXT: [[HOISTEDLOADB:%.*]] = load float, float addrspace(65554)* inttoptr (i32 48 to float addrspace(65554)*), align 16
; CHECK-NEXT: [[HOISTEDSUBB:%.*]] = fsub fast float -0.000000e+00, [[HOISTEDLOADB]]
; next instruction is not a hoisted instruction
; CHECK-NEXT: %mulA2 = fmul fast float [[HOISTEDSUBB]], [[HOISTEDLOADB]]
; %mulA2 is the `leave` instruction for the rest of hoisted instructions
; CHECK-NEXT: [[HOISTEDMULB1:%.*]] = fmul fast float [[HOISTEDSUBB]], [[HOISTEDLOADB]]
; CHECK-NEXT: [[HOISTEDMULB3:%.*]] = fmul fast float [[HOISTEDMULB1]], %mulA2
; CHECK-NEXT: br i1 %cond, label %then, label %exit
entry:
  %loadA = load float, float addrspace(65554)* inttoptr (i32 48 to float addrspace(65554)*), align 16
  %subA = fsub fast float -0.000000e+00, %loadA
  %mulA1 = fmul fast float %subA, %loadA
  %mulA2 = fmul fast float %subA, %loadA
  %mulA3 = fmul fast float %mulA1, %mulA2
  br i1 %cond, label %then, label %exit
; nothing left in the then BB
; CHECK-LABEL: then:
; CHECK-NEXT: br label %exit
then:
  %loadB = load float, float addrspace(65554)* inttoptr (i32 48 to float addrspace(65554)*), align 16
  %subB = fsub fast float -0.000000e+00, %loadB
  %mulB1 = fmul fast float %subB, %loadB
  ; note the use of %mulA2 in the following instruction
  %mulB3 = fmul fast float %mulB1, %mulA2
  br label %exit
; no phi instruction, hoisted %mulB3 is the return value
; CHECK-LABEL: exit:
; CHECK: ret float [[HOISTEDMULB3]]
exit:
  %retVal = phi float [ %mulB3, %then ], [ %mulA3, %entry ]
  ret float %retVal
}

declare void @foo()

