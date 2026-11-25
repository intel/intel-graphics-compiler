;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
; UNSUPPORTED: system-windows
; On BMG the first-compile remat estimates pressure at SIMD32. TotalGRFNum sets the register file size, which is also
; the budget for the drop to SIMD16. CEncoder sets the thread count of an annotation after it, and vISA takes the mode
; with that many threads. The 24 store addresses live from entry to exit keep SIMD32 over the budget after the remat.
; With a register file size of 128, rematerializing the addresses brings SIMD16 from 144 GRFs to 98:
; @plain: SIMD16 compiles with 128 GRFs, so the remat that brings it within them is kept.
; @annotated: 4 threads per EU give 256 GRFs. SIMD16 fits without the remat, so it is rolled back.
; With a register file size of 256:
; @unmatched: no mode has 10 threads per EU, so vISA keeps its default of 128 GRFs. SIMD16 goes from 272 to 226,
; within 256 but over 128, so the remat is rolled back.
; @annotated_md and @unmatched_md have the thread counts of @annotated and @unmatched only in the kernel metadata,
; which CEncoder reads after the GRF kernel options, and give the same results.
; @auto_md asks for automatic selection in the kernel metadata, which vISA turns off with a register file size, so
; SIMD16 compiles with 128 GRFs and the remat is kept.

; RUN: igc_opt -platformbmg --opaque-pointers %s -S -o - -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 --regkey=TotalGRFNum=128 -remat-first-compile --dce | FileCheck %s --check-prefix=FIRST
; RUN: igc_opt -platformbmg --opaque-pointers %s -S -o /dev/null -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 --regkey=TotalGRFNum=128 -remat-first-compile --regkey=RematLog=1 --regkey=RematLogToErr=1 2>&1 | FileCheck %s --check-prefix=LOG
; RUN: igc_opt -platformbmg --opaque-pointers %s -S -o - -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 --regkey=TotalGRFNum=256 -remat-first-compile --dce | FileCheck %s --check-prefix=FIRST256
; RUN: igc_opt -platformbmg --opaque-pointers %s -S -o /dev/null -igc-clone-address-arithmetic -remat-widest-simd --regkey=RematChainLimit=3 --regkey=RematFlowThreshold=1 --regkey=AllowSIMD16DropForXE2Plus=1 --regkey=TotalGRFNum=256 -remat-first-compile --regkey=RematLog=1 --regkey=RematLogToErr=1 2>&1 | FileCheck %s --check-prefix=LOG256

; FIRST-LABEL: define spir_kernel void @plain(
; FIRST: store <4 x double> {{.*}}, ptr addrspace(1) %cloned_{{.*}}
; FIRST-LABEL: define spir_kernel void @annotated(
; FIRST-NOT: !remat
; FIRST: store <4 x double> %acc11, ptr addrspace(1) %s2_1, align 8
; FIRST-NOT: !remat
; FIRST: ret void
; FIRST-LABEL: define spir_kernel void @annotated_md(
; FIRST-NOT: !remat
; FIRST: store <4 x double> %acc11, ptr addrspace(1) %s2_1, align 8
; FIRST-NOT: !remat
; FIRST: ret void
; FIRST-LABEL: define spir_kernel void @auto_md(
; FIRST: store <4 x double> {{.*}}, ptr addrspace(1) %cloned_{{.*}}
; FIRST-LABEL: define spir_kernel void @unmatched(

; LOG: Pressure budget: 128 MaxPressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG-NEXT: SIMD16 GRF number: 128 SIMD16 pressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG-NOT: rolling back
; LOG: Pressure budget: 128 MaxPressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG-NEXT: Budget not reached, rolling back
; LOG: Pressure budget: 128 MaxPressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG-NEXT: Budget not reached, rolling back
; LOG: Pressure budget: 128 MaxPressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG-NEXT: SIMD16 GRF number: 128 SIMD16 pressure: {{[0-9]+}} -> {{[0-9]+}}

; FIRST256-LABEL: define spir_kernel void @unmatched(
; FIRST256-NOT: !remat
; FIRST256: store <4 x double> %acc27, ptr addrspace(1) %s2_1, align 8
; FIRST256-NOT: !remat
; FIRST256: ret void
; FIRST256-LABEL: define spir_kernel void @unmatched_md(
; FIRST256-NOT: !remat
; FIRST256: store <4 x double> %acc27, ptr addrspace(1) %s2_1, align 8
; FIRST256-NOT: !remat
; FIRST256: ret void

; LOG256: SIMD16 GRF number: 128 SIMD16 pressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG256-NEXT: Budget not reached, rolling back
; LOG256: SIMD16 GRF number: 128 SIMD16 pressure: {{[0-9]+}} -> {{[0-9]+}}
; LOG256-NEXT: Budget not reached, rolling back
define spir_kernel void @plain(ptr addrspace(1) %base, i64 %offset, i64 %I) {
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

define spir_kernel void @annotated(ptr addrspace(1) %base, i64 %offset, i64 %I) #0 {
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

define spir_kernel void @annotated_md(ptr addrspace(1) %base, i64 %offset, i64 %I) {
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

define spir_kernel void @auto_md(ptr addrspace(1) %base, i64 %offset, i64 %I) {
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

define spir_kernel void @unmatched(ptr addrspace(1) %base, i64 %offset, i64 %I) #1 {
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
  %l1_13 = add i64 %basePtr, 100416
  %l2_13 = inttoptr i64 %l1_13 to ptr addrspace(1)
  %r_13 = load <4 x double>, ptr addrspace(1) %l2_13, align 8
  %l1_14 = add i64 %basePtr, 100448
  %l2_14 = inttoptr i64 %l1_14 to ptr addrspace(1)
  %r_14 = load <4 x double>, ptr addrspace(1) %l2_14, align 8
  %l1_15 = add i64 %basePtr, 100480
  %l2_15 = inttoptr i64 %l1_15 to ptr addrspace(1)
  %r_15 = load <4 x double>, ptr addrspace(1) %l2_15, align 8
  %l1_16 = add i64 %basePtr, 100512
  %l2_16 = inttoptr i64 %l1_16 to ptr addrspace(1)
  %r_16 = load <4 x double>, ptr addrspace(1) %l2_16, align 8
  %l1_17 = add i64 %basePtr, 100544
  %l2_17 = inttoptr i64 %l1_17 to ptr addrspace(1)
  %r_17 = load <4 x double>, ptr addrspace(1) %l2_17, align 8
  %l1_18 = add i64 %basePtr, 100576
  %l2_18 = inttoptr i64 %l1_18 to ptr addrspace(1)
  %r_18 = load <4 x double>, ptr addrspace(1) %l2_18, align 8
  %l1_19 = add i64 %basePtr, 100608
  %l2_19 = inttoptr i64 %l1_19 to ptr addrspace(1)
  %r_19 = load <4 x double>, ptr addrspace(1) %l2_19, align 8
  %l1_20 = add i64 %basePtr, 100640
  %l2_20 = inttoptr i64 %l1_20 to ptr addrspace(1)
  %r_20 = load <4 x double>, ptr addrspace(1) %l2_20, align 8
  %l1_21 = add i64 %basePtr, 100672
  %l2_21 = inttoptr i64 %l1_21 to ptr addrspace(1)
  %r_21 = load <4 x double>, ptr addrspace(1) %l2_21, align 8
  %l1_22 = add i64 %basePtr, 100704
  %l2_22 = inttoptr i64 %l1_22 to ptr addrspace(1)
  %r_22 = load <4 x double>, ptr addrspace(1) %l2_22, align 8
  %l1_23 = add i64 %basePtr, 100736
  %l2_23 = inttoptr i64 %l1_23 to ptr addrspace(1)
  %r_23 = load <4 x double>, ptr addrspace(1) %l2_23, align 8
  %l1_24 = add i64 %basePtr, 100768
  %l2_24 = inttoptr i64 %l1_24 to ptr addrspace(1)
  %r_24 = load <4 x double>, ptr addrspace(1) %l2_24, align 8
  %l1_25 = add i64 %basePtr, 100800
  %l2_25 = inttoptr i64 %l1_25 to ptr addrspace(1)
  %r_25 = load <4 x double>, ptr addrspace(1) %l2_25, align 8
  %l1_26 = add i64 %basePtr, 100832
  %l2_26 = inttoptr i64 %l1_26 to ptr addrspace(1)
  %r_26 = load <4 x double>, ptr addrspace(1) %l2_26, align 8
  %l1_27 = add i64 %basePtr, 100864
  %l2_27 = inttoptr i64 %l1_27 to ptr addrspace(1)
  %r_27 = load <4 x double>, ptr addrspace(1) %l2_27, align 8
  %l1_28 = add i64 %basePtr, 100896
  %l2_28 = inttoptr i64 %l1_28 to ptr addrspace(1)
  %r_28 = load <4 x double>, ptr addrspace(1) %l2_28, align 8
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
  %acc12 = fadd <4 x double> %acc11, %r_13
  %acc13 = fadd <4 x double> %acc12, %r_14
  %acc14 = fadd <4 x double> %acc13, %r_15
  %acc15 = fadd <4 x double> %acc14, %r_16
  %acc16 = fadd <4 x double> %acc15, %r_17
  %acc17 = fadd <4 x double> %acc16, %r_18
  %acc18 = fadd <4 x double> %acc17, %r_19
  %acc19 = fadd <4 x double> %acc18, %r_20
  %acc20 = fadd <4 x double> %acc19, %r_21
  %acc21 = fadd <4 x double> %acc20, %r_22
  %acc22 = fadd <4 x double> %acc21, %r_23
  %acc23 = fadd <4 x double> %acc22, %r_24
  %acc24 = fadd <4 x double> %acc23, %r_25
  %acc25 = fadd <4 x double> %acc24, %r_26
  %acc26 = fadd <4 x double> %acc25, %r_27
  %acc27 = fadd <4 x double> %acc26, %r_28
  store <4 x double> %acc27, ptr addrspace(1) %s2_1, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_2, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_3, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_4, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_5, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_6, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_7, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_8, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_9, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_10, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_11, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_12, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_13, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_14, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_15, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_16, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_17, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_18, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_19, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_20, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_21, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_22, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_23, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_24, align 8
  ret void
}

define spir_kernel void @unmatched_md(ptr addrspace(1) %base, i64 %offset, i64 %I) {
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
  %l1_13 = add i64 %basePtr, 100416
  %l2_13 = inttoptr i64 %l1_13 to ptr addrspace(1)
  %r_13 = load <4 x double>, ptr addrspace(1) %l2_13, align 8
  %l1_14 = add i64 %basePtr, 100448
  %l2_14 = inttoptr i64 %l1_14 to ptr addrspace(1)
  %r_14 = load <4 x double>, ptr addrspace(1) %l2_14, align 8
  %l1_15 = add i64 %basePtr, 100480
  %l2_15 = inttoptr i64 %l1_15 to ptr addrspace(1)
  %r_15 = load <4 x double>, ptr addrspace(1) %l2_15, align 8
  %l1_16 = add i64 %basePtr, 100512
  %l2_16 = inttoptr i64 %l1_16 to ptr addrspace(1)
  %r_16 = load <4 x double>, ptr addrspace(1) %l2_16, align 8
  %l1_17 = add i64 %basePtr, 100544
  %l2_17 = inttoptr i64 %l1_17 to ptr addrspace(1)
  %r_17 = load <4 x double>, ptr addrspace(1) %l2_17, align 8
  %l1_18 = add i64 %basePtr, 100576
  %l2_18 = inttoptr i64 %l1_18 to ptr addrspace(1)
  %r_18 = load <4 x double>, ptr addrspace(1) %l2_18, align 8
  %l1_19 = add i64 %basePtr, 100608
  %l2_19 = inttoptr i64 %l1_19 to ptr addrspace(1)
  %r_19 = load <4 x double>, ptr addrspace(1) %l2_19, align 8
  %l1_20 = add i64 %basePtr, 100640
  %l2_20 = inttoptr i64 %l1_20 to ptr addrspace(1)
  %r_20 = load <4 x double>, ptr addrspace(1) %l2_20, align 8
  %l1_21 = add i64 %basePtr, 100672
  %l2_21 = inttoptr i64 %l1_21 to ptr addrspace(1)
  %r_21 = load <4 x double>, ptr addrspace(1) %l2_21, align 8
  %l1_22 = add i64 %basePtr, 100704
  %l2_22 = inttoptr i64 %l1_22 to ptr addrspace(1)
  %r_22 = load <4 x double>, ptr addrspace(1) %l2_22, align 8
  %l1_23 = add i64 %basePtr, 100736
  %l2_23 = inttoptr i64 %l1_23 to ptr addrspace(1)
  %r_23 = load <4 x double>, ptr addrspace(1) %l2_23, align 8
  %l1_24 = add i64 %basePtr, 100768
  %l2_24 = inttoptr i64 %l1_24 to ptr addrspace(1)
  %r_24 = load <4 x double>, ptr addrspace(1) %l2_24, align 8
  %l1_25 = add i64 %basePtr, 100800
  %l2_25 = inttoptr i64 %l1_25 to ptr addrspace(1)
  %r_25 = load <4 x double>, ptr addrspace(1) %l2_25, align 8
  %l1_26 = add i64 %basePtr, 100832
  %l2_26 = inttoptr i64 %l1_26 to ptr addrspace(1)
  %r_26 = load <4 x double>, ptr addrspace(1) %l2_26, align 8
  %l1_27 = add i64 %basePtr, 100864
  %l2_27 = inttoptr i64 %l1_27 to ptr addrspace(1)
  %r_27 = load <4 x double>, ptr addrspace(1) %l2_27, align 8
  %l1_28 = add i64 %basePtr, 100896
  %l2_28 = inttoptr i64 %l1_28 to ptr addrspace(1)
  %r_28 = load <4 x double>, ptr addrspace(1) %l2_28, align 8
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
  %acc12 = fadd <4 x double> %acc11, %r_13
  %acc13 = fadd <4 x double> %acc12, %r_14
  %acc14 = fadd <4 x double> %acc13, %r_15
  %acc15 = fadd <4 x double> %acc14, %r_16
  %acc16 = fadd <4 x double> %acc15, %r_17
  %acc17 = fadd <4 x double> %acc16, %r_18
  %acc18 = fadd <4 x double> %acc17, %r_19
  %acc19 = fadd <4 x double> %acc18, %r_20
  %acc20 = fadd <4 x double> %acc19, %r_21
  %acc21 = fadd <4 x double> %acc20, %r_22
  %acc22 = fadd <4 x double> %acc21, %r_23
  %acc23 = fadd <4 x double> %acc22, %r_24
  %acc24 = fadd <4 x double> %acc23, %r_25
  %acc25 = fadd <4 x double> %acc24, %r_26
  %acc26 = fadd <4 x double> %acc25, %r_27
  %acc27 = fadd <4 x double> %acc26, %r_28
  store <4 x double> %acc27, ptr addrspace(1) %s2_1, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_2, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_3, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_4, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_5, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_6, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_7, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_8, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_9, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_10, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_11, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_12, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_13, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_14, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_15, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_16, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_17, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_18, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_19, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_20, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_21, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_22, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_23, align 8
  store <4 x double> %acc27, ptr addrspace(1) %s2_24, align 8
  ret void
}

attributes #0 = { "num-thread-per-eu"="4" }
attributes #1 = { "num-thread-per-eu"="10" }

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !4, !5, !10, !11}
!2 = !{!"FuncMDMap[0]", ptr @annotated_md}
!3 = !{!"FuncMDValue[0]", !6}
!4 = !{!"FuncMDMap[1]", ptr @unmatched_md}
!5 = !{!"FuncMDValue[1]", !8}
!6 = !{!"UserAnnotations", !7}
!7 = !{!"UserAnnotationsVec[0]", !"num-thread-per-eu 4"}
!8 = !{!"UserAnnotations", !9}
!9 = !{!"UserAnnotationsVec[0]", !"num-thread-per-eu 10"}
!10 = !{!"FuncMDMap[2]", ptr @auto_md}
!11 = !{!"FuncMDValue[2]", !12}
!12 = !{!"UserAnnotations", !13}
!13 = !{!"UserAnnotationsVec[0]", !"num-thread-per-eu auto"}
