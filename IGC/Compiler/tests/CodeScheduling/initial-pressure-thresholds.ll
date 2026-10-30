;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey ForceOCLSIMDWidth=16 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling -disable-output %s 2>&1 | FileCheck %s --check-prefix=SKIP
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey ForceOCLSIMDWidth=16 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling -disable-output %s 2>&1 | FileCheck %s --check-prefix=SCHED
;
; Three adjustments of the register pressure checks depend on the initial
; pressure of the block. Each threshold is a delta below the number of GRFs,
; tuned for 256 GRF; fewer than 256 GRF use the threshold of 256 GRF:
;
;   HighInitialPressureThresholdDelta (76): above 180 GRF for 256 GRF, above
;   436 for 512, the high and critical checks get HighInitialPressureRPMargin
;   (20 GRF).
;   LargeLoadDensityMarginCriticalMaxInitialRPDelta (136): below 120 GRF for
;   256 GRF, below 376 for 512, the critical check gets LargeLoadDensityMargin
;   (one GRF per 2D block read here: 10 GRF).
;   LargeLoadHighRPAdjustmentsMinInitialRPDelta (96): from 160 GRF for 256 GRF,
;   from 416 for 512, the high check gets LargeLoadDensityMargin.
;
; The critical check decides whether the original order can spill. Each block
; of the first five peaks between the critical threshold with and without the
; tested margin, so the skip message shows whether the margin applies. The
; last two blocks are scheduled; they peak between the high threshold with and
; without the large-load margin, so their register pressure decisions show it.

; SKIP-LABEL: Function margin_grf512_initial_201
; SKIP: Initial CurrentPressure in registers: 201
; SKIP: Max original regpressure: 487
; SKIP-NEXT: Original schedule can not have spills, skipping scheduling

; SKIP-LABEL: Function margin_grf512_initial_457
; SKIP: Initial CurrentPressure in registers: 457
; SKIP: Max original regpressure: 487
; SKIP-NEXT: Max original regpressure is below threshold, skipping scheduling

; SKIP-LABEL: Function margin_grf256_initial_201
; SKIP: Initial CurrentPressure in registers: 201
; SKIP: Max original regpressure: 231
; SKIP-NEXT: Max original regpressure is below threshold, skipping scheduling

; SKIP-LABEL: Function density_grf512_initial_152
; SKIP: Initial CurrentPressure in registers: 152
; SKIP: Max original regpressure: 492
; SKIP-NEXT: Max original regpressure is below threshold, skipping scheduling

; SKIP-LABEL: Function density_grf256_initial_152
; SKIP: Initial CurrentPressure in registers: 152
; SKIP: Max original regpressure: 236
; SKIP-NEXT: Original schedule can not have spills, skipping scheduling

; SCHED-LABEL: Function large_load_high_grf512_initial_170
; SCHED: Initial CurrentPressure in registers: 170
; SCHED: Attempt 0 (greedy-mw): result=retained, complete=1, maxRP=476, MW={{[0-9]+}}, RP=0,

; SCHED-LABEL: Function large_load_high_grf256_initial_170
; SCHED: Initial CurrentPressure in registers: 170
; SCHED: Attempt 0 (greedy-mw): result=retained, complete=1, maxRP=216, MW={{[0-9]+}}, RP={{[1-9][0-9]*}},

define spir_kernel void @margin_grf512_initial_201(ptr addrspace(1) %src, ptr addrspace(1) %dst) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <192 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <192 x float>, ptr addrspace(1) %p, align 4
  br label %bb

bb:
  %local = load <280 x float>, ptr addrspace(1) %p, align 4
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  store <280 x float> %local, ptr addrspace(1) %dst, align 4
  store <192 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @margin_grf512_initial_457(ptr addrspace(1) %src, ptr addrspace(1) %dst) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <448 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <448 x float>, ptr addrspace(1) %p, align 4
  br label %bb

bb:
  %local = load <24 x float>, ptr addrspace(1) %p, align 4
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  store <24 x float> %local, ptr addrspace(1) %dst, align 4
  store <448 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @margin_grf256_initial_201(ptr addrspace(1) %src, ptr addrspace(1) %dst) #1 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <192 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <192 x float>, ptr addrspace(1) %p, align 4
  br label %bb

bb:
  %local = load <24 x float>, ptr addrspace(1) %p, align 4
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  store <24 x float> %local, ptr addrspace(1) %dst, align 4
  store <192 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @density_grf512_initial_152(ptr addrspace(1) %src, ptr addrspace(1) %dst) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <141 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <141 x float>, ptr addrspace(1) %p, align 4
  %base = ptrtoint ptr addrspace(1) %src to i64
  br label %bb

bb:
  %local = load <296 x float>, ptr addrspace(1) %p, align 4
  %ld0 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 16, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 32, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld3 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 48, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld4 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 64, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld5 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 80, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld6 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 96, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld7 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 112, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld8 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 128, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld9 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 144, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  store <8 x i16> %ld0, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld1, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld2, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld3, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld4, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld5, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld6, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld7, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld8, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld9, ptr addrspace(1) %dst, align 16
  store <296 x float> %local, ptr addrspace(1) %dst, align 4
  store <141 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @density_grf256_initial_152(ptr addrspace(1) %src, ptr addrspace(1) %dst) #1 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <141 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <141 x float>, ptr addrspace(1) %p, align 4
  %base = ptrtoint ptr addrspace(1) %src to i64
  br label %bb

bb:
  %local = load <40 x float>, ptr addrspace(1) %p, align 4
  %ld0 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 16, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 32, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld3 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 48, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld4 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 64, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld5 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 80, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld6 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 96, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld7 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 112, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld8 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 128, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld9 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 144, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  store <8 x i16> %ld0, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld1, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld2, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld3, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld4, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld5, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld6, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld7, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld8, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld9, ptr addrspace(1) %dst, align 16
  store <40 x float> %local, ptr addrspace(1) %dst, align 4
  store <141 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @large_load_high_grf512_initial_170(ptr addrspace(1) %src, ptr addrspace(1) %dst) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <159 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <159 x float>, ptr addrspace(1) %p, align 4
  %base = ptrtoint ptr addrspace(1) %src to i64
  br label %bb

bb:
  %local = load <262 x float>, ptr addrspace(1) %p, align 4
  %ld0 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 16, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 32, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld3 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 48, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld4 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 64, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld5 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 80, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld6 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 96, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld7 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 112, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld8 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 128, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld9 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 144, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  store <8 x i16> %ld0, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld1, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld2, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld3, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld4, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld5, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld6, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld7, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld8, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld9, ptr addrspace(1) %dst, align 16
  store <262 x float> %local, ptr addrspace(1) %dst, align 4
  store <159 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @large_load_high_grf256_initial_170(ptr addrspace(1) %src, ptr addrspace(1) %dst) #1 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <159 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <159 x float>, ptr addrspace(1) %p, align 4
  %base = ptrtoint ptr addrspace(1) %src to i64
  br label %bb

bb:
  %local = load <2 x float>, ptr addrspace(1) %p, align 4
  %ld0 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 16, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 32, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld3 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 48, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld4 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 64, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld5 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 80, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld6 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 96, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld7 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 112, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld8 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 128, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld9 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 144, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  store <8 x i16> %ld0, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld1, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld2, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld3, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld4, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld5, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld6, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld7, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld8, ptr addrspace(1) %dst, align 16
  store <8 x i16> %ld9, ptr addrspace(1) %dst, align 16
  store <2 x float> %local, ptr addrspace(1) %dst, align 4
  store <159 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { "num-grf-per-thread"="512" }
attributes #1 = { "num-grf-per-thread"="256" }
