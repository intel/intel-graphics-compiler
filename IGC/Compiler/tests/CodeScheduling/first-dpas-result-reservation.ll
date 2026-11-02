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
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableCodeSchedulingIfNoSpills=1 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey ForceOCLSIMDWidth=16 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling -disable-output %s 2>&1 | FileCheck %s
;
; While no DPAS chain of the block is open, the register pressure checks of a
; 2D block read include the result of the first DPAS
; (ReserveFirstDPASResultForLoads). The first DPAS of a chain has a constant
; accumulator, so it writes its result to new registers and increases the
; register pressure. The next DPAS of the chain replaces its accumulator and
; does not.
;
; The block starts at 113 GRF and has 16 2D block reads, so the critical check
; allows 256 - 15 - 16 = 225 GRF. Both DPAS chains start with a zero
; accumulator. The MaxWeight phase stops the reads at 217 GRF, because the 14th
; read and the first DPAS result would reach 233. The two chain heads then
; peak at 225, and the attempt is retained. Without the reservation, the 14th
; read is issued, the first chain head reaches 233, and the attempt is
; critical.

; CHECK-LABEL: Function reads_before_the_first_chain
; CHECK: Initial CurrentPressure in registers: 113
; CHECK: Greedy MW attempt
; CHECK: (209, 512 {{.*}} %r12 = call
; CHECK-NEXT: (217, 512 {{.*}} %x0.0 = call
; CHECK-NEXT: (225, 0 {{.*}} %x1.0 = call
; CHECK: Attempt 0 (greedy-mw): result=retained, complete=1, maxRP=225,

define spir_kernel void @reads_before_the_first_chain(ptr addrspace(1) %src, <8 x i16> %a0, <8 x i16> %a1, ptr addrspace(1) %dst) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %idx = zext i16 %lane to i64
  %p = getelementptr <96 x float>, ptr addrspace(1) %src, i64 %idx
  %live.in = load <96 x float>, ptr addrspace(1) %p, align 4
  %base = ptrtoint ptr addrspace(1) %src to i64
  br label %bb

bb:
  %r0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r2 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 32, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r3 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 48, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r4 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 64, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r5 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 80, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r6 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 96, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r7 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 112, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r8 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 128, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r9 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 144, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r10 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 160, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r11 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 176, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r12 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 192, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r13 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 208, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r14 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 224, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r15 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 240, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %x0.0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a0, <8 x i32> %r0, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.0, <8 x i16> %a0, <8 x i32> %r1, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.1, <8 x i16> %a0, <8 x i32> %r2, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.2, <8 x i16> %a0, <8 x i32> %r3, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.3, <8 x i16> %a0, <8 x i32> %r4, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.5 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.4, <8 x i16> %a0, <8 x i32> %r5, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.6 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.5, <8 x i16> %a0, <8 x i32> %r6, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.6, <8 x i16> %a0, <8 x i32> %r7, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.8 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.7, <8 x i16> %a0, <8 x i32> %r8, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.9 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.8, <8 x i16> %a0, <8 x i32> %r9, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.10 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.9, <8 x i16> %a0, <8 x i32> %r10, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.11 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.10, <8 x i16> %a0, <8 x i32> %r11, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.12 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.11, <8 x i16> %a0, <8 x i32> %r12, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.13 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.12, <8 x i16> %a0, <8 x i32> %r13, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.14 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.13, <8 x i16> %a0, <8 x i32> %r14, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x0.15 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x0.14, <8 x i16> %a0, <8 x i32> %r15, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a1, <8 x i32> %r0, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.0, <8 x i16> %a1, <8 x i32> %r1, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.1, <8 x i16> %a1, <8 x i32> %r2, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.2, <8 x i16> %a1, <8 x i32> %r3, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.3, <8 x i16> %a1, <8 x i32> %r4, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.5 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.4, <8 x i16> %a1, <8 x i32> %r5, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.6 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.5, <8 x i16> %a1, <8 x i32> %r6, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.6, <8 x i16> %a1, <8 x i32> %r7, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.8 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.7, <8 x i16> %a1, <8 x i32> %r8, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.9 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.8, <8 x i16> %a1, <8 x i32> %r9, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.10 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.9, <8 x i16> %a1, <8 x i32> %r10, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.11 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.10, <8 x i16> %a1, <8 x i32> %r11, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.12 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.11, <8 x i16> %a1, <8 x i32> %r12, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.13 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.12, <8 x i16> %a1, <8 x i32> %r13, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.14 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.13, <8 x i16> %a1, <8 x i32> %r14, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x1.15 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1.14, <8 x i16> %a1, <8 x i32> %r15, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %x0.15, ptr addrspace(1) %dst, align 32
  store <8 x float> %x1.15, ptr addrspace(1) %dst, align 32
  store <96 x float> %live.in, ptr addrspace(1) %dst, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { "num-grf-per-thread"="256" }
