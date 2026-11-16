;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; On PTL the first-compile remat estimates pressure at SIMD32. The 24 store addresses live from entry to exit put the
; kernel over the largest GRF mode: 288 GRFs at SIMD32, 144 at SIMD16. Rematerializing them brings it to 196 and 98,
; low enough for the XE3 VRT SIMD16 drop, so the kernel would not keep SIMD32 and the remat is rolled back. The same
; remat is kept when it is not a first-compile run, and when the SIMD32 width is fixed.

; RUN: igc_opt -platformPtl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 -remat-first-compile --dce | FileCheck %s --check-prefix=ROLLBACK
; RUN: igc_opt -platformPtl --opaque-pointers %s -S -o /dev/null -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 -remat-first-compile --regkey=RematLog=1 --regkey=RematLogToErr=1 2>&1 | FileCheck %s --check-prefix=LOG
; RUN: igc_opt -platformPtl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 --dce | FileCheck %s --check-prefix=KEEP
; RUN: igc_opt -platformPtl --opaque-pointers %s -S -o - -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 -remat-first-compile --regkey=ForceOCLSIMDWidth=32 --dce | FileCheck %s --check-prefix=KEEP

; ROLLBACK: store <4 x double> %acc11, ptr addrspace(1) %s2_1, align 8
; ROLLBACK-NOT: !remat
; LOG: Pressure budget: 256 MaxPressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG: SIMD16 drop after rematerialization, rolling back
; KEEP: store <4 x double> {{.*}}, ptr addrspace(1) %cloned_{{.*}}
; KEEP: !remat
define spir_kernel void @main(ptr addrspace(1) %base, i64 %offset, i64 %I) {
  %baseArith = ptrtoint ptr addrspace(1) %base to i64
  %offsetI = mul nsw i64 %I, 1288
  %baseOff = add i64 %baseArith, %offset
  %basePtr = add i64 %baseOff, %offsetI
  %s1_1 = add i64 %basePtr, 8
  %s2_1 = inttoptr i64 %s1_1 to ptr addrspace(1)
  %s1_2 = add i64 %basePtr, 16
  %s2_2 = inttoptr i64 %s1_2 to ptr addrspace(1)
  %s1_3 = add i64 %basePtr, 24
  %s2_3 = inttoptr i64 %s1_3 to ptr addrspace(1)
  %s1_4 = add i64 %basePtr, 32
  %s2_4 = inttoptr i64 %s1_4 to ptr addrspace(1)
  %s1_5 = add i64 %basePtr, 40
  %s2_5 = inttoptr i64 %s1_5 to ptr addrspace(1)
  %s1_6 = add i64 %basePtr, 48
  %s2_6 = inttoptr i64 %s1_6 to ptr addrspace(1)
  %s1_7 = add i64 %basePtr, 56
  %s2_7 = inttoptr i64 %s1_7 to ptr addrspace(1)
  %s1_8 = add i64 %basePtr, 64
  %s2_8 = inttoptr i64 %s1_8 to ptr addrspace(1)
  %s1_9 = add i64 %basePtr, 72
  %s2_9 = inttoptr i64 %s1_9 to ptr addrspace(1)
  %s1_10 = add i64 %basePtr, 80
  %s2_10 = inttoptr i64 %s1_10 to ptr addrspace(1)
  %s1_11 = add i64 %basePtr, 88
  %s2_11 = inttoptr i64 %s1_11 to ptr addrspace(1)
  %s1_12 = add i64 %basePtr, 96
  %s2_12 = inttoptr i64 %s1_12 to ptr addrspace(1)
  %s1_13 = add i64 %basePtr, 104
  %s2_13 = inttoptr i64 %s1_13 to ptr addrspace(1)
  %s1_14 = add i64 %basePtr, 112
  %s2_14 = inttoptr i64 %s1_14 to ptr addrspace(1)
  %s1_15 = add i64 %basePtr, 120
  %s2_15 = inttoptr i64 %s1_15 to ptr addrspace(1)
  %s1_16 = add i64 %basePtr, 128
  %s2_16 = inttoptr i64 %s1_16 to ptr addrspace(1)
  %s1_17 = add i64 %basePtr, 136
  %s2_17 = inttoptr i64 %s1_17 to ptr addrspace(1)
  %s1_18 = add i64 %basePtr, 144
  %s2_18 = inttoptr i64 %s1_18 to ptr addrspace(1)
  %s1_19 = add i64 %basePtr, 152
  %s2_19 = inttoptr i64 %s1_19 to ptr addrspace(1)
  %s1_20 = add i64 %basePtr, 160
  %s2_20 = inttoptr i64 %s1_20 to ptr addrspace(1)
  %s1_21 = add i64 %basePtr, 168
  %s2_21 = inttoptr i64 %s1_21 to ptr addrspace(1)
  %s1_22 = add i64 %basePtr, 176
  %s2_22 = inttoptr i64 %s1_22 to ptr addrspace(1)
  %s1_23 = add i64 %basePtr, 184
  %s2_23 = inttoptr i64 %s1_23 to ptr addrspace(1)
  %s1_24 = add i64 %basePtr, 192
  %s2_24 = inttoptr i64 %s1_24 to ptr addrspace(1)
  %l1_1 = add i64 %basePtr, 100032
  %l2_1 = inttoptr i64 %l1_1 to ptr addrspace(1)
  %r_1 = load <4 x double>, ptr addrspace(1) %l2_1, align 8
  %l1_2 = add i64 %basePtr, 100064
  %l2_2 = inttoptr i64 %l1_2 to ptr addrspace(1)
  %r_2 = load <4 x double>, ptr addrspace(1) %l2_2, align 8
  %l1_3 = add i64 %basePtr, 100096
  %l2_3 = inttoptr i64 %l1_3 to ptr addrspace(1)
  %r_3 = load <4 x double>, ptr addrspace(1) %l2_3, align 8
  %l1_4 = add i64 %basePtr, 100128
  %l2_4 = inttoptr i64 %l1_4 to ptr addrspace(1)
  %r_4 = load <4 x double>, ptr addrspace(1) %l2_4, align 8
  %l1_5 = add i64 %basePtr, 100160
  %l2_5 = inttoptr i64 %l1_5 to ptr addrspace(1)
  %r_5 = load <4 x double>, ptr addrspace(1) %l2_5, align 8
  %l1_6 = add i64 %basePtr, 100192
  %l2_6 = inttoptr i64 %l1_6 to ptr addrspace(1)
  %r_6 = load <4 x double>, ptr addrspace(1) %l2_6, align 8
  %l1_7 = add i64 %basePtr, 100224
  %l2_7 = inttoptr i64 %l1_7 to ptr addrspace(1)
  %r_7 = load <4 x double>, ptr addrspace(1) %l2_7, align 8
  %l1_8 = add i64 %basePtr, 100256
  %l2_8 = inttoptr i64 %l1_8 to ptr addrspace(1)
  %r_8 = load <4 x double>, ptr addrspace(1) %l2_8, align 8
  %l1_9 = add i64 %basePtr, 100288
  %l2_9 = inttoptr i64 %l1_9 to ptr addrspace(1)
  %r_9 = load <4 x double>, ptr addrspace(1) %l2_9, align 8
  %l1_10 = add i64 %basePtr, 100320
  %l2_10 = inttoptr i64 %l1_10 to ptr addrspace(1)
  %r_10 = load <4 x double>, ptr addrspace(1) %l2_10, align 8
  %l1_11 = add i64 %basePtr, 100352
  %l2_11 = inttoptr i64 %l1_11 to ptr addrspace(1)
  %r_11 = load <4 x double>, ptr addrspace(1) %l2_11, align 8
  %l1_12 = add i64 %basePtr, 100384
  %l2_12 = inttoptr i64 %l1_12 to ptr addrspace(1)
  %r_12 = load <4 x double>, ptr addrspace(1) %l2_12, align 8
  %acc1 = fadd <4 x double> %r_1, %r_2
  %acc2 = fadd <4 x double> %acc1, %r_3
  %acc3 = fadd <4 x double> %acc2, %r_4
  %acc4 = fadd <4 x double> %acc3, %r_5
  %acc5 = fadd <4 x double> %acc4, %r_6
  %acc6 = fadd <4 x double> %acc5, %r_7
  %acc7 = fadd <4 x double> %acc6, %r_8
  %acc8 = fadd <4 x double> %acc7, %r_9
  %acc9 = fadd <4 x double> %acc8, %r_10
  %acc10 = fadd <4 x double> %acc9, %r_11
  %acc11 = fadd <4 x double> %acc10, %r_12
  store <4 x double> %acc11, ptr addrspace(1) %s2_1, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_2, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_3, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_4, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_5, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_6, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_7, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_8, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_9, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_10, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_11, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_12, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_13, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_14, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_15, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_16, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_17, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_18, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_19, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_20, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_21, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_22, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_23, align 8
  store <4 x double> %acc11, ptr addrspace(1) %s2_24, align 8
  ret void
}
