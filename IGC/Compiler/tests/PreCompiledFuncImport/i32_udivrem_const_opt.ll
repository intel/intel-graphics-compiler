;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=32 \
; RUN:         --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport: i32/i16 udiv/urem lowering for power-of-2 constant divisors
; ------------------------------------------------
;
; Verify that PreCompiledFuncImport directly lowers:
;   - udiv by a power-of-2 constant to lshr
;   - urem by a power-of-2 constant to and
;
; And that emulation IS inserted for:
;   - udiv with a non-power-of-2 constant divisor
;   - urem with a non-power-of-2 constant divisor
;   - udiv / urem with a variable divisor

; --- i32 udiv by power-of-2: directly lowered to lshr ---
; CHECK-LABEL:     @udiv_i32_pow2(
; CHECK-NOT:       call {{.*}}precompiled_u32divrem
; CHECK-NOT:       udiv
; CHECK:           lshr i32 %a, 2
define i32 @udiv_i32_pow2(i32 %a) #0 {
  %r = udiv i32 %a, 4
  ret i32 %r
}

; --- i32 udiv by non-power-of-2 constant: emulation call expected ---
; CHECK-LABEL:     @udiv_i32_nonpow2(
; CHECK:           call i32 @precompiled_u32divrem
define i32 @udiv_i32_nonpow2(i32 %a) #0 {
  %r = udiv i32 %a, 3
  ret i32 %r
}

; --- i32 udiv by variable: emulation call expected ---
; CHECK-LABEL:     @udiv_i32_variable(
; CHECK:           call i32 @precompiled_u32divrem
define i32 @udiv_i32_variable(i32 %a, i32 %b) #0 {
  %r = udiv i32 %a, %b
  ret i32 %r
}

; --- i32 urem by non-power-of-2 constant: emulation call expected ---
; CHECK-LABEL:     @urem_i32_nonpow2(
; CHECK:           call i32 @precompiled_u32divrem
define i32 @urem_i32_nonpow2(i32 %a) #0 {
  %r = urem i32 %a, 5
  ret i32 %r
}

; --- i32 urem by power-of-2: directly lowered to and ---
; CHECK-LABEL:     @urem_i32_pow2(
; CHECK-NOT:       call {{.*}}precompiled_u32divrem
; CHECK-NOT:       urem
; CHECK:           and i32 %a, 3
define i32 @urem_i32_pow2(i32 %a) #0 {
  %r = urem i32 %a, 4
  ret i32 %r
}

; --- i32 urem by variable: emulation call expected ---
; CHECK-LABEL:     @urem_i32_variable(
; CHECK:           call i32 @precompiled_u32divrem
define i32 @urem_i32_variable(i32 %a, i32 %b) #0 {
  %r = urem i32 %a, %b
  ret i32 %r
}

; --- i16 udiv by power-of-2: directly lowered to lshr on the narrow type ---
; CHECK-LABEL:     @udiv_i16_pow2(
; CHECK-NOT:       call {{.*}}precompiled_u32divrem
; CHECK-NOT:       udiv
; CHECK:           lshr i16 %a, 3
define i16 @udiv_i16_pow2(i16 %a) #0 {
  %r = udiv i16 %a, 8
  ret i16 %r
}

; --- i16 urem by non-power-of-2 constant: emulation call expected ---
; CHECK-LABEL:     @urem_i16_nonpow2(
; CHECK:           call i32 @precompiled_u32divrem
define i16 @urem_i16_nonpow2(i16 %a) #0 {
  %r = urem i16 %a, 3
  ret i16 %r
}

; --- i16 urem by power-of-2: directly lowered to and on the narrow type ---
; CHECK-LABEL:     @urem_i16_pow2(
; CHECK-NOT:       call {{.*}}precompiled_u32divrem
; CHECK-NOT:       urem
; CHECK:           and i16 %a, 3
define i16 @urem_i16_pow2(i16 %a) #0 {
  %r = urem i16 %a, 4
  ret i16 %r
}

attributes #0 = { nounwind }
