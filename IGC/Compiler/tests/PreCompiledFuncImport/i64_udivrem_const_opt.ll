;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=1 \
; RUN:         --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport: i64 udiv/urem lowering for power-of-2 constant divisors
; ------------------------------------------------
;
; Verify that PreCompiledFuncImport directly lowers:
;   - udiv i64 by a power-of-2 constant to lshr
;   - urem i64 by a power-of-2 constant to and
;
; And that emulation IS inserted for:
;   - udiv i64 with a non-power-of-2 constant divisor
;   - urem i64 with a non-power-of-2 constant divisor
;   - udiv i64 / urem i64 with a variable divisor

; --- i64 udiv by power-of-2: directly lowered to lshr ---
; CHECK-LABEL:     @udiv_i64_pow2(
; CHECK-NOT:       call {{.*}}__igcbuiltin_u64_udiv
; CHECK-NOT:       udiv
; CHECK:           lshr i64 %a, 2
define i64 @udiv_i64_pow2(i64 %a) #0 {
  %r = udiv i64 %a, 4
  ret i64 %r
}

; --- i64 udiv by non-power-of-2 constant: emulation call expected ---
; CHECK-LABEL:     @udiv_i64_nonpow2(
; CHECK:           call i64 @__igcbuiltin_u64_udiv_sp
define i64 @udiv_i64_nonpow2(i64 %a) #0 {
  %r = udiv i64 %a, 3
  ret i64 %r
}

; --- i64 udiv by variable: emulation call expected ---
; CHECK-LABEL:     @udiv_i64_variable(
; CHECK:           call i64 @__igcbuiltin_u64_udiv_sp
define i64 @udiv_i64_variable(i64 %a, i64 %b) #0 {
  %r = udiv i64 %a, %b
  ret i64 %r
}

; --- i64 urem by non-power-of-2 constant: emulation call expected ---
; CHECK-LABEL:     @urem_i64_nonpow2(
; CHECK:           call i64 @__igcbuiltin_u64_urem_sp
define i64 @urem_i64_nonpow2(i64 %a) #0 {
  %r = urem i64 %a, 5
  ret i64 %r
}

; --- i64 urem by power-of-2: directly lowered to and ---
; CHECK-LABEL:     @urem_i64_pow2(
; CHECK-NOT:       call {{.*}}__igcbuiltin_u64_urem
; CHECK-NOT:       urem
; CHECK:           and i64 %a, 3
define i64 @urem_i64_pow2(i64 %a) #0 {
  %r = urem i64 %a, 4
  ret i64 %r
}

; --- i64 urem by variable: emulation call expected ---
; CHECK-LABEL:     @urem_i64_variable(
; CHECK:           call i64 @__igcbuiltin_u64_urem_sp
define i64 @urem_i64_variable(i64 %a, i64 %b) #0 {
  %r = urem i64 %a, %b
  ret i64 %r
}

attributes #0 = { nounwind }
