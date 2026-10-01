;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; A mask is an i1 value whose users are only select conditions, directly or
; through other masks. Each mask holds a flag register from its definition to
; its last use that does not fold it (MatchBoolOp folds the first compare
; operand of an and/or of i1 when the other operand has one use). Check that
; CodeScheduling
; - computes a mask only once one of its selects has its value operands
;   scheduled (DeferMasksUntilSelectReady), and
; - keeps the number of open masks within the flag registers of the dispatch
;   width minus two (LimitOpenMasks): 6 for SIMD16 with 8 flag registers (PVC),
;   2 for SIMD32 with 8 and for SIMD16 with 4 flag registers (DG2).

; RUN: igc_opt --opaque-pointers -platformpvc --regkey ForceOCLSIMDWidth=16 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 --igc-code-scheduling --verify -S %s \
; RUN:   | FileCheck %s --check-prefixes=CHECK,LIMIT6
; RUN: igc_opt --opaque-pointers -platformpvc --regkey ForceOCLSIMDWidth=32 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 --igc-code-scheduling --verify -S %s \
; RUN:   | FileCheck %s --check-prefixes=CHECK,LIMIT2
; RUN: igc_opt --opaque-pointers -platformdg2 --regkey ForceOCLSIMDWidth=16 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 --igc-code-scheduling --verify -S %s \
; RUN:   | FileCheck %s --check-prefixes=CHECK,LIMIT2
; RUN: igc_opt --opaque-pointers -platformpvc --regkey ForceOCLSIMDWidth=16 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:         --igc-code-scheduling -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=DUMP16
; RUN: igc_opt --opaque-pointers -platformdg2 --regkey ForceOCLSIMDWidth=16 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:         --igc-code-scheduling -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=DUMP4FLAGS

; DUMP16-LABEL: Function open_mask_limit
; DUMP16:       Masks: 24, open mask limit: 6
; DUMP16-LABEL: Function shared_masks
; DUMP16:       Masks: 28, open mask limit: 6
; DUMP4FLAGS-LABEL: Function open_mask_limit
; DUMP4FLAGS:       Masks: 24, open mask limit: 2

; Eight selects choose between lanes of one DPAS result. In each mask
; %m = and %c, %e, MatchBoolOp folds %c, so %e holds a flag register until %m.
; All %e have the same MaxWeight, so without the limit MaxWeight computes all
; eight of them before the first %m. With the limit, the open masks close
; (%m0, %s0) before the next one opens.

define spir_kernel void @open_mask_limit(ptr addrspace(1) %out, <8 x i16> %a, <8 x i32> %b, i32 %q, <8 x i32> %k) {
; CHECK-LABEL: @open_mask_limit(
; LIMIT6:      %e5 = icmp ne i32 %k5, 0
; LIMIT6-NOT:  %e6 = icmp
; LIMIT6:      %s0 = select i1 %m0
; LIMIT6:      %e6 = icmp ne i32 %k6, 0
; LIMIT2:      %e1 = icmp ne i32 %k1, 0
; LIMIT2-NOT:  %e2 = icmp
; LIMIT2:      %s0 = select i1 %m0
; LIMIT2:      %e2 = icmp ne i32 %k2, 0
; CHECK:       ret void
entry:
  br label %body

body:
  %d = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %k0 = extractelement <8 x i32> %k, i32 0
  %k1 = extractelement <8 x i32> %k, i32 1
  %k2 = extractelement <8 x i32> %k, i32 2
  %k3 = extractelement <8 x i32> %k, i32 3
  %k4 = extractelement <8 x i32> %k, i32 4
  %k5 = extractelement <8 x i32> %k, i32 5
  %k6 = extractelement <8 x i32> %k, i32 6
  %k7 = extractelement <8 x i32> %k, i32 7
  %x0 = extractelement <8 x float> %d, i32 0
  %c0 = icmp sle i32 %q, %k0
  %e0 = icmp ne i32 %k0, 0
  %m0 = and i1 %c0, %e0
  %s0 = select i1 %m0, float %x0, float 0xC1D0000000000000
  %x1 = extractelement <8 x float> %d, i32 1
  %c1 = icmp sle i32 %q, %k1
  %e1 = icmp ne i32 %k1, 0
  %m1 = and i1 %c1, %e1
  %s1 = select i1 %m1, float %x1, float 0xC1D0000000000000
  %x2 = extractelement <8 x float> %d, i32 2
  %c2 = icmp sle i32 %q, %k2
  %e2 = icmp ne i32 %k2, 0
  %m2 = and i1 %c2, %e2
  %s2 = select i1 %m2, float %x2, float 0xC1D0000000000000
  %x3 = extractelement <8 x float> %d, i32 3
  %c3 = icmp sle i32 %q, %k3
  %e3 = icmp ne i32 %k3, 0
  %m3 = and i1 %c3, %e3
  %s3 = select i1 %m3, float %x3, float 0xC1D0000000000000
  %x4 = extractelement <8 x float> %d, i32 4
  %c4 = icmp sle i32 %q, %k4
  %e4 = icmp ne i32 %k4, 0
  %m4 = and i1 %c4, %e4
  %s4 = select i1 %m4, float %x4, float 0xC1D0000000000000
  %x5 = extractelement <8 x float> %d, i32 5
  %c5 = icmp sle i32 %q, %k5
  %e5 = icmp ne i32 %k5, 0
  %m5 = and i1 %c5, %e5
  %s5 = select i1 %m5, float %x5, float 0xC1D0000000000000
  %x6 = extractelement <8 x float> %d, i32 6
  %c6 = icmp sle i32 %q, %k6
  %e6 = icmp ne i32 %k6, 0
  %m6 = and i1 %c6, %e6
  %s6 = select i1 %m6, float %x6, float 0xC1D0000000000000
  %x7 = extractelement <8 x float> %d, i32 7
  %c7 = icmp sle i32 %q, %k7
  %e7 = icmp ne i32 %k7, 0
  %m7 = and i1 %c7, %e7
  %s7 = select i1 %m7, float %x7, float 0xC1D0000000000000
  %v0 = insertelement <8 x float> undef, float %s0, i32 0
  %v1 = insertelement <8 x float> %v0, float %s1, i32 1
  %v2 = insertelement <8 x float> %v1, float %s2, i32 2
  %v3 = insertelement <8 x float> %v2, float %s3, i32 3
  %v4 = insertelement <8 x float> %v3, float %s4, i32 4
  %v5 = insertelement <8 x float> %v4, float %s5, i32 5
  %v6 = insertelement <8 x float> %v5, float %s6, i32 6
  %v7 = insertelement <8 x float> %v6, float %s7, i32 7
  %r = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %v7, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

; Each column mask %or<j> feeds the selects of two rows. RematChainsAnalysis
; stops at a value with several users, so the remat filter does not hold the
; column masks back. The select values need three instructions after the DPAS,
; the masks two, so without the mask filter MaxWeight computes all column masks
; before any select value, and each holds a flag register until the last row.
; With the filter, a column mask follows the first value of its column.

define spir_kernel void @shared_masks(ptr addrspace(1) %out, <8 x i16> %a, <8 x i32> %b, <2 x i32> %q, <4 x i32> %k, i32 %klen) {
; CHECK-LABEL: @shared_masks(
; CHECK-NOT:   = or i1
; CHECK:       %y00 = fmul float %w00, %w00
; CHECK:       %or0 = or i1 %e0, %f0
; CHECK:       %s00 = select i1 %m00
; CHECK-NOT:   = or i1
; CHECK:       %y01 = fmul float %w01, %w01
; CHECK:       %or1 = or i1 %e1, %f1
; CHECK:       ret void
entry:
  br label %body

body:
  %d = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %q0 = extractelement <2 x i32> %q, i32 0
  %q1 = extractelement <2 x i32> %q, i32 1
  %k0 = extractelement <4 x i32> %k, i32 0
  %e0 = icmp ne i32 %k0, 0
  %f0 = icmp slt i32 %k0, %klen
  %or0 = or i1 %e0, %f0
  %k1 = extractelement <4 x i32> %k, i32 1
  %e1 = icmp ne i32 %k1, 0
  %f1 = icmp slt i32 %k1, %klen
  %or1 = or i1 %e1, %f1
  %k2 = extractelement <4 x i32> %k, i32 2
  %e2 = icmp ne i32 %k2, 0
  %f2 = icmp slt i32 %k2, %klen
  %or2 = or i1 %e2, %f2
  %k3 = extractelement <4 x i32> %k, i32 3
  %e3 = icmp ne i32 %k3, 0
  %f3 = icmp slt i32 %k3, %klen
  %or3 = or i1 %e3, %f3
  %x00 = extractelement <8 x float> %d, i32 0
  %u00 = fmul float %x00, 2.0
  %w00 = fadd float %u00, 1.0
  %y00 = fmul float %w00, %w00
  %c00 = icmp sle i32 %q0, %k0
  %m00 = and i1 %c00, %or0
  %s00 = select i1 %m00, float %y00, float 0xC1D0000000000000
  %x01 = extractelement <8 x float> %d, i32 1
  %u01 = fmul float %x01, 2.0
  %w01 = fadd float %u01, 1.0
  %y01 = fmul float %w01, %w01
  %c01 = icmp sle i32 %q0, %k1
  %m01 = and i1 %c01, %or1
  %s01 = select i1 %m01, float %y01, float 0xC1D0000000000000
  %x02 = extractelement <8 x float> %d, i32 2
  %u02 = fmul float %x02, 2.0
  %w02 = fadd float %u02, 1.0
  %y02 = fmul float %w02, %w02
  %c02 = icmp sle i32 %q0, %k2
  %m02 = and i1 %c02, %or2
  %s02 = select i1 %m02, float %y02, float 0xC1D0000000000000
  %x03 = extractelement <8 x float> %d, i32 3
  %u03 = fmul float %x03, 2.0
  %w03 = fadd float %u03, 1.0
  %y03 = fmul float %w03, %w03
  %c03 = icmp sle i32 %q0, %k3
  %m03 = and i1 %c03, %or3
  %s03 = select i1 %m03, float %y03, float 0xC1D0000000000000
  %x10 = extractelement <8 x float> %d, i32 4
  %u10 = fmul float %x10, 2.0
  %w10 = fadd float %u10, 1.0
  %y10 = fmul float %w10, %w10
  %c10 = icmp sle i32 %q1, %k0
  %m10 = and i1 %c10, %or0
  %s10 = select i1 %m10, float %y10, float 0xC1D0000000000000
  %x11 = extractelement <8 x float> %d, i32 5
  %u11 = fmul float %x11, 2.0
  %w11 = fadd float %u11, 1.0
  %y11 = fmul float %w11, %w11
  %c11 = icmp sle i32 %q1, %k1
  %m11 = and i1 %c11, %or1
  %s11 = select i1 %m11, float %y11, float 0xC1D0000000000000
  %x12 = extractelement <8 x float> %d, i32 6
  %u12 = fmul float %x12, 2.0
  %w12 = fadd float %u12, 1.0
  %y12 = fmul float %w12, %w12
  %c12 = icmp sle i32 %q1, %k2
  %m12 = and i1 %c12, %or2
  %s12 = select i1 %m12, float %y12, float 0xC1D0000000000000
  %x13 = extractelement <8 x float> %d, i32 7
  %u13 = fmul float %x13, 2.0
  %w13 = fadd float %u13, 1.0
  %y13 = fmul float %w13, %w13
  %c13 = icmp sle i32 %q1, %k3
  %m13 = and i1 %c13, %or3
  %s13 = select i1 %m13, float %y13, float 0xC1D0000000000000
  %v0 = insertelement <8 x float> zeroinitializer, float %s00, i32 0
  %v1 = insertelement <8 x float> %v0, float %s01, i32 1
  %v2 = insertelement <8 x float> %v1, float %s02, i32 2
  %v3 = insertelement <8 x float> %v2, float %s03, i32 3
  %v4 = insertelement <8 x float> %v3, float %s10, i32 4
  %v5 = insertelement <8 x float> %v4, float %s11, i32 5
  %v6 = insertelement <8 x float> %v5, float %s12, i32 6
  %v7 = insertelement <8 x float> %v6, float %s13, i32 7
  %r = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %v7, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
