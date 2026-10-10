;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; Test that a module-forced SIMD width limits the widest SIMD used for register pressure
; estimation. Same kernel and settings as cloneAddressArithmeticWidestSIMD.ll, where @main is
; rematerialized at SIMD32. Here the module is forced to SIMD16, so @main is estimated at SIMD16
; and not rematerialized.

; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=10 -remat-widest-simd --dce | FileCheck %s

; CHECK-LABEL: define spir_kernel void @main(
; CHECK-NOT: remat
; CHECK: ret void
define spir_kernel void @main(ptr addrspace(1) %base, i64 %offset, i64 %I, i64 %J) {
  %baseArith = ptrtoint ptr addrspace(1) %base to i64
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3

  %a0 = add i64 %baseArith, 100780848
  %a1 = add i64 %a0, %basePtr
  %a2 = add i64 %a1, %offsetI
  %a3 = add i64 %a2, %offsetJ
  %a4 = inttoptr i64 %a3 to ptr addrspace(1)
  %r0 = load double, ptr addrspace(1) %a4, align 8

  %rr0 = fmul double %r0, 2.0

  store double %rr0, ptr addrspace(1) %a4
  ret void
}

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"csInfo", !2}
!2 = !{!"forcedSIMDSize", i8 16}
