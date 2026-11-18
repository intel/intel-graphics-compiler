;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s \
; RUN:   --implicit-check-not=error: --implicit-check-not=@caller( --implicit-check-not=@callee( \
; RUN:   --implicit-check-not=@recursive( --implicit-check-not=@ping( --implicit-check-not=@pong(
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - calls between functions with matrix arguments
; resolve regardless of definition order, including self and mutual recursion.
; ------------------------------------------------

%acc = type target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2)

define spir_kernel void @kernel(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK:         call spir_func <16 x float> @caller_resolved(<16 x float>
; CHECK:         call spir_func <16 x float> @recursive_resolved(<16 x float>
; CHECK:         call spir_func <16 x float> @ping_resolved(<16 x float>
  %a = call spir_func %acc @_Z32__spirv_CooperativeMatrixLoadKHRPU3AS1fil(ptr addrspace(1) %src, i32 0, i64 16)
  %b = call spir_func %acc @caller(%acc %a)
  %c = call spir_func %acc @recursive(%acc %b, i32 3)
  %d = call spir_func %acc @ping(%acc %c, i32 3)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fil(ptr addrspace(1) %dst, %acc %d, i32 0, i64 16)
  ret void
}

; The callee is defined after the caller.
define spir_func %acc @caller(%acc %m) {
  %r = call spir_func %acc @callee(%acc %m)
  ret %acc %r
}

define spir_func %acc @callee(%acc %m) {
  ret %acc %m
}

define spir_func %acc @recursive(%acc %m, i32 %n) {
  %cmp = icmp eq i32 %n, 0
  br i1 %cmp, label %done, label %rec
rec:
  %n1 = sub i32 %n, 1
  %r = call spir_func %acc @recursive(%acc %m, i32 %n1)
  ret %acc %r
done:
  ret %acc %m
}

define spir_func %acc @ping(%acc %m, i32 %n) {
  %cmp = icmp eq i32 %n, 0
  br i1 %cmp, label %done, label %rec
rec:
  %n1 = sub i32 %n, 1
  %r = call spir_func %acc @pong(%acc %m, i32 %n1)
  ret %acc %r
done:
  ret %acc %m
}

define spir_func %acc @pong(%acc %m, i32 %n) {
  %r = call spir_func %acc @ping(%acc %m, i32 %n)
  ret %acc %r
}

; CHECK-LABEL: define spir_func <16 x float> @caller_resolved(<16 x float> %m) {
; CHECK-NEXT:    [[R:%.*]] = call spir_func <16 x float> @callee_resolved(<16 x float> %m)
; CHECK-NEXT:    ret <16 x float> [[R]]

; CHECK-LABEL: define spir_func <16 x float> @callee_resolved(<16 x float> %m) {
; CHECK-NEXT:    ret <16 x float> %m

; CHECK-LABEL: define spir_func <16 x float> @recursive_resolved(<16 x float> %m, i32 %n) {
; CHECK:       rec:
; CHECK:         [[R:%.*]] = call spir_func <16 x float> @recursive_resolved(<16 x float> %m, i32 %n1)
; CHECK-NEXT:    ret <16 x float> [[R]]
; CHECK:       done:
; CHECK-NEXT:    ret <16 x float> %m

; CHECK-LABEL: define spir_func <16 x float> @ping_resolved(<16 x float> %m, i32 %n) {
; CHECK:       rec:
; CHECK:         [[R:%.*]] = call spir_func <16 x float> @pong_resolved(<16 x float> %m, i32 %n1)
; CHECK-NEXT:    ret <16 x float> [[R]]
; CHECK:       done:
; CHECK-NEXT:    ret <16 x float> %m

; CHECK-LABEL: define spir_func <16 x float> @pong_resolved(<16 x float> %m, i32 %n) {
; CHECK-NEXT:    [[R:%.*]] = call spir_func <16 x float> @ping_resolved(<16 x float> %m, i32 %n)
; CHECK-NEXT:    ret <16 x float> [[R]]

declare spir_func %acc @_Z32__spirv_CooperativeMatrixLoadKHRPU3AS1fil(ptr addrspace(1), i32, i64)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fil(ptr addrspace(1), %acc, i32, i64)

!igc.functions = !{!0}
!0 = !{ptr @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
