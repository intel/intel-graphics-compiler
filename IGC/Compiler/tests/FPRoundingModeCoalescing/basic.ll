;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-fp-rounding-mode-coalescing -check-debugify -S < %s 2>&1 | FileCheck %s

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

; Scenario:
;   * Three FMA RTZ (non-default RM).
;   * No other instructions depending on RM.
; Result:
;   * No need to reorder.
define spir_kernel void @no_dependence_on_rm(double %src1, double %src2, double %src3, double addrspace(1)* %dst, i32 addrspace(1)* %other) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %tmp1 = load i32, i32 addrspace(1)* %other
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %src3)
; CHECK:         %tmp2 = add i32 %tmp1, 1
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src3, double %src2, double %src1)
; CHECK:         %tmp3 = add i32 %tmp2, 1
; CHECK:         %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src3, double %src2)
; CHECK:         %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
; CHECK:         %result2 = fadd double %result1, %fma.rtz.3.result
; CHECK:         store double %result2, double addrspace(1)* %dst
; CHECK:         store i32 %tmp3, i32 addrspace(1)* %other
; CHECK:         ret void
  %tmp1 = load i32, i32 addrspace(1)* %other
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %src3)
  %tmp2 = add i32 %tmp1, 1
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src3, double %src2, double %src1)
  %tmp3 = add i32 %tmp2, 1
  %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src3, double %src2)
  %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
  %result2 = fadd double %result1, %fma.rtz.3.result
  store double %result2, double addrspace(1)* %dst
  store i32 %tmp3, i32 addrspace(1)* %other
  ret void
}

; Scenario:
;   * Three FMA RTZ (non-default RM).
;   * Other instructions depend on FMAs' results.
; Result:
;   * FMA RTZ are reordered and grouped together.
define spir_kernel void @no_uses_for_fma_rtz(double %src1, double %src2, double %src3, double addrspace(1)* %dst, i32 addrspace(1)* %other) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %tmp.i32.1 = load i32, i32 addrspace(1)* %other
; CHECK:         %tmp.double.1 = fadd double %src1, %src2
; CHECK:         %tmp.double.2 = fadd double %tmp.double.1, %src3
; CHECK:         %tmp.i32.2 = add i32 %tmp.i32.1, 1
; CHECK:         %tmp.double.3 = fadd double %tmp.double.2, %src3
; CHECK:         %tmp.double.4 = fadd double %tmp.double.3, %src3
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %src3)
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src3, double %src2, double %src1)
; CHECK:         %tmp.i32.3 = add i32 %tmp.i32.2, 1
; CHECK:         %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src3, double %src2)
; CHECK:         %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
; CHECK:         %result2 = fadd double %result1, %fma.rtz.3.result
; CHECK:         %result3 = fadd double %result2, %tmp.double.4
; CHECK:         store double %result3, double addrspace(1)* %dst
; CHECK:         store i32 %tmp.i32.3, i32 addrspace(1)* %other
; CHECK:         ret void
  %tmp.i32.1 = load i32, i32 addrspace(1)* %other
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %src3)
  %tmp.double.1 = fadd double %src1, %src2
  %tmp.double.2 = fadd double %tmp.double.1, %src3
  %tmp.i32.2 = add i32 %tmp.i32.1, 1
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src3, double %src2, double %src1)
  %tmp.double.3 = fadd double %tmp.double.2, %src3
  %tmp.double.4 = fadd double %tmp.double.3, %src3
; COM: Can insert after this fadd above.
; COM: i32 instruction below doesn't impact RM switching.
  %tmp.i32.3 = add i32 %tmp.i32.2, 1
  %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src3, double %src2)
  %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
  %result2 = fadd double %result1, %fma.rtz.3.result
  %result3 = fadd double %result2, %tmp.double.4
  store double %result3, double addrspace(1)* %dst
  store i32 %tmp.i32.3, i32 addrspace(1)* %other
  ret void
}

; Scenario:
;   * Three FMA RTZ (non-default RM).
;   * FMAs have uses ignoring RM.
;   * No FMA depends on the results of previous FMA.
; Result:
;   * FMA RTZ are reordered and grouped together.
;   * Uses are not moved compared to their previous relation to FMA.
define spir_kernel void @uses_ignoring_rm_can_be_moved(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; COM: First dependencies for FMAs.
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.arg = fsub double %src2, %src3
; CHECK:         %fma.rtz.3.arg = fsub double %src1, %src2
; COM: Next all FMAs and users ignoring RM.
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %tmp11 = fcmp oeq double %src1, %fma.rtz.1.result
; CHECK:         %tmp12 = fcmp oeq double %src2, %fma.rtz.1.result
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; CHECK:         %tmp21 = fcmp oeq double %src1, %fma.rtz.2.result
; CHECK:         %tmp22 = fcmp oeq double %src2, %fma.rtz.2.result
; CHECK:         %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.3.arg)
; COM: Finally the unmodified rest.
; CHECK:         %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
; CHECK:         %result2 = fadd double %result1, %fma.rtz.3.result
; CHECK:         store double %result2, double addrspace(1)* %dst
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)

; COM: Depends on result of first FMA, but ignores RM.
  %tmp11 = fcmp oeq double %src1, %fma.rtz.1.result
  %tmp12 = fcmp oeq double %src2, %fma.rtz.1.result

  %fma.rtz.2.arg = fsub double %src2, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)

; COM: Depends on result of second FMA, but ignores RM.
  %tmp21 = fcmp oeq double %src1, %fma.rtz.2.result
  %tmp22 = fcmp oeq double %src2, %fma.rtz.2.result

  %fma.rtz.3.arg = fsub double %src1, %src2
  %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.3.arg)

  %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
  %result2 = fadd double %result1, %fma.rtz.3.result
  store double %result2, double addrspace(1)* %dst
  ret void
}

; Scenario:
;   * Three FMA RTZ (non-default RM).
;   * FMAs have uses in default RM.
;   * No FMA depends on the results of previous FMA.
; Result:
;   * FMA RTZ are reordered and grouped together.
;   * Uses are moved after last FMA.
define spir_kernel void @uses_in_default_rm_can_be_moved(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; COM: First dependencies for FMAs.
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.arg = fsub double %src2, %src3
; CHECK:         %fma.rtz.3.arg = fsub double %src1, %src2
; COM: Next all FMAs grouped together.
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; CHECK:         %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.3.arg)
; COM: Then all uses of FMAs' results that could be moved.
; CHECK:         %tmp11 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %tmp12 = fadd double %src2, %tmp11
; CHECK:         %tmp13 = fadd double %fma.rtz.1.result, %tmp12
; CHECK:         %tmp21 = fadd double %src1, %fma.rtz.2.result
; CHECK:         %tmp22 = fadd double %fma.rtz.1.result, %tmp21
; COM: Finally the unmodified rest.
; CHECK:         %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
; CHECK:         %result2 = fadd double %result1, %fma.rtz.3.result
; CHECK:         store double %result2, double addrspace(1)* %dst
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)

; COM: Depends on result of first FMA, but can be moved after third FMA.
  %tmp11 = fadd double %src1, %fma.rtz.1.result
  %tmp12 = fadd double %src2, %tmp11
  %tmp13 = fadd double %fma.rtz.1.result, %tmp12

  %fma.rtz.2.arg = fsub double %src2, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)

; COM: Depends on result of second FMA, but can be moved after third FMA.
  %tmp21 = fadd double %src1, %fma.rtz.2.result
  %tmp22 = fadd double %fma.rtz.1.result, %tmp21

  %fma.rtz.3.arg = fsub double %src1, %src2
  %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.3.arg)
; COM: Users will be moved here.

  %result1 = fadd double %fma.rtz.1.result, %fma.rtz.2.result
  %result2 = fadd double %result1, %fma.rtz.3.result
  store double %result2, double addrspace(1)* %dst
  ret void
}

; Scenario:
;   * Three FMA RTZ (non-default RM).
;   * Second FMA directly uses result of first FMA.
;   * Third FMA directly uses result of second FMA.
; Result:
;   * FMA RTZ are reordered and grouped together.
define spir_kernel void @direct_use(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.3.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %fma.rtz.1.result, double %fma.rtz.2.arg)
; CHECK:         %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %fma.rtz.2.result, double %fma.rtz.3.arg)
; CHECK:         %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %tmp.rtz.2 = fadd double %src1, %fma.rtz.2.result
; CHECK:         %tmp.rtz.3 = fadd double %src1, %fma.rtz.3.result
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
  %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result

  %fma.rtz.2.arg = fsub double %src1, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %fma.rtz.1.result, double %fma.rtz.2.arg)
  %tmp.rtz.2 = fadd double %src1, %fma.rtz.2.result

  %fma.rtz.3.arg = fsub double %src1, %src3
  %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %fma.rtz.2.result, double %fma.rtz.3.arg)
  %tmp.rtz.3 = fadd double %src1, %fma.rtz.3.result

  ret void
}

; Scenario:
;   * Two uitof RTZ (non-default RM).
;   * Second uitof has indrect dependency on the result of the first uitof.
; Result:
;   * Instructions can't be moved because of the dependency.
define spir_kernel void @indirect_use(i32 %input) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)
; CHECK:         %conv3.i = fptoui float %0 to i32
; CHECK:         %sub4.i = sub i32 %input, %conv3.i
; CHECK:         %1 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %sub4.i)
; CHECK:         ret void

  %0 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)
  %conv3.i = fptoui float %0 to i32
  %sub4.i = sub i32 %input, %conv3.i
  %1 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %sub4.i)

  ret void
}

; Scenario:
;   * Four FMA RTZ (non-default RM).
;   * Third FMA depends on the user of second FMA.
; Result:
;   * Second FMA can't be moved because of the dependency it creates.
;   * Other instructions can be reordered.
define spir_kernel void @uses_in_default_rm_create_dependency(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; COM: Second FMA can't be moved, as it results is used to calculate arg for third FMA.
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.arg = fsub double %src2, %src3
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; CHECK:         %tmp2 = fadd double %src1, %fma.rtz.2.result
; CHECK:         %fma.rtz.3.arg = fsub double %src1, %src2
; CHECK:         %fma.rtz.4.arg = fsub double %src2, %src3
; COM: Grouped FMAs.
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %tmp2, double %fma.rtz.3.arg)
; CHECK:         %fma.rtz.4.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.4.arg)
; COM: Other moved uses.
; CHECK:         %tmp1 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %tmp3 = fadd double %src1, %fma.rtz.3.result
; CHECK:         %tmp4 = fadd double %src1, %fma.rtz.4.result
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)

  %tmp1 = fadd double %src1, %fma.rtz.1.result
  %fma.rtz.2.arg = fsub double %src2, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)

; COM: Result of second FMA is used to calculate arg for third FMA. Because of this dependency, instructions can't be moved.
  %tmp2 = fadd double %src1, %fma.rtz.2.result
  %fma.rtz.3.arg = fsub double %src1, %src2
  %fma.rtz.3.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %tmp2, double %fma.rtz.3.arg)

  %tmp3 = fadd double %src1, %fma.rtz.3.result
  %fma.rtz.4.arg = fsub double %src2, %src3
  %fma.rtz.4.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.4.arg)

  %tmp4 = fadd double %src1, %fma.rtz.4.result

  ret void
}

; Scenario:
;   * uitof RTZ (non-default RM) to move has chain of uses.
;   * uitof RTZ can be inserted after default-RM instruction (insert point).
; Result:
;   * uitof RTZ and uses before insert point are moved after insert point.
;   * Uses after insert point are not moved.
define spir_kernel void @move_after_insert_point(i32 %input) {
entry:
; CHECK-LABEL: entry:
; COM: fadd - insert point (default RM)
; CHECK:         %insert = fadd float 2.000000e+00, 3.000000e+00
; COM: %0 and %chain1 moved after insert point
; CHECK:         %0 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)
; CHECK:         %chain1 = fdiv float 1.000000e+00, %0
; CHECK:         %chain2 = fdiv float 1.000000e+00, %chain1
; CHECK:         %1 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)
; CHECK:         %chain3 = fdiv float 1.000000e+00, %chain2
; CHECK:         %2 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)
; CHECK:         ret void

  ; One RTZ instruction and one RM-agnostic instruction. Both can be moved.
  %0 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)
  %chain1 = fdiv float 1.000000e+00, %0

  ; Default RM instruction. Becomes insert point.
  %insert = fadd float 2.000000e+00, 3.000000e+00

  ; chain2 and chain3 are after insert point and should not be moved.
  %chain2 = fdiv float 1.000000e+00, %chain1
  %1 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)
  %chain3 = fdiv float 1.000000e+00, %chain2
  %2 = call float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32 %input)

  ret void
}

; Scenario:
;   * Two FMA RTZ (non-default RM).
;   * Two FMA RTP (non-default RM).
;   * Third FMA depends on the user of second FMA.
; Result:
;   * Pass only reorders one non-default RM.
;   * FMA RTP are grouped.
;   * FMA RTZ are not grouped.
define spir_kernel void @multiple_rm_modes(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; COM: FMA RTZ are untouched.
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %fma.rtp.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; CHECK:         %tmp.rtz.2 = fadd double %src1, %fma.rtz.2.result
; CHECK:         %fma.rtp.2.arg = fsub double %src1, %src3
; COM: FMA RTP are grouped.
; CHECK:         %fma.rtp.1.result = call double @llvm.genx.GenISA.fma.rtp.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtp.1.arg)
; CHECK:         %fma.rtp.2.result = call double @llvm.genx.GenISA.fma.rtp.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtp.2.arg)
; CHECK:         %tmp.rtp.1 = fadd double %src1, %fma.rtp.1.result
; CHECK:         %tmp.rtp.2 = fadd double %src1, %fma.rtp.2.result
; CHECK:         ret void

; COM: First RTZ FMA
  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
  %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result

; COM: First RTP FMA
  %fma.rtp.1.arg = fsub double %src1, %src3
  %fma.rtp.1.result = call double @llvm.genx.GenISA.fma.rtp.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtp.1.arg)
  %tmp.rtp.1 = fadd double %src1, %fma.rtp.1.result

; COM: Second RTZ FMA
  %fma.rtz.2.arg = fsub double %src1, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
  %tmp.rtz.2 = fadd double %src1, %fma.rtz.2.result

; COM: Second RTP FMA
  %fma.rtp.2.arg = fsub double %src1, %src3
  %fma.rtp.2.result = call double @llvm.genx.GenISA.fma.rtp.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtp.2.arg)
  %tmp.rtp.2 = fadd double %src1, %fma.rtp.2.result

  ret void
}

; Don't reorder instructions if max distance threshold is reached.
define spir_kernel void @max_distance_threshold_with_insert_point(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %tmp.rtz.2 = fadd double %src1, %tmp.rtz.1
; CHECK:         %tmp.rtz.3 = fadd double %src1, %tmp.rtz.2
; CHECK:         %tmp.rtz.4 = fadd double %src1, %tmp.rtz.3
; CHECK:         %tmp.rtz.5 = fadd double %src1, %tmp.rtz.4
; CHECK:         %tmp.rtz.6 = fadd double %src1, %tmp.rtz.5
; CHECK:         %tmp.rtz.7 = fadd double %src1, %tmp.rtz.6
; CHECK:         %tmp.rtz.8 = fadd double %src1, %tmp.rtz.7
; CHECK:         %tmp.rtz.9 = fadd double %src1, %tmp.rtz.8
; CHECK:         %tmp.rtz.10 = fadd double %src1, %tmp.rtz.9
; CHECK:         %tmp.rtz.11 = fadd double %src1, %tmp.rtz.10
; CHECK:         %tmp.rtz.12 = fadd double %src1, %tmp.rtz.11
; CHECK:         %tmp.rtz.13 = fadd double %src1, %tmp.rtz.12
; CHECK:         %tmp.rtz.14 = fadd double %src1, %tmp.rtz.13
; CHECK:         %tmp.rtz.15 = fadd double %src1, %tmp.rtz.14
; CHECK:         %tmp.rtz.16 = fadd double %src1, %tmp.rtz.15
; CHECK:         %tmp.rtz.17 = fadd double %src1, %tmp.rtz.16
; CHECK:         %tmp.rtz.18 = fadd double %src1, %tmp.rtz.17
; CHECK:         %tmp.rtz.19 = fadd double %src1, %tmp.rtz.18
; CHECK:         %tmp.rtz.20 = fadd double %src1, %tmp.rtz.19
; CHECK:         %tmp.rtz.21 = fadd double %src1, %tmp.rtz.20
; CHECK:         %fma.rtz.2.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
  %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
  %tmp.rtz.2 = fadd double %src1, %tmp.rtz.1
  %tmp.rtz.3 = fadd double %src1, %tmp.rtz.2
  %tmp.rtz.4 = fadd double %src1, %tmp.rtz.3
  %tmp.rtz.5 = fadd double %src1, %tmp.rtz.4
  %tmp.rtz.6 = fadd double %src1, %tmp.rtz.5
  %tmp.rtz.7 = fadd double %src1, %tmp.rtz.6
  %tmp.rtz.8 = fadd double %src1, %tmp.rtz.7
  %tmp.rtz.9 = fadd double %src1, %tmp.rtz.8
  %tmp.rtz.10 = fadd double %src1, %tmp.rtz.9
  %tmp.rtz.11 = fadd double %src1, %tmp.rtz.10
  %tmp.rtz.12 = fadd double %src1, %tmp.rtz.11
  %tmp.rtz.13 = fadd double %src1, %tmp.rtz.12
  %tmp.rtz.14 = fadd double %src1, %tmp.rtz.13
  %tmp.rtz.15 = fadd double %src1, %tmp.rtz.14
  %tmp.rtz.16 = fadd double %src1, %tmp.rtz.15
  %tmp.rtz.17 = fadd double %src1, %tmp.rtz.16
  %tmp.rtz.18 = fadd double %src1, %tmp.rtz.17
  %tmp.rtz.19 = fadd double %src1, %tmp.rtz.18
  %tmp.rtz.20 = fadd double %src1, %tmp.rtz.19
  %tmp.rtz.21 = fadd double %src1, %tmp.rtz.20

  %fma.rtz.2.arg = fsub double %src1, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)

  ret void
}

; Don't reorder instructions if max distance threshold is reached.
define spir_kernel void @max_distance_threshold_without_insert_point(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %tmp.rtz.2 = fadd double %src1, %tmp.rtz.1
; CHECK:         %tmp.rtz.3 = fadd double %src1, %tmp.rtz.2
; CHECK:         %tmp.rtz.4 = fadd double %src1, %tmp.rtz.3
; CHECK:         %tmp.rtz.5 = fadd double %src1, %tmp.rtz.4
; CHECK:         %tmp.rtz.6 = fadd double %src1, %tmp.rtz.5
; CHECK:         %tmp.rtz.7 = fadd double %src1, %tmp.rtz.6
; CHECK:         %tmp.rtz.8 = fadd double %src1, %tmp.rtz.7
; CHECK:         %tmp.rtz.9 = fadd double %src1, %tmp.rtz.8
; CHECK:         %tmp.rtz.10 = fadd double %src1, %tmp.rtz.9
; CHECK:         %tmp.rtz.11 = fadd double %src1, %tmp.rtz.10
; CHECK:         %tmp.rtz.12 = fadd double %src1, %tmp.rtz.11
; CHECK:         %tmp.rtz.13 = fadd double %src1, %tmp.rtz.12
; CHECK:         %tmp.rtz.14 = fadd double %src1, %tmp.rtz.13
; CHECK:         %tmp.rtz.15 = fadd double %src1, %tmp.rtz.14
; CHECK:         %tmp.rtz.16 = fadd double %src1, %tmp.rtz.15
; CHECK:         %tmp.rtz.17 = fadd double %src1, %tmp.rtz.16
; CHECK:         %tmp.rtz.18 = fadd double %src1, %tmp.rtz.17
; CHECK:         %tmp.rtz.19 = fadd double %src1, %tmp.rtz.18
; CHECK:         %tmp.rtz.20 = fadd double %src1, %tmp.rtz.19
; CHECK:         %tmp.rtz.21 = fadd double %src1, %tmp.rtz.20
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %src3)
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
  %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
  %tmp.rtz.2 = fadd double %src1, %tmp.rtz.1
  %tmp.rtz.3 = fadd double %src1, %tmp.rtz.2
  %tmp.rtz.4 = fadd double %src1, %tmp.rtz.3
  %tmp.rtz.5 = fadd double %src1, %tmp.rtz.4
  %tmp.rtz.6 = fadd double %src1, %tmp.rtz.5
  %tmp.rtz.7 = fadd double %src1, %tmp.rtz.6
  %tmp.rtz.8 = fadd double %src1, %tmp.rtz.7
  %tmp.rtz.9 = fadd double %src1, %tmp.rtz.8
  %tmp.rtz.10 = fadd double %src1, %tmp.rtz.9
  %tmp.rtz.11 = fadd double %src1, %tmp.rtz.10
  %tmp.rtz.12 = fadd double %src1, %tmp.rtz.11
  %tmp.rtz.13 = fadd double %src1, %tmp.rtz.12
  %tmp.rtz.14 = fadd double %src1, %tmp.rtz.13
  %tmp.rtz.15 = fadd double %src1, %tmp.rtz.14
  %tmp.rtz.16 = fadd double %src1, %tmp.rtz.15
  %tmp.rtz.17 = fadd double %src1, %tmp.rtz.16
  %tmp.rtz.18 = fadd double %src1, %tmp.rtz.17
  %tmp.rtz.19 = fadd double %src1, %tmp.rtz.18
  %tmp.rtz.20 = fadd double %src1, %tmp.rtz.19
  %tmp.rtz.21 = fadd double %src1, %tmp.rtz.20

  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %src3)

  ret void
}

define spir_kernel void @phi_node_is_ignored(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         br label %l1
; CHECK:       l1:                                               ; preds = %l1, %entry
; CHECK:         %phi = phi double [ %src3, %entry ], [ %tmp13, %l1 ]
; COM: First dependencies for FMAs.
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.arg = fsub double %src1, %src2
; COM: Next all FMAs grouped together.
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; COM: Then all uses of FMAs' results that could be moved.
; CHECK:         %tmp11 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %tmp12 = fadd double %src2, %tmp11
; CHECK:         %tmp13 = fadd double %fma.rtz.1.result, %tmp12
; CHECK:         %tmp14 = fcmp oeq double %tmp13, 5.000000e+00
; CHECK:         br i1 %tmp14, label %l1, label %l2
; CHECK:       l2:                                               ; preds = %l1
; CHECK:         %result = fadd double %fma.rtz.1.result, %fma.rtz.2.result
; CHECK:         store double %result, double addrspace(1)* %dst
; CHECK:         ret void
  br label %l1

l1:
  %phi = phi double [ %src3, %entry ], [ %tmp13, %l1 ]

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)

; COM: Depends on result of first FMA, but can be moved after third FMA.
  %tmp11 = fadd double %src1, %fma.rtz.1.result
  %tmp12 = fadd double %src2, %tmp11
  %tmp13 = fadd double %fma.rtz.1.result, %tmp12
  %tmp14 = fcmp oeq double %tmp13, 5.0

  %fma.rtz.2.arg = fsub double %src1, %src2
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; COM: Users will be moved here.

  br i1 %tmp14, label %l1, label %l2

l2:
  %result = fadd double %fma.rtz.1.result, %fma.rtz.2.result
  store double %result, double addrspace(1)* %dst
  ret void
}

; Don't reorder instructions if there is a risk of side effect.
define spir_kernel void @side_effect(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
; CHECK:         store double %tmp.rtz.1, double addrspace(1)* %dst
; CHECK:         %fma.rtz.2.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
  %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result

  store double %tmp.rtz.1, double addrspace(1)* %dst

  %fma.rtz.2.arg = fsub double %src1, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.2.arg)

  ret void
}

declare float @llvm.genx.GenISA.uitof.rtz.f32.i32(i32)

declare double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double, double, double)
declare double @llvm.genx.GenISA.fma.rtp.f64.f64.f64.f64(double, double, double)

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12}

!0 = !{void (double, double, double, double addrspace(1)*, i32 addrspace(1)*)* @no_dependence_on_rm, !100}
!1 = !{void (double, double, double, double addrspace(1)*, i32 addrspace(1)*)* @no_uses_for_fma_rtz, !100}
!2 = !{void (double, double, double, double addrspace(1)*)* @uses_ignoring_rm_can_be_moved, !100}
!3 = !{void (double, double, double, double addrspace(1)*)* @uses_in_default_rm_can_be_moved, !100}
!4 = !{void (double, double, double, double addrspace(1)*)* @direct_use, !100}
!5 = !{void (i32)* @indirect_use, !100}
!6 = !{void (double, double, double, double addrspace(1)*)* @uses_in_default_rm_create_dependency, !100}
!7 = !{void (i32)* @move_after_insert_point, !100}
!8 = !{void (double, double, double, double addrspace(1)*)* @multiple_rm_modes, !100}
!9 = !{void (double, double, double, double addrspace(1)*)* @max_distance_threshold_with_insert_point, !100}
!10 = !{void (double, double, double, double addrspace(1)*)* @max_distance_threshold_without_insert_point, !100}
!11 = !{void (double, double, double, double addrspace(1)*)* @phi_node_is_ignored, !100}
!12 = !{void (double, double, double, double addrspace(1)*)* @side_effect, !100}
!100 = !{!101}
!101 = !{!"function_type", i32 0}
