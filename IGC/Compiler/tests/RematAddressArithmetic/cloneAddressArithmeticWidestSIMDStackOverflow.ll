;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; Test that the widest SIMD treats stack overflow detection helpers like the OpenCL
; backend does: they do not make a kernel a subroutine kernel, so @main is estimated at
; SIMD32 and rematerialized. @with_subroutine calls a real subroutine, stays at SIMD16 and
; is not rematerialized. Estimated pressure is 5 GRFs at SIMD16 and 9 GRFs at SIMD32,
; against a limit of 6 GRFs (5% of 128).

; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o - -GenXCodeGenModule -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=5 --dce | FileCheck %s --check-prefix=GUESS
; RUN: igc_opt -platformlnl --opaque-pointers %s -S -o - -GenXCodeGenModule -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=5 -remat-widest-simd --dce | FileCheck %s --check-prefix=WIDEST

; GUESS-NOT: remat

; WIDEST-LABEL: define spir_kernel void @with_subroutine(
; WIDEST-NOT: remat
; WIDEST-LABEL: define spir_kernel void @main(
; WIDEST: [[CLONED:%.*]] = inttoptr i64 %a3 to ptr addrspace(1), !remat
; WIDEST-NEXT: load double, ptr addrspace(1) [[CLONED]]

define spir_kernel void @main(ptr addrspace(1) %base, i64 %offset, i64 %I0, i64 %J0) {
  call spir_func void @__stackoverflow_detection()
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane64 = zext i16 %lane to i64
  %I = add i64 %I0, %lane64
  %J = add i64 %J0, %lane64
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

define spir_kernel void @with_subroutine(ptr addrspace(1) %base, i64 %offset, i64 %I0, i64 %J0) {
  call spir_func void @helper()
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane64 = zext i16 %lane to i64
  %I = add i64 %I0, %lane64
  %J = add i64 %J0, %lane64
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

define internal spir_func void @helper() #0 {
  ret void
}

define internal spir_func void @__stackoverflow_detection() #0 {
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

attributes #0 = { noinline }

!igc.functions = !{!0, !3, !6, !7}
!0 = !{ptr @main, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @__stackoverflow_detection, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 2}
!6 = !{ptr @with_subroutine, !1}
!7 = !{ptr @helper, !4}
