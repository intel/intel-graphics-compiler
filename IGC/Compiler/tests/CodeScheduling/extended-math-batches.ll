;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Test: CodeScheduling schedules the computations of extended math operands in
; batches.
;
; The blocks compute exp2((s - m) * scale) for independent s. The instructions
; of one level (all fsub, all fmul, all exp2) have the same MaxWeight, so
; MaxWeight scheduling alone keeps the input order, level by level, and puts
; all exp2 back to back. With ExtendedMathBatchElements (default 16 at SIMD16),
; a batch takes computations until their exp2 results have 16 vector elements,
; and the next batch starts after the exp2 of the batch are scheduled.
;
; An <8 x float> result has 8 elements, so a batch has two computations; a
; <4 x float> result has 4, so a batch has four.
;
; 256 GRF keep the register pressure low, so that every instruction is chosen
; by MaxWeight.

; RUN: igc_opt --opaque-pointers -platformpvc --regkey ForceOCLSIMDWidth=16 --regkey TotalGRFNum=256 \
; RUN:         --regkey DisableCodeScheduling=0 --regkey CodeSchedulingForceMWOnly=1 \
; RUN:         --regkey EnableCodeSchedulingIfNoSpills=1 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:         --igc-code-scheduling --verify -S %s 2>&1 | FileCheck %s

; CHECK-LABEL: @two_per_batch(
; CHECK:       body:
; CHECK-NEXT:    [[D0:%.*]] = fsub fast <8 x float> %s0, %m
; CHECK-NEXT:    [[D1:%.*]] = fsub fast <8 x float> %s1, %m
; CHECK-NEXT:    [[X0:%.*]] = fmul fast <8 x float> [[D0]], %scale
; CHECK-NEXT:    [[X1:%.*]] = fmul fast <8 x float> [[D1]], %scale
; CHECK-NEXT:    [[P0:%.*]] = call <8 x float> @llvm.exp2.v8f32(<8 x float> [[X0]])
; CHECK-NEXT:    [[P1:%.*]] = call <8 x float> @llvm.exp2.v8f32(<8 x float> [[X1]])
; CHECK-NEXT:    [[D2:%.*]] = fsub fast <8 x float> %s2, %m
; CHECK-NEXT:    [[D3:%.*]] = fsub fast <8 x float> %s3, %m
; CHECK-NEXT:    [[X2:%.*]] = fmul fast <8 x float> [[D2]], %scale
; CHECK-NEXT:    [[X3:%.*]] = fmul fast <8 x float> [[D3]], %scale
; CHECK-NEXT:    [[P2:%.*]] = call <8 x float> @llvm.exp2.v8f32(<8 x float> [[X2]])
; CHECK-NEXT:    [[P3:%.*]] = call <8 x float> @llvm.exp2.v8f32(<8 x float> [[X3]])

define spir_kernel void @two_per_batch(ptr addrspace(1) %out, <8 x float> %s0, <8 x float> %s1, <8 x float> %s2,
                                       <8 x float> %s3, <8 x float> %m, <8 x float> %scale, <8 x i32> %w) {
entry:
  br label %body

body:
  %d0 = fsub fast <8 x float> %s0, %m
  %d1 = fsub fast <8 x float> %s1, %m
  %d2 = fsub fast <8 x float> %s2, %m
  %d3 = fsub fast <8 x float> %s3, %m
  %x0 = fmul fast <8 x float> %d0, %scale
  %x1 = fmul fast <8 x float> %d1, %scale
  %x2 = fmul fast <8 x float> %d2, %scale
  %x3 = fmul fast <8 x float> %d3, %scale
  %p0 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %x0)
  %p1 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %x1)
  %p2 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %x2)
  %p3 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %x3)
  %a = fadd fast <8 x float> %p0, %p1
  %b = fadd fast <8 x float> %p2, %p3
  %c = fadd fast <8 x float> %a, %b
  ; CodeScheduling schedules only blocks with DPAS.
  %h = fptrunc <8 x float> %c to <8 x half>
  %hi = bitcast <8 x half> %h to <8 x i16>
  %acc = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %hi, <8 x i32> %w, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %acc, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: @four_per_batch(
; CHECK:       body:
; CHECK-NEXT:    [[D0:%.*]] = fsub fast <4 x float> %s0, %m
; CHECK-NEXT:    [[D1:%.*]] = fsub fast <4 x float> %s1, %m
; CHECK-NEXT:    [[D2:%.*]] = fsub fast <4 x float> %s2, %m
; CHECK-NEXT:    [[D3:%.*]] = fsub fast <4 x float> %s3, %m
; CHECK-NEXT:    [[X0:%.*]] = fmul fast <4 x float> [[D0]], %scale
; CHECK-NEXT:    [[X1:%.*]] = fmul fast <4 x float> [[D1]], %scale
; CHECK-NEXT:    [[X2:%.*]] = fmul fast <4 x float> [[D2]], %scale
; CHECK-NEXT:    [[X3:%.*]] = fmul fast <4 x float> [[D3]], %scale
; CHECK-NEXT:    [[P0:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X0]])
; CHECK-NEXT:    [[P1:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X1]])
; CHECK-NEXT:    [[P2:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X2]])
; CHECK-NEXT:    [[P3:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X3]])
; CHECK-NEXT:    [[D4:%.*]] = fsub fast <4 x float> %s4, %m
; CHECK-NEXT:    [[D5:%.*]] = fsub fast <4 x float> %s5, %m
; CHECK-NEXT:    [[D6:%.*]] = fsub fast <4 x float> %s6, %m
; CHECK-NEXT:    [[D7:%.*]] = fsub fast <4 x float> %s7, %m
; CHECK-NEXT:    [[X4:%.*]] = fmul fast <4 x float> [[D4]], %scale
; CHECK-NEXT:    [[X5:%.*]] = fmul fast <4 x float> [[D5]], %scale
; CHECK-NEXT:    [[X6:%.*]] = fmul fast <4 x float> [[D6]], %scale
; CHECK-NEXT:    [[X7:%.*]] = fmul fast <4 x float> [[D7]], %scale
; CHECK-NEXT:    [[P4:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X4]])
; CHECK-NEXT:    [[P5:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X5]])
; CHECK-NEXT:    [[P6:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X6]])
; CHECK-NEXT:    [[P7:%.*]] = call <4 x float> @llvm.exp2.v4f32(<4 x float> [[X7]])

define spir_kernel void @four_per_batch(ptr addrspace(1) %out, <4 x float> %s0, <4 x float> %s1, <4 x float> %s2,
                                        <4 x float> %s3, <4 x float> %s4, <4 x float> %s5, <4 x float> %s6,
                                        <4 x float> %s7, <4 x float> %m, <4 x float> %scale, <8 x i32> %w) {
entry:
  br label %body

body:
  %d0 = fsub fast <4 x float> %s0, %m
  %d1 = fsub fast <4 x float> %s1, %m
  %d2 = fsub fast <4 x float> %s2, %m
  %d3 = fsub fast <4 x float> %s3, %m
  %d4 = fsub fast <4 x float> %s4, %m
  %d5 = fsub fast <4 x float> %s5, %m
  %d6 = fsub fast <4 x float> %s6, %m
  %d7 = fsub fast <4 x float> %s7, %m
  %x0 = fmul fast <4 x float> %d0, %scale
  %x1 = fmul fast <4 x float> %d1, %scale
  %x2 = fmul fast <4 x float> %d2, %scale
  %x3 = fmul fast <4 x float> %d3, %scale
  %x4 = fmul fast <4 x float> %d4, %scale
  %x5 = fmul fast <4 x float> %d5, %scale
  %x6 = fmul fast <4 x float> %d6, %scale
  %x7 = fmul fast <4 x float> %d7, %scale
  %p0 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x0)
  %p1 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x1)
  %p2 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x2)
  %p3 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x3)
  %p4 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x4)
  %p5 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x5)
  %p6 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x6)
  %p7 = call <4 x float> @llvm.exp2.v4f32(<4 x float> %x7)
  %a0 = fadd fast <4 x float> %p0, %p1
  %a1 = fadd fast <4 x float> %p2, %p3
  %a2 = fadd fast <4 x float> %p4, %p5
  %a3 = fadd fast <4 x float> %p6, %p7
  %b0 = fadd fast <4 x float> %a0, %a1
  %b1 = fadd fast <4 x float> %a2, %a3
  %c = fadd fast <4 x float> %b0, %b1
  ; CodeScheduling schedules only blocks with DPAS.
  %c8 = shufflevector <4 x float> %c, <4 x float> %c, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %h = fptrunc <8 x float> %c8 to <8 x half>
  %hi = bitcast <8 x half> %h to <8 x i16>
  %acc = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %hi, <8 x i32> %w, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %acc, ptr addrspace(1) %out
  ret void
}

declare <8 x float> @llvm.exp2.v8f32(<8 x float>)
declare <4 x float> @llvm.exp2.v4f32(<4 x float>)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
