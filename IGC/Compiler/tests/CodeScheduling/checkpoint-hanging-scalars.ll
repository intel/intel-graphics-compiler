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
; RUN:   --regkey ForceOCLSIMDWidth=16 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling -disable-output %s 2>&1 | FileCheck %s
;
; A checkpoint copies the register pressure tracker of its schedule. %v0 and %v1
; build a vector out of the scalars %s0 and %s1. The greedy MaxWeight attempt
; saves a checkpoint before %ld2, which is placed after %s0 dies and before %s1
; is computed. Both 2-element loads make the attempt exceed the GRF number, so
; the checkpoint attempt runs. It places %ld2 and the instructions after it in
; the same order and must track the same pressure: the vector releases its
; scalars when %s1 dies.
;
; CHECK-LABEL: Function hanging_scalars_checkpoint
; CHECK: Greedy MW attempt
; CHECK: ([[P:[0-9]+]], {{.*}} %p = getelementptr
; CHECK: Attempt 0 (greedy-mw): result=critical, complete=1, maxRP=[[MAXRP:[0-9]+]],
; CHECK: Checkpoint attempt 2
; CHECK-NEXT: MW*: {{.*}} %ld2 = call
; CHECK: ([[P]], {{.*}} %p = getelementptr
; CHECK: Attempt 2 (checkpoint): result=critical, complete=1, maxRP=[[MAXRP]],

define spir_kernel void @hanging_scalars_checkpoint(ptr addrspace(1) %A, ptr addrspace(1) %dst,
                                                    ptr addrspace(1) %big) #0 {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> %ld1, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  %e0 = extractelement <8 x float> %d1, i32 0
  %s0 = fadd float %e0, 1.0
  %v0 = insertelement <2 x float> undef, float %s0, i32 0
  %x = fptosi float %s0 to i32
  %ld2 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 %x, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 4)
  %b2 = bitcast <16 x i16> %ld2 to <8 x i32>
  %d2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %b2, i32 11, i32 11, i32 8, i32 8, i1 false)
  %e2 = extractelement <8 x float> %d2, i32 0
  %s1 = fadd float %e2, 2.0
  %v1 = insertelement <2 x float> %v0, float %s1, i32 1
  %off = fptosi float %s1 to i64
  %p = getelementptr <64 x float>, ptr addrspace(1) %big, i64 %off
  %w0 = load <64 x float>, ptr addrspace(1) %p, align 4
  %p1 = getelementptr <64 x float>, ptr addrspace(1) %p, i64 1
  %w1 = load <64 x float>, ptr addrspace(1) %p1, align 4
  store <64 x float> %w0, ptr addrspace(1) %p1, align 4
  store <64 x float> %w1, ptr addrspace(1) %p, align 4
  store <2 x float> %v1, ptr addrspace(1) %dst, align 8
  ret void
}

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { "num-grf-per-thread"="128" }
