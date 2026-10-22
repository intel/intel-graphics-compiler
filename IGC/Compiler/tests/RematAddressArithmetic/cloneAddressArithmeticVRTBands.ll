;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-14-plus
; UNSUPPORTED: system-windows


; Xe3 / PTL, default 5% proximity
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-clone-address-arithmetic \
; RUN:   --regkey=RematChainLimit=10 --regkey=RematSingleFlowRematEnabled=0 --regkey=RematFlowThreshold=100 \
; RUN:   --regkey=RematRPELimit=5 --regkey=RematVRTProximityPercent=5 --platformPtl \
; RUN:   | FileCheck %s --check-prefix=P5

; Xe3 / PTL, tighter 2% proximity
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-clone-address-arithmetic \
; RUN:   --regkey=RematChainLimit=10 --regkey=RematSingleFlowRematEnabled=0 --regkey=RematFlowThreshold=100 \
; RUN:   --regkey=RematRPELimit=5 --regkey=RematVRTProximityPercent=2 --platformPtl \
; RUN:   | FileCheck %s --check-prefix=P2

; Non-VRT platform
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-clone-address-arithmetic \
; RUN:   --regkey=RematChainLimit=10 --regkey=RematSingleFlowRematEnabled=0 --regkey=RematFlowThreshold=100 \
; RUN:   --regkey=RematRPELimit=5 --platformlnl | FileCheck %s --check-prefix=FALLBACK

; Xe3P platform (CRI)
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-clone-address-arithmetic \
; RUN:   --regkey=RematChainLimit=10 --regkey=RematSingleFlowRematEnabled=0 --regkey=RematFlowThreshold=100 \
; RUN:   --regkey=RematRPELimit=5 --regkey=RematVRTProximityPercent=5 --platformCri | FileCheck %s --check-prefix=P5CRI

; Max register pressure of the four kernels, as reported by the register
; estimator: @small = 8, @fight = 164, @far = 188, @spill = 268 GRF.

; Xe3 table (ModeTableXe3): {32,10} {64,10} {96,10} {128,8} {160,6} {192,5} {256,4}
;   8   -> 32 band, every lower band has the same 10 threads    -> no remat
;   164 -> 192 band (5 threads), nearest lower band with more
;          threads is 160 (6 threads), 160 + 5% = 168 >= 164    -> remat
;   188 -> 192 band, 168 < 188                                  -> no remat
;   268 -> above the top budget (256), spills regardless        -> remat
; P5-LABEL: define spir_kernel void @small
; P5-NOT: !remat
; P5-LABEL: define spir_kernel void @fight
; P5: !remat
; P5-LABEL: define spir_kernel void @far
; P5-NOT: !remat
; P5-LABEL: define spir_kernel void @spill
; P5: !remat

; Xe3P table (ModeTableXe3P): {32,8} {64,8} {96,8} {128,8} {160,8} {192,8} {256,4} {512,4}
; Budgets 32..192 all yield 8 threads, so no kernel at or below 192 can gain a
; thread by rematting; 268 lands in the 512 band, whose nearest lower band with
; a different thread count is 192 (192 + 5% = 201 < 268). Nothing remats here.
; P5CRI-LABEL: define spir_kernel void @small
; P5CRI-NOT: !remat
; P5CRI-LABEL: define spir_kernel void @fight
; P5CRI-NOT: !remat
; P5CRI-LABEL: define spir_kernel void @far
; P5CRI-NOT: !remat
; P5CRI-LABEL: define spir_kernel void @spill
; P5CRI-NOT: !remat

; Same Xe3 table as P5, but the tighter 2% proximity puts the 160 band
; threshold at 163, just below @fight's 164, so @fight no longer remats.
; P2-LABEL: define spir_kernel void @small
; P2-NOT: !remat
; P2-LABEL: define spir_kernel void @fight
; P2-NOT: !remat
; P2-LABEL: define spir_kernel void @far
; P2-NOT: !remat
; P2-LABEL: define spir_kernel void @spill
; P2: !remat

; Non-VRT platform: the VRT path is skipped entirely and the plain
; RematRPELimit cutoff applies, which every kernel here clears.

; FALLBACK-LABEL: define spir_kernel void @small
; FALLBACK: !remat
; FALLBACK-LABEL: define spir_kernel void @fight
; FALLBACK: !remat
; FALLBACK-LABEL: define spir_kernel void @far
; FALLBACK: !remat
; FALLBACK-LABEL: define spir_kernel void @spill
; FALLBACK: !remat

define spir_kernel void @small(ptr addrspace(1) %base, i64 %offset, i64 %I, i64 %J) {
  %baseArith = ptrtoint ptr addrspace(1) %base to i64
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3
  %a0_1 = add i64 %baseArith, 100780849
  %a1_1 = add i64 %a0_1, %basePtr
  %a2_1 = add i64 %a1_1, %offsetI
  %a3_1 = add i64 %a2_1, %offsetJ
  %a4_1 = inttoptr i64 %a3_1 to ptr addrspace(1)
  %r_1 = load double, ptr addrspace(1) %a4_1, align 8
  %rr = fmul double %r_1, 2.0
  store double %rr, ptr addrspace(1) %a4_1
  ret void
}

define spir_kernel void @fight(ptr addrspace(1) %base, i64 %offset, i64 %I, i64 %J) {
  %baseArith = ptrtoint ptr addrspace(1) %base to i64
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3
  %a0_1 = add i64 %baseArith, 100780849
  %a1_1 = add i64 %a0_1, %basePtr
  %a2_1 = add i64 %a1_1, %offsetI
  %a3_1 = add i64 %a2_1, %offsetJ
  %a4_1 = inttoptr i64 %a3_1 to ptr addrspace(1)
  %r_1 = load double, ptr addrspace(1) %a4_1, align 8
  %a0_2 = add i64 %baseArith, 100780850
  %a1_2 = add i64 %a0_2, %basePtr
  %a2_2 = add i64 %a1_2, %offsetI
  %a3_2 = add i64 %a2_2, %offsetJ
  %a4_2 = inttoptr i64 %a3_2 to ptr addrspace(1)
  %r_2 = load <79 x double>, ptr addrspace(1) %a4_2, align 8
  store <79 x double> %r_2, ptr addrspace(1) %a4_2
  store double %r_1, ptr addrspace(1) %a4_1
  ret void
}

define spir_kernel void @far(ptr addrspace(1) %base, i64 %offset, i64 %I, i64 %J) {
  %baseArith = ptrtoint ptr addrspace(1) %base to i64
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3
  %a0_1 = add i64 %baseArith, 100780849
  %a1_1 = add i64 %a0_1, %basePtr
  %a2_1 = add i64 %a1_1, %offsetI
  %a3_1 = add i64 %a2_1, %offsetJ
  %a4_1 = inttoptr i64 %a3_1 to ptr addrspace(1)
  %r_1 = load double, ptr addrspace(1) %a4_1, align 8
  %a0_2 = add i64 %baseArith, 100780850
  %a1_2 = add i64 %a0_2, %basePtr
  %a2_2 = add i64 %a1_2, %offsetI
  %a3_2 = add i64 %a2_2, %offsetJ
  %a4_2 = inttoptr i64 %a3_2 to ptr addrspace(1)
  %r_2 = load <91 x double>, ptr addrspace(1) %a4_2, align 8
  store <91 x double> %r_2, ptr addrspace(1) %a4_2
  store double %r_1, ptr addrspace(1) %a4_1
  ret void
}

define spir_kernel void @spill(ptr addrspace(1) %base, i64 %offset, i64 %I, i64 %J) {
  %baseArith = ptrtoint ptr addrspace(1) %base to i64
  %basePtr = mul nuw nsw i64 %offset, 207368
  %offsetI = mul nsw i64 %I, 1288
  %offsetJ = shl nsw i64 %J, 3
  %a0_1 = add i64 %baseArith, 100780849
  %a1_1 = add i64 %a0_1, %basePtr
  %a2_1 = add i64 %a1_1, %offsetI
  %a3_1 = add i64 %a2_1, %offsetJ
  %a4_1 = inttoptr i64 %a3_1 to ptr addrspace(1)
  %r_1 = load double, ptr addrspace(1) %a4_1, align 8
  %a0_2 = add i64 %baseArith, 100780850
  %a1_2 = add i64 %a0_2, %basePtr
  %a2_2 = add i64 %a1_2, %offsetI
  %a3_2 = add i64 %a2_2, %offsetJ
  %a4_2 = inttoptr i64 %a3_2 to ptr addrspace(1)
  %r_2 = load <131 x double>, ptr addrspace(1) %a4_2, align 8
  store <131 x double> %r_2, ptr addrspace(1) %a4_2
  store double %r_1, ptr addrspace(1) %a4_1
  ret void
}
