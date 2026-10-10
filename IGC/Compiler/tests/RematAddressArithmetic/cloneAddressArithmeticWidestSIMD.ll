;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; Test register pressure estimation at the widest allowed SIMD on LNL, where abort on spill
; makes bestGuessSIMDSize() pick SIMD16. Estimated pressure is 8 GRFs at SIMD16 and 16 GRFs
; at SIMD32, against a limit of 12 GRFs (10% of 128).
; At the best-guess SIMD all kernels are estimated at SIMD16 and not rematerialized.
; At the widest SIMD @main is estimated at SIMD32 and rematerialized, while @reqd_sg16 stays at
; its required sub-group size of 16 and @ray_query at the SIMD16 ray query width, so neither is
; rematerialized.

; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=10 --dce | FileCheck %s --check-prefix=GUESS
; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=10 -remat-widest-simd --dce | FileCheck %s --check-prefix=WIDEST

; GUESS-NOT: remat

; WIDEST-LABEL: define spir_kernel void @main(
; WIDEST: [[CLONED:%.*]] = inttoptr i64 %{{.*}} to ptr addrspace(1), !remat
; WIDEST-NEXT: load double, ptr addrspace(1) [[CLONED]]
; WIDEST-LABEL: define spir_kernel void @reqd_sg16(
; WIDEST-NOT: remat
; WIDEST-LABEL: define spir_kernel void @ray_query(
; WIDEST-NOT: remat
; WIDEST: ret void
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

define spir_kernel void @reqd_sg16(ptr addrspace(1) %base, i64 %offset, i64 %I, i64 %J) {
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

define spir_kernel void @ray_query(ptr addrspace(1) %base, i64 %offset, i64 %I, i64 %J) {
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
!1 = !{!"FuncMD", !2, !3, !5, !6}
!2 = !{!"FuncMDMap[0]", ptr @reqd_sg16}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDMap[1]", ptr @ray_query}
!6 = !{!"FuncMDValue[1]", !7}
!7 = !{!"hasSyncRTCalls", i1 true}
