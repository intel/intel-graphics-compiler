;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

; m = as_int8(a * b), out[0] = m - m.s12345670.
; The bitcast slice is vectorized first. The first sub operand is the same
; slice in lane order, so the vectorized bitcast is reused as is. The second
; operand is the same scalars rotated by one lane, so it has to be rebuilt
; from extracts of the vectorized bitcast instead of reusing it, otherwise
; the result is m - m.

; CHECK-LABEL: @k(
; CHECK: %vectorized_binary = fmul <8 x float> %0, %1
; CHECK-NEXT: %vectorized_cast = bitcast <8 x float> %vectorized_binary to <8 x i32>
; CHECK-NEXT: [[E0:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 0
; CHECK-NEXT: [[E1:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 1
; CHECK-NEXT: [[E2:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 2
; CHECK-NEXT: [[E3:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 3
; CHECK-NEXT: [[E4:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 4
; CHECK-NEXT: [[E5:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 5
; CHECK-NEXT: [[E6:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 6
; CHECK-NEXT: [[E7:%vector_extract[0-9]*]] = extractelement <8 x i32> %vectorized_cast, i32 7
; CHECK-NEXT: [[V0:%vector[0-9]*]] = insertelement <8 x i32> undef, i32 [[E1]], i32 0
; CHECK-NEXT: [[V1:%vector[0-9]*]] = insertelement <8 x i32> [[V0]], i32 [[E2]], i32 1
; CHECK-NEXT: [[V2:%vector[0-9]*]] = insertelement <8 x i32> [[V1]], i32 [[E3]], i32 2
; CHECK-NEXT: [[V3:%vector[0-9]*]] = insertelement <8 x i32> [[V2]], i32 [[E4]], i32 3
; CHECK-NEXT: [[V4:%vector[0-9]*]] = insertelement <8 x i32> [[V3]], i32 [[E5]], i32 4
; CHECK-NEXT: [[V5:%vector[0-9]*]] = insertelement <8 x i32> [[V4]], i32 [[E6]], i32 5
; CHECK-NEXT: [[V6:%vector[0-9]*]] = insertelement <8 x i32> [[V5]], i32 [[E7]], i32 6
; CHECK-NEXT: [[V7:%vector[0-9]*]] = insertelement <8 x i32> [[V6]], i32 [[E0]], i32 7
; CHECK-NEXT: [[SUB:%vectorized_binary[0-9]+]] = sub <8 x i32> %vectorized_cast, [[V7]]
; CHECK-NEXT: store <8 x i32> [[SUB]], ptr addrspace(1) %out, align 32

define spir_kernel void @k(ptr addrspace(1) align 32 %a, ptr addrspace(1) align 32 %b, ptr addrspace(1) align 32 %out) {
entry:
  %0 = load <8 x float>, ptr addrspace(1) %a, align 32
  %.scalar = extractelement <8 x float> %0, i64 0
  %.scalar5 = extractelement <8 x float> %0, i64 1
  %.scalar6 = extractelement <8 x float> %0, i64 2
  %.scalar7 = extractelement <8 x float> %0, i64 3
  %.scalar8 = extractelement <8 x float> %0, i64 4
  %.scalar9 = extractelement <8 x float> %0, i64 5
  %.scalar10 = extractelement <8 x float> %0, i64 6
  %.scalar11 = extractelement <8 x float> %0, i64 7
  %1 = load <8 x float>, ptr addrspace(1) %b, align 32
  %.scalar12 = extractelement <8 x float> %1, i64 0
  %.scalar13 = extractelement <8 x float> %1, i64 1
  %.scalar14 = extractelement <8 x float> %1, i64 2
  %.scalar15 = extractelement <8 x float> %1, i64 3
  %.scalar16 = extractelement <8 x float> %1, i64 4
  %.scalar17 = extractelement <8 x float> %1, i64 5
  %.scalar18 = extractelement <8 x float> %1, i64 6
  %.scalar19 = extractelement <8 x float> %1, i64 7
  %mul.i20 = fmul float %.scalar, %.scalar12
  %mul.i21 = fmul float %.scalar5, %.scalar13
  %mul.i22 = fmul float %.scalar6, %.scalar14
  %mul.i23 = fmul float %.scalar7, %.scalar15
  %mul.i24 = fmul float %.scalar8, %.scalar16
  %mul.i25 = fmul float %.scalar9, %.scalar17
  %mul.i26 = fmul float %.scalar10, %.scalar18
  %mul.i27 = fmul float %.scalar11, %.scalar19
  %astype.i28 = bitcast float %mul.i20 to i32
  %astype.i29 = bitcast float %mul.i21 to i32
  %astype.i30 = bitcast float %mul.i22 to i32
  %astype.i31 = bitcast float %mul.i23 to i32
  %astype.i32 = bitcast float %mul.i24 to i32
  %astype.i33 = bitcast float %mul.i25 to i32
  %astype.i34 = bitcast float %mul.i26 to i32
  %astype.i35 = bitcast float %mul.i27 to i32
  %sub.i36 = sub i32 %astype.i28, %astype.i29
  %sub.i37 = sub i32 %astype.i29, %astype.i30
  %sub.i38 = sub i32 %astype.i30, %astype.i31
  %sub.i39 = sub i32 %astype.i31, %astype.i32
  %sub.i40 = sub i32 %astype.i32, %astype.i33
  %sub.i41 = sub i32 %astype.i33, %astype.i34
  %sub.i42 = sub i32 %astype.i34, %astype.i35
  %sub.i43 = sub i32 %astype.i35, %astype.i28
  %sub.i.assembled.vect = insertelement <8 x i32> poison, i32 %sub.i36, i64 0
  %sub.i.assembled.vect44 = insertelement <8 x i32> %sub.i.assembled.vect, i32 %sub.i37, i64 1
  %sub.i.assembled.vect45 = insertelement <8 x i32> %sub.i.assembled.vect44, i32 %sub.i38, i64 2
  %sub.i.assembled.vect46 = insertelement <8 x i32> %sub.i.assembled.vect45, i32 %sub.i39, i64 3
  %sub.i.assembled.vect47 = insertelement <8 x i32> %sub.i.assembled.vect46, i32 %sub.i40, i64 4
  %sub.i.assembled.vect48 = insertelement <8 x i32> %sub.i.assembled.vect47, i32 %sub.i41, i64 5
  %sub.i.assembled.vect49 = insertelement <8 x i32> %sub.i.assembled.vect48, i32 %sub.i42, i64 6
  %sub.i.assembled.vect50 = insertelement <8 x i32> %sub.i.assembled.vect49, i32 %sub.i43, i64 7
  store <8 x i32> %sub.i.assembled.vect50, ptr addrspace(1) %out, align 32
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @k}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
