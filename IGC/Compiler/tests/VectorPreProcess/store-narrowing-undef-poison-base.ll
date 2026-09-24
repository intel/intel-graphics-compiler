;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -platformdg2 -igc-vectorpreprocess -S %s -o - | FileCheck %s

; A store of a vector whose trailing lanes were never inserted is narrowed to
; the inserted lanes, so the rest of memory is left untouched. LLVM 22
; InstCombine rewrites the undef base of such a chain to
; <poison, poison, poison, undef>, which must be narrowed like plain undef.

target triple = "spir64-unknown-unknown"

; CHECK-LABEL: @poison_undef_base
; CHECK: store <3 x double>
; CHECK-NOT: store <4 x double>
define void @poison_undef_base(ptr addrspace(1) %p, double %x, double %y, double %z) {
  %1 = insertelement <4 x double> <double poison, double poison, double poison, double undef>, double %x, i64 0
  %2 = insertelement <4 x double> %1, double %y, i64 1
  %3 = insertelement <4 x double> %2, double %z, i64 2
  store <4 x double> %3, ptr addrspace(1) %p, align 32
  ret void
}

; A defined lane in the base has to be stored, so no narrowing.
; CHECK-LABEL: @defined_lane_base
; CHECK: store <4 x double>
define void @defined_lane_base(ptr addrspace(1) %p, double %x, double %y, double %z) {
  %1 = insertelement <4 x double> <double poison, double poison, double poison, double 1.000000e+00>, double %x, i64 0
  %2 = insertelement <4 x double> %1, double %y, i64 1
  %3 = insertelement <4 x double> %2, double %z, i64 2
  store <4 x double> %3, ptr addrspace(1) %p, align 32
  ret void
}

; Narrowing is not limited to 4 lanes: 5 inserted lanes of an <8 x float> are
; stored as <4 x float> at offset 0 plus a float at element 4.
; CHECK-LABEL: @five_of_eight
; CHECK: [[P0:%.*]] = getelementptr float, ptr addrspace(1) %p, i32 0
; CHECK: store <4 x float> {{.*}}, ptr addrspace(1) [[P0]]
; CHECK: [[P4:%.*]] = getelementptr float, ptr addrspace(1) %p, i32 4
; CHECK: store float %e, ptr addrspace(1) [[P4]]
; CHECK-NOT: store <8 x float>
define void @five_of_eight(ptr addrspace(1) %p, float %a, float %b, float %c, float %d, float %e) {
  %1 = insertelement <8 x float> undef, float %a, i64 0
  %2 = insertelement <8 x float> %1, float %b, i64 1
  %3 = insertelement <8 x float> %2, float %c, i64 2
  %4 = insertelement <8 x float> %3, float %d, i64 3
  %5 = insertelement <8 x float> %4, float %e, i64 4
  store <8 x float> %5, ptr addrspace(1) %p, align 32
  ret void
}

; CHECK-LABEL: @six_of_eight
; CHECK: [[P0:%.*]] = getelementptr float, ptr addrspace(1) %p, i32 0
; CHECK: store <4 x float> {{.*}}, ptr addrspace(1) [[P0]]
; CHECK: [[P4:%.*]] = getelementptr float, ptr addrspace(1) %p, i32 4
; CHECK: store <2 x float> {{.*}}, ptr addrspace(1) [[P4]]
; CHECK-NOT: store <8 x float>
define void @six_of_eight(ptr addrspace(1) %p, float %a, float %b, float %c, float %d, float %e, float %f) {
  %1 = insertelement <8 x float> undef, float %a, i64 0
  %2 = insertelement <8 x float> %1, float %b, i64 1
  %3 = insertelement <8 x float> %2, float %c, i64 2
  %4 = insertelement <8 x float> %3, float %d, i64 3
  %5 = insertelement <8 x float> %4, float %e, i64 4
  %6 = insertelement <8 x float> %5, float %f, i64 5
  store <8 x float> %6, ptr addrspace(1) %p, align 32
  ret void
}

; CHECK-LABEL: @seven_of_eight
; CHECK: [[P0:%.*]] = getelementptr float, ptr addrspace(1) %p, i32 0
; CHECK: store <4 x float> {{.*}}, ptr addrspace(1) [[P0]]
; CHECK: [[P4:%.*]] = getelementptr float, ptr addrspace(1) %p, i32 4
; CHECK: store <3 x float> {{.*}}, ptr addrspace(1) [[P4]]
; CHECK-NOT: store <8 x float>
define void @seven_of_eight(ptr addrspace(1) %p, float %a, float %b, float %c, float %d, float %e, float %f, float %g) {
  %1 = insertelement <8 x float> undef, float %a, i64 0
  %2 = insertelement <8 x float> %1, float %b, i64 1
  %3 = insertelement <8 x float> %2, float %c, i64 2
  %4 = insertelement <8 x float> %3, float %d, i64 3
  %5 = insertelement <8 x float> %4, float %e, i64 4
  %6 = insertelement <8 x float> %5, float %f, i64 5
  %7 = insertelement <8 x float> %6, float %g, i64 6
  store <8 x float> %7, ptr addrspace(1) %p, align 32
  ret void
}
