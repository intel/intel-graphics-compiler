;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; Test SOALayoutChecker bitcast visitor.
;
; RUN: igc_opt --typed-pointers -igc-priv-mem-to-reg \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S < %s | FileCheck %s

; Struct used as a two-element window into a scalar array.
%Pair = type { float, float }

; float* -> %Pair*: the source pointee is exactly the alloca's leaf scalar, so the
; struct is treated as a window over two consecutive elements. Field 0 of the
; window at element 2 is element 2 of the promoted vector, field 1 is element 3.
;
; CHECK-LABEL: @t_scalar_to_struct(
; CHECK:      alloca <8 x float>
; CHECK-NOT:  alloca [8 x float]
; CHECK:      insertelement <8 x float> %{{.*}}, float %{{.*}}, i32 2
; CHECK:      extractelement <8 x float> %{{.*}}, i32 3
; CHECK:      ret void
define void @t_scalar_to_struct(float %x, float* %out) {
  %p = alloca [8 x float], align 4, !uniform !4
  %g = getelementptr [8 x float], [8 x float]* %p, i32 0, i32 2
  %bc = bitcast float* %g to %Pair*
  %f0 = getelementptr %Pair, %Pair* %bc, i32 0, i32 0
  store float %x, float* %f0, align 4
  %f1 = getelementptr %Pair, %Pair* %bc, i32 0, i32 1
  %v = load float, float* %f1, align 4
  store float %v, float* %out, align 4
  ret void
}

; Neither pointee is a struct and neither is i8, and the two agree neither on
; scalar size (half is 16-bit, i32 is 32-bit) nor on total store size (4 vs 8
; bytes), so the only rule left is the leaf-size fallback: <2 x i32>'s scalar is
; 32 bits, same as the leaf float. The 8-byte load covers elements 2 and 3.
;
; CHECK-LABEL: @t_leaf_size_fallback(
; CHECK:      alloca <8 x float>
; CHECK-NOT:  alloca [8 x float]
; CHECK:      [[E2:%.*]] = extractelement <8 x float> %{{.*}}, i32 2
; CHECK:      [[I2:%.*]] = bitcast float [[E2]] to i32
; CHECK:      [[V0:%.*]] = insertelement <2 x i32> poison, i32 [[I2]], i32 0
; CHECK:      [[E3:%.*]] = extractelement <8 x float> %{{.*}}, i32 3
; CHECK:      [[I3:%.*]] = bitcast float [[E3]] to i32
; CHECK:      insertelement <2 x i32> [[V0]], i32 [[I3]], i32 1
; CHECK:      ret void
define void @t_leaf_size_fallback(float %x, <2 x i32>* %out) {
  %p = alloca [8 x float], align 4, !uniform !4
  %g0 = getelementptr [8 x float], [8 x float]* %p, i32 0, i32 0
  store float %x, float* %g0, align 4
  %g = getelementptr [8 x float], [8 x float]* %p, i32 0, i32 2
  %h = bitcast float* %g to <2 x half>*
  %w = bitcast <2 x half>* %h to <2 x i32>*
  %v = load <2 x i32>, <2 x i32>* %w, align 4
  store <2 x i32> %v, <2 x i32>* %out, align 4
  ret void
}

; Negative case: the destination scalar (i16, 16 bits) does not match the leaf
; scalar, and no other rule applies, so the alloca stays in memory.
;
; CHECK-LABEL: @t_leaf_size_mismatch(
; CHECK:      alloca [8 x float]
; CHECK-NOT:  alloca <8 x float>
; CHECK:      ret void
define void @t_leaf_size_mismatch(float %x, <2 x i16>* %out) {
  %p = alloca [8 x float], align 4, !uniform !4
  %g0 = getelementptr [8 x float], [8 x float]* %p, i32 0, i32 0
  store float %x, float* %g0, align 4
  %g = getelementptr [8 x float], [8 x float]* %p, i32 0, i32 2
  %h = bitcast float* %g to <2 x half>*
  %w = bitcast <2 x half>* %h to <4 x i16>*
  %v = load <4 x i16>, <4 x i16>* %w, align 4
  %s = shufflevector <4 x i16> %v, <4 x i16> poison, <2 x i32> <i32 0, i32 1>
  store <2 x i16> %s, <2 x i16>* %out, align 4
  ret void
}

!igc.functions = !{!5, !6, !7}

!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
!5 = !{void (float, float*)* @t_scalar_to_struct, !1}
!6 = !{void (float, <2 x i32>*)* @t_leaf_size_fallback, !1}
!7 = !{void (float, <2 x i16>*)* @t_leaf_size_mismatch, !1}
