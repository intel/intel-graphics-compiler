;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-loop-split-wide-phis -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LoopSplitWidePHIs
; ------------------------------------------------

; Check the basic functionality of the LoopSplitWidePHIs pass for PHIs that are
; split into > 2 parts.
; The <32 x i64> PHI, being formed by a concat of <8 x float> parts bitcast to
; i64 vector types, and used only in <8 x float> parts bitcast from i64 vector
; types, will be split into four <8 x float> PHIs which directly feed and use
; the result of the chain of dpas instructions.

define spir_kernel void @test_basic_4way() {
ph:
  br label %loop

; CHECK-LABEL: loop
; CHECK:   [[TMP0:%split.*]] = phi <8 x float> [ [[INIT:%.*]], %ph ], [ [[TMP8:%dpas.*]], %loop ]
; CHECK:   [[TMP1:%dpas.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP0]]
; CHECK:   [[TMP2:%dpas.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP1]]
; CHECK:   [[TMP3:%dpas.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP2]]
; CHECK:   [[TMP4:%dpas.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP3]]
; CHECK:   [[TMP5:%dpas.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP4]]
; CHECK:   [[TMP6:%dpas.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP5]]
; CHECK:   [[TMP7:%dpas.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP6]]
; CHECK:   [[TMP8]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[TMP7]]

loop:
  %.sroa.5.448.vec.assembled.vect1393.lcssa14471451 = phi <32 x i64> [ zeroinitializer, %ph ], [ %.sroa.5.448.vec.assembled.vect1393.1.3, %loop ]
  %.sroa.5204.256.vec.extract.assembled.vect1038 = shufflevector <32 x i64> %.sroa.5.448.vec.assembled.vect1393.lcssa14471451, <32 x i64> zeroinitializer, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %0 = bitcast <8 x i64> %.sroa.5204.256.vec.extract.assembled.vect1038 to <16 x float>
  %vecinit25.i79.i.assembled.vect1045 = shufflevector <16 x float> %0, <16 x float> zeroinitializer, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %dpas331 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vecinit25.i79.i.assembled.vect1045, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas331.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas331, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas331.11494 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas331.1, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas331.1.1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas331.11494, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas331.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas331.1.1, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas331.1.2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas331.2, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas331.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas331.1.2, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas331.1.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas331.3, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas332.1.3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %vecinit52.uncasted.i83.i.assembled.vect1081.1.3 = shufflevector <8 x float> %dpas331.1.3, <8 x float> %dpas332.1.3, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %1 = bitcast <16 x float> %vecinit52.uncasted.i83.i.assembled.vect1081.1.3 to <8 x i64>
  %2 = bitcast <16 x float> zeroinitializer to <8 x i64>
  %3 = shufflevector <8 x i64> %2, <8 x i64> zeroinitializer, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %4 = bitcast <16 x float> zeroinitializer to <8 x i64>
  %5 = bitcast <16 x float> zeroinitializer to <8 x i64>
  %6 = shufflevector <8 x i64> %5, <8 x i64> zeroinitializer, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %.sroa.5.448.vec.assembled.vect1377.1.3 = shufflevector <8 x i64> %1, <8 x i64> %4, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %.sroa.5.448.vec.assembled.vect1385.1.3 = shufflevector <32 x i64> %.sroa.5.448.vec.assembled.vect1377.1.3, <32 x i64> %3, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %.sroa.5.448.vec.assembled.vect1393.1.3 = shufflevector <32 x i64> %.sroa.5.448.vec.assembled.vect1385.1.3, <32 x i64> %6, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39>
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
