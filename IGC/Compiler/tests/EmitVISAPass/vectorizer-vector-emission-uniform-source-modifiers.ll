;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -dce -platformbmg -rev-id B -simd-mode 16 -igc-emit-visa --regkey=DumpVISAASMToConsole=1 < %s | FileCheck %s

; When all sources and the destination are uniform, vector emission writes the
; whole vector with one instruction instead of one instruction per element. The
; source modifiers that pattern matching folds into the sources (the negation
; that MatchFMad moves from an fsub to a mad source, fneg, fabs) must stay on
; that instruction.

define spir_kernel void @uniform_vector_modifiers(<8 x float> %a, <8 x float> %b, <8 x float> %c, <32 x float> %x, <32 x float> %y, <8 x i32> %i, <8 x i32> %j) {
; a * b - c: MatchFMad puts the negation on the addend.
; CHECK: mad (M1_NM, 8) mad_sub_addend(0,0)<1> b(0,0)<1;1,0> a(0,0)<1;1,0> (-)c(0,0)<1;1,0>
  %mul_addend = fmul fast <8 x float> %a, %b
  %mad_sub_addend = fsub fast <8 x float> %mul_addend, %c

; a - b * c: MatchFMad puts the negation on a factor.
; CHECK: mad (M1_NM, 8) mad_sub_factor(0,0)<1> (-)c(0,0)<1;1,0> b(0,0)<1;1,0> a(0,0)<1;1,0>
  %mul_factor = fmul fast <8 x float> %b, %c
  %mad_sub_factor = fsub fast <8 x float> %a, %mul_factor

; |a| * b + c
; CHECK: mad (M1_NM, 8) mad_abs(0,0)<1> b(0,0)<1;1,0> (abs)a(0,0)<1;1,0> c(0,0)<1;1,0>
  %abs_mad = call <8 x float> @llvm.fabs.v8f32(<8 x float> %a)
  %mul_abs = fmul fast <8 x float> %abs_mad, %b
  %mad_abs = fadd fast <8 x float> %mul_abs, %c

; -a + b
; CHECK: add (M1_NM, 8) add_neg(0,0)<1> (-)a(0,0)<1;1,0> b(0,0)<1;1,0>
  %neg_add = fneg <8 x float> %a
  %add_neg = fadd fast <8 x float> %neg_add, %b

; -a - b
; CHECK: add (M1_NM, 8) sub_neg(0,0)<1> (-)a(0,0)<1;1,0> (-)b(0,0)<1;1,0>
  %neg_sub = fneg <8 x float> %a
  %sub_neg = fsub fast <8 x float> %neg_sub, %b

; |a| * b
; CHECK: mul (M1_NM, 8) mul_abs2(0,0)<1> (abs)a(0,0)<1;1,0> b(0,0)<1;1,0>
  %abs_mul = call <8 x float> @llvm.fabs.v8f32(<8 x float> %a)
  %mul_abs2 = fmul fast <8 x float> %abs_mul, %b

; maxnum(-a, b)
; CHECK: max (M1_NM, 8) max_neg(0,0)<1> (-)a(0,0)<1;1,0> b(0,0)<1;1,0>
  %neg_max = fneg <8 x float> %a
  %max_neg = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %neg_max, <8 x float> %b)

; exp2(-c)
; CHECK: exp (M1_NM, 8) exp_neg(0,0)<1> (-)c(0,0)<1;1,0>
  %neg_exp = fneg <8 x float> %c
  %exp_neg = call <8 x float> @llvm.exp2.v8f32(<8 x float> %neg_exp)

; -x + y on 32 elements: split into two SIMD16 instructions.
; CHECK: add (M1_NM, 16) add_split(0,0)<1> (-)x(0,0)<1;1,0> y(0,0)<1;1,0>
; CHECK: add (M1_NM, 16) add_split(1,0)<1> (-)x(1,0)<1;1,0> y(1,0)<1;1,0>
  %neg_split = fneg <32 x float> %x
  %add_split = fadd fast <32 x float> %neg_split, %y

; |i| + j on integers.
; CHECK: add (M1_NM, 8) add_int_abs(0,0)<1> (abs)i(0,0)<1;1,0> j(0,0)<1;1,0>
  %abs_int = call <8 x i32> @llvm.abs.v8i32(<8 x i32> %i, i1 false)
  %add_int_abs = add <8 x i32> %abs_int, %j

  %st_mad_sub_addend = bitcast <8 x float> %mad_sub_addend to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_mad_sub_addend)
  %st_mad_sub_factor = bitcast <8 x float> %mad_sub_factor to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_mad_sub_factor)
  %st_mad_abs = bitcast <8 x float> %mad_abs to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_mad_abs)
  %st_add_neg = bitcast <8 x float> %add_neg to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_add_neg)
  %st_sub_neg = bitcast <8 x float> %sub_neg to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_sub_neg)
  %st_mul_abs2 = bitcast <8 x float> %mul_abs2 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_mul_abs2)
  %st_max_neg = bitcast <8 x float> %max_neg to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_max_neg)
  %st_exp_neg = bitcast <8 x float> %exp_neg to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %st_exp_neg)
  %st_add_split = bitcast <32 x float> %add_split to <32 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v32i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <32 x i32> %st_add_split)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 1, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %add_int_abs)
  ret void
}

declare <8 x float> @llvm.fabs.v8f32(<8 x float>)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>)
declare <8 x float> @llvm.exp2.v8f32(<8 x float>)
declare <8 x i32> @llvm.abs.v8i32(<8 x i32>, i1)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v32i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <32 x i32>)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @uniform_vector_modifiers, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @uniform_vector_modifiers}
!6 = !{!"FuncMDValue[0]", !7, !20}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !13, !14, !15, !16, !17, !18}
!9 = !{!"argAllocMDListVec[0]", !10, !11, !12}
!10 = !{!"type", i32 0}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"indexType", i32 -1}
!13 = !{!"argAllocMDListVec[1]", !10, !11, !12}
!14 = !{!"argAllocMDListVec[2]", !10, !11, !12}
!15 = !{!"argAllocMDListVec[3]", !10, !11, !12}
!16 = !{!"argAllocMDListVec[4]", !10, !11, !12}
!17 = !{!"argAllocMDListVec[5]", !10, !11, !12}
!18 = !{!"argAllocMDListVec[6]", !10, !11, !12}
!20 = !{!"requiredSubGroupSize", i32 16}
