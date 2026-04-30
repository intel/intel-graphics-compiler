;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Verify that ConstantCoalescing shrinks a wide vector load when the only
; uses are shufflevector instructions that pick a prefix of the load.

; FIXME: make this test work without shader type
; REQUIRES: llvm-14-plus, shader-types
; RUN: igc_opt --opaque-pointers %s -S --inputcs --platformdg2 -o - -igc-constant-coalescing -dce | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; Test that a <4 x i32> load whose only user is a shufflevector picking the
; first two elements is shrunk to a <2 x i32> load.
define <2 x i32> @f0() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 16
  %ptr = inttoptr i64 %adr to ptr addrspace(2)
  %data = load <4 x i32>, ptr addrspace(2) %ptr, align 16
  %lo = shufflevector <4 x i32> %data, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  ret <2 x i32> %lo
}
; CHECK-LABEL: define <2 x i32> @f0
; CHECK: [[PTR:%.*]] = inttoptr i64 {{.*}} to ptr addrspace(2)
; CHECK: [[DATA:%.*]] = load <2 x i32>, ptr addrspace(2) [[PTR]]
; CHECK-NOT: load <4 x i32>
; CHECK: [[LO:%.*]] = shufflevector <2 x i32> [[DATA]], <2 x i32> poison, <2 x i32> <i32 0, i32 1>
; CHECK: ret <2 x i32> [[LO]]

; Test that mixed extractelement + shufflevector users are also handled.
define i32 @f1() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 32
  %ptr = inttoptr i64 %adr to ptr addrspace(2)
  %data = load <4 x i32>, ptr addrspace(2) %ptr, align 16
  %lo = shufflevector <4 x i32> %data, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  %a = extractelement <2 x i32> %lo, i32 0
  %b = extractelement <4 x i32> %data, i32 1
  %sum = add i32 %a, %b
  ret i32 %sum
}
; CHECK-LABEL: define i32 @f1
; CHECK: load <2 x i32>, ptr addrspace(2)
; CHECK-NOT: load <4 x i32>

; Test that mixed users with disjoint index sets are handled: a shufflevector
; picks lanes 0 and 1, and an extractelement reads lane 2. The combined
; max-elt-plus is 3, so the load should shrink from <4 x i32> to a <3 x i32>
; chunk (rounded to <3 x i32> for 12-byte access).
define i32 @f_disjoint() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 80
  %ptr = inttoptr i64 %adr to ptr addrspace(2)
  %data = load <4 x i32>, ptr addrspace(2) %ptr, align 16
  %lo = shufflevector <4 x i32> %data, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  %a = extractelement <2 x i32> %lo, i32 0
  %b = extractelement <2 x i32> %lo, i32 1
  %c = extractelement <4 x i32> %data, i32 2
  %ab = add i32 %a, %b
  %abc = add i32 %ab, %c
  ret i32 %abc
}
; CHECK-LABEL: define i32 @f_disjoint
; CHECK: load <3 x i32>, ptr addrspace(2)
; CHECK-NOT: load <4 x i32>
; CHECK: shufflevector <3 x i32> {{.*}}, <3 x i32> poison, <2 x i32> <i32 0, i32 1>

; Test that a shufflevector whose mask references the second source vector
; prevents shrinking, even when that source is a constant. The load must
; remain wide. This exercises the in-bounds (m < N) check in
; CheckVectorElementUses for shufflevector users.
define <2 x i32> @f2() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 48
  %ptr = inttoptr i64 %adr to ptr addrspace(2)
  %data = load <4 x i32>, ptr addrspace(2) %ptr, align 16
  ; mask index 4 references the second (constant) source.
  %mix = shufflevector <4 x i32> %data, <4 x i32> zeroinitializer, <2 x i32> <i32 0, i32 4>
  ret <2 x i32> %mix
}
; CHECK-LABEL: define <2 x i32> @f2
; CHECK: load <4 x i32>, ptr addrspace(2)

; Test that undef/poison entries in a shufflevector mask do not block
; shrinking. The mask only reads lanes 0 and 1 from the load; the trailing
; lane is poison and should be ignored.
define <3 x i32> @f3() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 64
  %ptr = inttoptr i64 %adr to ptr addrspace(2)
  %data = load <4 x i32>, ptr addrspace(2) %ptr, align 16
  %lo = shufflevector <4 x i32> %data, <4 x i32> undef, <3 x i32> <i32 0, i32 1, i32 undef>
  ret <3 x i32> %lo
}
; CHECK-LABEL: define <3 x i32> @f3
; CHECK: load <2 x i32>, ptr addrspace(2)
; CHECK-NOT: load <4 x i32>
; CHECK: shufflevector <2 x i32> {{.*}}, <2 x i32> poison, <3 x i32> <i32 0, i32 1, i32 {{(undef|poison)}}>


; Function Attrs: nounwind readnone
declare i64 @llvm.genx.GenISA.RuntimeValue.i64(i32) #0

attributes #0 = { nounwind readnone }

!igc.functions = !{!3, !4, !5, !6, !7}

!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{ptr @f0, !1}
!4 = !{ptr @f1, !1}
!5 = !{ptr @f2, !1}
!6 = !{ptr @f3, !1}
!7 = !{ptr @f_disjoint, !1}
