;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Only the OpenCL context is constructed in OS builds, so --inputcs/--inputps
; need the shader-types feature.
; REQUIRES: llvm-14-plus, regkeys, shader-types
;
; RUN: igc_opt --opaque-pointers --platformbmg --inputcs -igc-memopt -S \
; RUN:   --regkey=EnableUniformSLMLoadWiden=0 < %s | FileCheck %s --check-prefix=KEY-OFF
;
; RUN: igc_opt --opaque-pointers --platformbmg --inputcs -igc-memopt -S \
; RUN:   --regkey=EnableUniformSLMLoadWiden=1 < %s | FileCheck %s --check-prefix=KEY-ON
;
; RUN: igc_opt --opaque-pointers --platformbmg --inputps -igc-memopt -S \
; RUN:   --regkey=EnableUniformSLMLoadWiden=1 < %s | FileCheck %s --check-prefix=KEY-ON
;
; RUN: igc_opt --opaque-pointers --platformbmg --inputocl -igc-memopt -S \
; RUN:   --regkey=EnableUniformSLMLoadWiden=0 < %s | FileCheck %s --check-prefix=OCL

; Test: the uniform-load merge path in MemOpt was gated on ShaderType::OPENCL_SHADER.
;
; Each function loads four uniform i32 at element offsets 0/2/4/6 off a pointer
; argument: uniform, but not contiguous. Merging them needs the uniform path,
; which spans the convex hull of the run (7 elements) and reads the gaps rather
; than requiring adjacency. Falling back to the contiguity merge can only pair
; 0..2 and 4..6, leaving two <3 x i32>.
;
; The two functions differ only in address space, which is what the key selects
; on: @widen_slm is addrspace(3) (SLM), @widen_ugm is addrspace(1) (global).
; With the key on, a compute or pixel shader gets the hull merge for SLM and
; keeps the contiguity fallback for global. The key also rounds the SLM hull up
; to a legal LSC transposed length, so 7 lands as <8 x i32>; see
; uniform-slm-load-widen.ll for that half on its own.
;
; The OpenCL arm is the control: it reaches the uniform path on every address
; space regardless of the key, so both functions get the bare hull there.

define spir_kernel void @widen_slm(ptr addrspace(3) %src, ptr addrspace(3) %dst) {
entry:
  %g0 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 0
  %l0 = load i32, ptr addrspace(3) %g0, align 4
  %g2 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 2
  %l2 = load i32, ptr addrspace(3) %g2, align 4
  %g4 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 4
  %l4 = load i32, ptr addrspace(3) %g4, align 4
  %g6 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 6
  %l6 = load i32, ptr addrspace(3) %g6, align 4
  %a = add i32 %l0, %l2
  %b = add i32 %l4, %l6
  %c = add i32 %a, %b
  store i32 %c, ptr addrspace(3) %dst, align 4
  ret void
}

define spir_kernel void @widen_ugm(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %g0 = getelementptr inbounds i32, ptr addrspace(1) %src, i64 0
  %l0 = load i32, ptr addrspace(1) %g0, align 4
  %g2 = getelementptr inbounds i32, ptr addrspace(1) %src, i64 2
  %l2 = load i32, ptr addrspace(1) %g2, align 4
  %g4 = getelementptr inbounds i32, ptr addrspace(1) %src, i64 4
  %l4 = load i32, ptr addrspace(1) %g4, align 4
  %g6 = getelementptr inbounds i32, ptr addrspace(1) %src, i64 6
  %l6 = load i32, ptr addrspace(1) %g6, align 4
  %a = add i32 %l0, %l2
  %b = add i32 %l4, %l6
  %c = add i32 %a, %b
  store i32 %c, ptr addrspace(1) %dst, align 4
  ret void
}

; Key off: contiguity-only merge everywhere, nothing spans the gap at element 3.
;
; KEY-OFF-LABEL: define spir_kernel void @widen_slm
; KEY-OFF:         load <3 x i32>, ptr addrspace(3) %g0, align 4
; KEY-OFF:         %g4 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 4
; KEY-OFF:         load <3 x i32>, ptr addrspace(3) %g4, align 4
;
; KEY-OFF-LABEL: define spir_kernel void @widen_ugm
; KEY-OFF:         load <3 x i32>, ptr addrspace(1) %g0, align 4
; KEY-OFF:         %g4 = getelementptr inbounds i32, ptr addrspace(1) %src, i64 4
; KEY-OFF:         load <3 x i32>, ptr addrspace(1) %g4, align 4

; Key on: hull merge for SLM only. One load covering elements 0..6, gaps
; included, rounded up to 8. Global keeps the two halves.
;
; KEY-ON-LABEL: define spir_kernel void @widen_slm
; KEY-ON:         load <8 x i32>, ptr addrspace(3) %g0, align 4
; KEY-ON-NOT:     load
;
; KEY-ON-LABEL: define spir_kernel void @widen_ugm
; KEY-ON:         load <3 x i32>, ptr addrspace(1) %g0, align 4
; KEY-ON:         %g4 = getelementptr inbounds i32, ptr addrspace(1) %src, i64 4
; KEY-ON:         load <3 x i32>, ptr addrspace(1) %g4, align 4

; OpenCL already had the hull merge on every address space before this change.
;
; OCL-LABEL: define spir_kernel void @widen_slm
; OCL:         load <7 x i32>, ptr addrspace(3) %g0, align 4
; OCL-NOT:     load
;
; OCL-LABEL: define spir_kernel void @widen_ugm
; OCL:         load <7 x i32>, ptr addrspace(1) %g0, align 4
; OCL-NOT:     load

!igc.functions = !{!0, !3}

!0 = !{ptr @widen_slm, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @widen_ugm, !1}
