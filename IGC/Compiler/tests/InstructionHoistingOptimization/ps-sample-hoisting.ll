;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; Default (non-speculative) mode:
; RUN: igc_opt -platformbmg --opaque-pointers --regkey EnableInstructionHoistingOptimizationPS=1 -igc-instruction-hoisting-optimization -S -inputps < %s | FileCheck %s --check-prefixes=CHECK,NOSPEC
; Speculative mode (hoisting across a conditional branch allowed):
; RUN: igc_opt -platformbmg --opaque-pointers --regkey EnableInstructionHoistingOptimizationPS=1,EnableSpeculativeSampleHoisting=1 -igc-instruction-hoisting-optimization -S -inputps < %s | FileCheck %s --check-prefixes=CHECK,SPEC

; ------------------------------------------------------------------------------
; Pixel-shader sample-send hoisting (per-sampler, bounded to the dominating
; block, register-pressure gated at the widest PS SIMD).
;   1. safe_hoist                 - straight-line sampler hoisted up to its
;                                   operand's block, after that operand. Happens
;                                   in BOTH modes (non-speculative).
;   2. implicit_no_speculative    - implicit-derivative sampler (GenISA.sampleptr)
;                                   in a conditional block is NEVER hoisted: the
;                                   default mode forbids speculation, and even
;                                   with speculation ON the derivative guard keeps
;                                   it put (a divergent branch would change the
;                                   quad's derivatives).
;   3. explicit_speculative       - explicit-LOD sampler (GenISA.sampleLptr) in a
;                                   conditional block: left in place by default,
;                                   hoisted into the dominating block only when
;                                   speculation is enabled (no derivatives, so
;                                   speculating it is safe).
; These tiny functions stay well under the GRF budget, so the pressure gate never
; rejects.
; ------------------------------------------------------------------------------

; Straight-line hoist: the sample moves from %next up into %entry, placed after
; its in-block operand %coord. Non-speculative, so it happens in both modes.
define spir_kernel void @safe_hoist(ptr %t, ptr %s, ptr addrspace(1) %out) {
; CHECK-LABEL: @safe_hoist(
; CHECK:       entry:
; CHECK:         [[COORD:%.*]] = fadd float 1.000000e+00, 2.000000e+00
; CHECK:         call <4 x float> @llvm.genx.GenISA.sampleptr{{.*}}(float [[COORD]],
; A hoisted sampler is tagged so CodeSinking won't sink it back down.
; CHECK-SAME:    !igc.latencyHoisted
; CHECK:         br label %next
; CHECK:       next:
; CHECK:         extractelement
entry:
  %coord = fadd float 1.000000e+00, 2.000000e+00
  br label %next

next:
  %smp = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p0.p0.p0(
      float %coord, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00,
      float 0.000000e+00, ptr null, ptr %t, ptr %s, i32 0, i32 0, i32 0)
  %r = extractelement <4 x float> %smp, i32 0
  store float %r, ptr addrspace(1) %out
  ret void
}

; Implicit-derivative sample in a conditional block: not hoisted in either mode.
; Default mode: speculation gate. Speculative mode: derivative guard.
define spir_kernel void @implicit_no_speculative(ptr %t, ptr %s, ptr addrspace(1) %out, i32 %cond) {
; CHECK-LABEL: @implicit_no_speculative(
; CHECK:       entry:
; CHECK-NOT:     @llvm.genx.GenISA.sampleptr
; CHECK:       cond_bb:
; CHECK:         call <4 x float> @llvm.genx.GenISA.sampleptr
entry:
  %coord = fadd float 1.000000e+00, 2.000000e+00
  %c = icmp eq i32 %cond, 0
  br i1 %c, label %cond_bb, label %merge

cond_bb:
  %smp = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p0.p0.p0(
      float %coord, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00,
      float 0.000000e+00, ptr null, ptr %t, ptr %s, i32 0, i32 0, i32 0)
  %r = extractelement <4 x float> %smp, i32 0
  br label %merge

merge:
  %phi = phi float [ %r, %cond_bb ], [ 0.000000e+00, %entry ]
  store float %phi, ptr addrspace(1) %out
  ret void
}

; Explicit-LOD sample in a conditional block: left in place by default, hoisted
; into the dominating %entry only when speculation is enabled.
define spir_kernel void @explicit_speculative(ptr %t, ptr %s, ptr addrspace(1) %out, i32 %cond) {
; CHECK-LABEL: @explicit_speculative(
; SPEC:        entry:
; SPEC:          call <4 x float> @llvm.genx.GenISA.sampleLptr
; SPEC:          br i1
; NOSPEC:      entry:
; NOSPEC-NOT:    @llvm.genx.GenISA.sampleLptr
; NOSPEC:      cond_bb:
; NOSPEC:        call <4 x float> @llvm.genx.GenISA.sampleLptr
entry:
  %coord = fadd float 1.000000e+00, 2.000000e+00
  %c = icmp eq i32 %cond, 0
  br i1 %c, label %cond_bb, label %merge

cond_bb:
  %smp = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p0.p0.p0(
      float 0.000000e+00, float %coord, float 0.000000e+00, float 0.000000e+00,
      float 0.000000e+00, ptr null, ptr %t, ptr %s, i32 0, i32 0, i32 0)
  %r = extractelement <4 x float> %smp, i32 0
  br label %merge

merge:
  %phi = phi float [ %r, %cond_bb ], [ 0.000000e+00, %entry ]
  store float %phi, ptr addrspace(1) %out
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p0.p0.p0(
    float, float, float, float, float, ptr, ptr, ptr, i32, i32, i32)
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p0.p0.p0(
    float, float, float, float, float, ptr, ptr, ptr, i32, i32, i32)
