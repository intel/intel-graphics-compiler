;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------
; Vectors wider than 16 elements (SPV_EXT_long_vector) are passed as "byval"
; pointers and returned through an "sret" pointer. Vectors of up to 16
; elements are still passed and returned in registers.

define spir_kernel void @test_k(ptr %p, ptr %q) {
; CHECK-LABEL: define spir_kernel void @test_k(
; CHECK:         [[X:%.*]] = load <17 x i32>, ptr %p
; CHECK:         [[Y:%.*]] = load <16 x i32>, ptr %q
; CHECK:         [[RET:%.*]] = alloca <17 x i32>
; CHECK:         [[ARG:%.*]] = alloca <17 x i32>
; CHECK:         store <17 x i32> [[X]], ptr [[ARG]]
; CHECK:         call void @long_vector(ptr noalias sret(<17 x i32>) [[RET]], ptr byval(<17 x i32>) [[ARG]], <16 x i32> [[Y]])
; CHECK:         [[R:%.*]] = load <17 x i32>, ptr [[RET]]
; CHECK:         store <17 x i32> [[R]], ptr %p
; CHECK:         [[S:%.*]] = call <16 x i32> @short_vector(<16 x i32> [[Y]])
; CHECK:         store <16 x i32> [[S]], ptr %q
  %x = load <17 x i32>, ptr %p
  %y = load <16 x i32>, ptr %q
  %r = call <17 x i32> @long_vector(<17 x i32> %x, <16 x i32> %y)
  store <17 x i32> %r, ptr %p
  %s = call <16 x i32> @short_vector(<16 x i32> %y)
  store <16 x i32> %s, ptr %q
  ret void
}

; CHECK-LABEL: define spir_func <16 x i32> @short_vector(<16 x i32> %y)
; CHECK-NEXT:    ret <16 x i32> %y
define spir_func <16 x i32> @short_vector(<16 x i32> %y) {
  ret <16 x i32> %y
}

; CHECK-LABEL: define spir_func void @long_vector(
; CHECK-SAME:    ptr noalias sret(<17 x i32>) [[SRET:%.*]], ptr %x, <16 x i32> %y)
; CHECK:         [[X:%.*]] = load <17 x i32>, ptr %x
; CHECK:         [[E:%.*]] = extractelement <16 x i32> %y, i32 0
; CHECK:         [[R:%.*]] = insertelement <17 x i32> [[X]], i32 [[E]], i32 16
; CHECK:         [[TMP:%.*]] = alloca <17 x i32>
; CHECK:         store <17 x i32> [[R]], ptr [[TMP]]
; CHECK:         call void @llvm.memcpy.p0.p0.i64(ptr align 8 [[SRET]], ptr align 8 [[TMP]], i64 128, i1 false)
; CHECK:         ret void
define spir_func <17 x i32> @long_vector(<17 x i32> %x, <16 x i32> %y) {
  %e = extractelement <16 x i32> %y, i32 0
  %r = insertelement <17 x i32> %x, i32 %e, i32 16
  ret <17 x i32> %r
}
