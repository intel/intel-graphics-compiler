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

; Check the basic functionality of the LoopSplitWidePHIs pass.
; The <16 x float> PHI, being formed by a concat of <8 x float> parts
; and used only in <8 x float> parts will be split into two <8 x float> PHIs.

define spir_kernel void @test_basic() {
preheader:
  br label %loop

; CHECK-LABEL: loop
; CHECK-DAG:  [[PHI0:%split.*]] = phi <8 x float> [ [[TMP3:%.*]], %preheader ], [ [[DPAS0:%.*]], %loop ]
; CHECK-DAG:  [[PHI1:%split.*]] = phi <8 x float> [ [[TMP4:%.*]], %preheader ], [ [[DPAS1:%.*]], %loop ]
; CHECK-DAG:  [[DPAS0]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[PHI0]]
; CHECK-DAG:  [[DPAS1]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[PHI1]]
loop: ; preds = %loop, %preheader
  %.sroa.20546.0 = phi <16 x float> [ zeroinitializer, %preheader ], [ %vecjoin, %loop ]
  %vecpart.1 = shufflevector <16 x float> %.sroa.20546.0, <16 x float> zeroinitializer, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %vecpart.2 = shufflevector <16 x float> %.sroa.20546.0, <16 x float> zeroinitializer, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vecpart.1, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas64 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vecpart.2, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %vecjoin = shufflevector <8 x float> %dpas, <8 x float> %dpas64, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

