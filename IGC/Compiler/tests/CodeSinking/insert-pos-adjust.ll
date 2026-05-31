;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-hoist-congruent-phi --regkey CodeSinkingMinSize=10 -S < %s | FileCheck %s
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

; The congruent sources of a phi may both depend on an instruction defined in a
; loop header, and may also feed a loop-exit phi whose block is reachable along
; an edge that bypasses the header. Hoisting into the header is legal (the
; loop-exit phi uses the values only on edges dominated by the header), but a
; naive whole-block dominance check on the phi user reports a false violation.
; Check that the two congruent `add`s are hoisted into the header and that the
; pass does not assert.
;
; CHECK-LABEL: @test_loop_exit_phi(
define i32 @test_loop_exit_phi(i1 %enter, i1 %c0, i1 %c1, i1 %c2, i32 %seed) {
entry:
  br i1 %enter, label %header, label %exit

; The congruent adds (add %iv, 1) are hoisted to the top of the header.
; CHECK-LABEL: header:
; CHECK: %iv = phi i32
; CHECK: [[H:%.*]] = add i32 %iv, 1
; CHECK: br i1 %c0, label %leaf.a, label %leaf.b
header:                                           ; preds = %join, %entry
  %iv = phi i32 [ %merged, %join ], [ 0, %entry ]
  %acc = add i32 %iv, %seed
  %big0 = mul i32 %acc, 3
  %big1 = mul i32 %big0, 5
  %big2 = mul i32 %big1, 7
  br i1 %c0, label %leaf.a, label %leaf.b

leaf.a:                                           ; preds = %header
  %addA = add i32 %iv, 1
  %cmpA = icmp ugt i32 %addA, 200
  br i1 %cmpA, label %brk.a, label %join

leaf.b:                                           ; preds = %header
  %addB = add i32 %iv, 1
  %cmpB = icmp ugt i32 %addB, 200
  br i1 %cmpB, label %brk.b, label %join

; %merged is the congruent phi: both incoming values are `add %iv, 1`.
join:                                             ; preds = %leaf.a, %leaf.b
  %merged = phi i32 [ %addA, %leaf.a ], [ %addB, %leaf.b ]
  br label %header

brk.a:                                            ; preds = %leaf.a
  br label %exit

brk.b:                                            ; preds = %leaf.b
  br label %exit

; %exit is reachable directly from %entry, so %header does not dominate it.
; %addA / %addB are used here on edges that *are* dominated by %header.
exit:                                             ; preds = %brk.a, %brk.b, %entry
  %res = phi i32 [ %addA, %brk.a ], [ %addB, %brk.b ], [ 0, %entry ]
  ret i32 %res
}

declare void @foo()

