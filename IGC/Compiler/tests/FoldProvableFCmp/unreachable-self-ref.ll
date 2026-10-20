;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-fold-provable-fcmp -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; The pass can run before anything has removed unreachable code, and there an
; instruction may use itself. Once the fold turns %cmp into false,
; simplifyInstruction reduces `%a = or i1 %a, false` to %a itself, which must
; not be passed to replaceAllUsesWith.
;
; ------------------------------------------------

; sitofp i8 is in [-128, 127] and never NaN, so `ogt 1000.0` is always false.
;
; CHECK-LABEL: define i1 @self_ref_in_unreachable_block(
; CHECK-NOT:     fcmp
; CHECK-NOT:     dead:
; CHECK:         ret i1 false
define i1 @self_ref_in_unreachable_block(i8 %v) {
entry:
  %x = sitofp i8 %v to float
  %cmp = fcmp ogt float %x, 1.000000e+03
  br label %exit

dead:
  %a = or i1 %a, %cmp
  br label %dead

exit:
  ret i1 %cmp
}
