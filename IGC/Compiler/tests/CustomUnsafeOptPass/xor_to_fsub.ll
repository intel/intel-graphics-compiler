;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @test_half(half %x) #0 {
entry:
  %0 = bitcast half %x to i16
  %xor = xor i16 %0, -32768
  %1 = bitcast i16 %xor to half
  ret void
}

; CHECK-LABEL: define void @test_half
; CHECK-NOT: xor
; CHECK: %0 = fsub half 0xH0000, %x

define void @test_float(float %x) #0 {
entry:
  %0 = bitcast float %x to i32
  %xor = xor i32 %0, -2147483648
  %1 = bitcast i32 %xor to float
  ret void
}

; CHECK-LABEL: define void @test_float
; CHECK-NOT: xor
; CHECK: %0 = fsub float 0.000000e+00, %x

define void @test_double(double %x) #0 {
entry:
  %0 = bitcast double %x to i64
  %xor = xor i64 %0, -9223372036854775808
  %1 = bitcast i64 %xor to double
  ret void
}

; CHECK-LABEL: define void @test_double
; CHECK-NOT: xor
; CHECK: %0 = fsub double 0.000000e+00, %x

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
