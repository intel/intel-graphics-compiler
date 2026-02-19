;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows, llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=0 --dce | FileCheck %s

define spir_kernel void @main(double addrspace(1)* %base, i64 %offset, i64 %I, i64 %J) {
; CHECK: define spir_kernel void @main(double addrspace(1)* [[BASE:%.*]], i64 [[OFFSET:%.*]], i64 %I, i64 %J)

  %baseArith = ptrtoint double addrspace(1)* %base to i64
  ; CHECK: [[FIRSTBASE:%.*]] = ptrtoint double addrspace(1)* [[BASE]] to i64
  ; CHECK: [[FIRSTOFFSET:%.*]] = mul nuw nsw i64 [[OFFSET]], 207368
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3

  %a0 = add i64 %baseArith, 100780848
  %a1 = add i64 %a0, %basePtr
  %a2 = add i64 %a1, %offsetI
  %a3 = add i64 %a2, %offsetJ
  %a4 = inttoptr i64 %a3 to double addrspace(1)*
  %r0 = load double, double addrspace(1)* %a4, align 8

  %rr0 = fmul double %r0, 2.0

  ; CHECK: [[SECONDBASE:%.*]] = ptrtoint double addrspace(1)* [[BASE]] to i64
  ; CHECK: [[SECONDOFFSET:%.*]] = mul nuw nsw i64 [[OFFSET]], 207368
  store double %rr0, double addrspace(1)* %a4
  ret void
}


define spir_func void @bar(double addrspace(1)* %base, i64 %offset, i64 %I, i64 %J) {

; CHECK: define spir_func void @bar(double addrspace(1)* [[BASE:%.*]], i64 [[OFFSET:%.*]], i64 %I, i64 %J)

  %baseArith = ptrtoint double addrspace(1)* %base to i64
  ; CHECK: [[FIRSTBASE:%.*]] = ptrtoint double addrspace(1)* [[BASE]] to i64
  ; CHECK: [[FIRSTOFFSET:%.*]] = mul nuw nsw i64 [[OFFSET]], 207368
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3

  %a0 = add i64 %baseArith, 100780848
  %a1 = add i64 %a0, %basePtr
  %a2 = add i64 %a1, %offsetI
  %a3 = add i64 %a2, %offsetJ
  %a4 = inttoptr i64 %a3 to double addrspace(1)*
  %a5 = inttoptr i64 %a3 to i64 addrspace(1)*
  %r0 = load double, double addrspace(1)* %a4, align 8

  %rr0 = fmul double %r0, 2.0

  ; CHECK: [[SECONDBASE:%.*]] = ptrtoint double addrspace(1)* [[BASE]] to i64
  ; CHECK: [[SECONDOFFSET:%.*]] = mul nuw nsw i64 [[OFFSET]], 207368
  %cast_a5 = bitcast i64 addrspace(1)* %a5 to double addrspace(1)*
  store double %rr0, double addrspace(1)* %cast_a5
  ret void
}
