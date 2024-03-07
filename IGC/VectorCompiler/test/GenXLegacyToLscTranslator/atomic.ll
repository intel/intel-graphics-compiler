;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXLegacyToLscTranslator -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -mattr=+translate_legacy_message -S < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.genx.dword.atomic.inc.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.dec.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)

declare <16 x i32> @llvm.genx.dword.atomic.add.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.sub.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.min.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.max.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.xchg.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.and.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.or.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.xor.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.imin.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic.imax.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)

declare <16 x i32> @llvm.genx.dword.atomic.cmpxchg.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)

declare <16 x i32> @llvm.genx.dword.atomic2.inc.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.dec.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>)

declare <16 x i32> @llvm.genx.dword.atomic2.add.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.sub.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.min.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.max.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.xchg.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.and.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.or.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.xor.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.imin.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.dword.atomic2.imax.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>)

declare <16 x i32> @llvm.genx.dword.atomic2.cmpxchg.v16i32.v16i1.v16i32(<16 x i1>, i32, <16 x i32>, <16 x i32>, <16 x i32>)

declare <16 x i32> @llvm.genx.svm.atomic.inc.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.dec.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>)

declare <16 x i32> @llvm.genx.svm.atomic.add.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.sub.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.min.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.max.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.xchg.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.and.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.or.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.xor.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.imin.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.svm.atomic.imax.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>)

declare <16 x i32> @llvm.genx.svm.atomic.cmpxchg.v16i32.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i32>, <16 x i32>, <16 x i32>)

declare <16 x i64> @llvm.genx.svm.atomic.inc.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.dec.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>)

declare <16 x i64> @llvm.genx.svm.atomic.add.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.sub.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.min.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.max.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.xchg.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.and.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.or.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.xor.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.imin.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)
declare <16 x i64> @llvm.genx.svm.atomic.imax.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>)

declare <16 x i64> @llvm.genx.svm.atomic.cmpxchg.v16i64.v16i1.v16i64(<16 x i1>, <16 x i64>, <16 x i64>, <16 x i64>, <16 x i64>)


; CHECK-LABEL: test_bti
define void @test_bti(<16 x i1> %pred, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %src1, <16 x i32> %passthru) {
  ; CHECK: %inc = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 8, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> %passthru)
  %inc = call <16 x i32> @llvm.genx.dword.atomic.inc.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %passthru)

  ; CHECK: %dec = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 9, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> %passthru)
  %dec = call <16 x i32> @llvm.genx.dword.atomic.dec.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %passthru)

  ; CHECK: %add = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 12, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %add = call <16 x i32> @llvm.genx.dword.atomic.add.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %sub = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 13, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %sub = call <16 x i32> @llvm.genx.dword.atomic.sub.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %min = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 16, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %min = call <16 x i32> @llvm.genx.dword.atomic.min.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %max = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 17, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %max = call <16 x i32> @llvm.genx.dword.atomic.max.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %xchg = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 11, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %xchg = call <16 x i32> @llvm.genx.dword.atomic.xchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %and = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 24, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %and = call <16 x i32> @llvm.genx.dword.atomic.and.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %or = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 25, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %or = call <16 x i32> @llvm.genx.dword.atomic.or.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %xor = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 26, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %xor = call <16 x i32> @llvm.genx.dword.atomic.xor.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %imin = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 14, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %imin = call <16 x i32> @llvm.genx.dword.atomic.imin.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %imax = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 15, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %imax = call <16 x i32> @llvm.genx.dword.atomic.imax.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %cas = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 18, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src1, <16 x i32> %src0, <16 x i32> %passthru)
  %cas = call <16 x i32> @llvm.genx.dword.atomic.cmpxchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %src1, <16 x i32> %passthru)

  ; CHECK: %inc2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 8, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
  %inc2 = call <16 x i32> @llvm.genx.dword.atomic2.inc.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr)

  ; CHECK: %dec2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 9, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
  %dec2 = call <16 x i32> @llvm.genx.dword.atomic2.dec.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr)

  ; CHECK: %add2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 12, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %add2 = call <16 x i32> @llvm.genx.dword.atomic2.add.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %sub2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 13, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %sub2 = call <16 x i32> @llvm.genx.dword.atomic2.sub.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %min2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 16, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %min2 = call <16 x i32> @llvm.genx.dword.atomic2.min.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %max2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 17, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %max2 = call <16 x i32> @llvm.genx.dword.atomic2.max.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %xchg2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 11, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %xchg2 = call <16 x i32> @llvm.genx.dword.atomic2.xchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %and2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 24, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %and2 = call <16 x i32> @llvm.genx.dword.atomic2.and.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %or2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 25, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %or2 = call <16 x i32> @llvm.genx.dword.atomic2.or.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %xor2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 26, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %xor2 = call <16 x i32> @llvm.genx.dword.atomic2.xor.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %imin2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 14, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %imin2 = call <16 x i32> @llvm.genx.dword.atomic2.imin.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %imax2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 15, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %imax2 = call <16 x i32> @llvm.genx.dword.atomic2.imax.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %cas2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 18, i8 2, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src1, <16 x i32> %src0, <16 x i32> undef)
  %cas2 = call <16 x i32> @llvm.genx.dword.atomic2.cmpxchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 123, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %src1)

  ret void
}

; CHECK-LABEL: test_slm
define void @test_slm(<16 x i1> %pred, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %src1, <16 x i32> %passthru) {
  ; CHECK: %inc = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 8, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> %passthru)
  %inc = call <16 x i32> @llvm.genx.dword.atomic.inc.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %passthru)

  ; CHECK: %dec = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 9, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> %passthru)
  %dec = call <16 x i32> @llvm.genx.dword.atomic.dec.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %passthru)

  ; CHECK: %add = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 12, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %add = call <16 x i32> @llvm.genx.dword.atomic.add.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %sub = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 13, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %sub = call <16 x i32> @llvm.genx.dword.atomic.sub.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %min = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 16, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %min = call <16 x i32> @llvm.genx.dword.atomic.min.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %max = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 17, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %max = call <16 x i32> @llvm.genx.dword.atomic.max.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %xchg = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 11, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %xchg = call <16 x i32> @llvm.genx.dword.atomic.xchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %and = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 24, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %and = call <16 x i32> @llvm.genx.dword.atomic.and.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %or = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 25, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %or = call <16 x i32> @llvm.genx.dword.atomic.or.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %xor = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 26, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %xor = call <16 x i32> @llvm.genx.dword.atomic.xor.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %imin = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 14, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %imin = call <16 x i32> @llvm.genx.dword.atomic.imin.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %imax = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 15, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  %imax = call <16 x i32> @llvm.genx.dword.atomic.imax.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %passthru)

  ; CHECK: %cas = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 18, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src1, <16 x i32> %src0, <16 x i32> %passthru)
  %cas = call <16 x i32> @llvm.genx.dword.atomic.cmpxchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %src1, <16 x i32> %passthru)

  ; CHECK: %inc2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 8, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
  %inc2 = call <16 x i32> @llvm.genx.dword.atomic2.inc.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr)

  ; CHECK: %dec2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 9, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
  %dec2 = call <16 x i32> @llvm.genx.dword.atomic2.dec.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr)

  ; CHECK: %add2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 12, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %add2 = call <16 x i32> @llvm.genx.dword.atomic2.add.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %sub2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 13, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %sub2 = call <16 x i32> @llvm.genx.dword.atomic2.sub.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %min2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 16, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %min2 = call <16 x i32> @llvm.genx.dword.atomic2.min.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %max2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 17, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %max2 = call <16 x i32> @llvm.genx.dword.atomic2.max.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %xchg2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 11, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %xchg2 = call <16 x i32> @llvm.genx.dword.atomic2.xchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %and2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 24, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %and2 = call <16 x i32> @llvm.genx.dword.atomic2.and.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %or2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 25, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %or2 = call <16 x i32> @llvm.genx.dword.atomic2.or.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %xor2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 26, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %xor2 = call <16 x i32> @llvm.genx.dword.atomic2.xor.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %imin2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 14, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %imin2 = call <16 x i32> @llvm.genx.dword.atomic2.imin.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %imax2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 15, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> undef)
  %imax2 = call <16 x i32> @llvm.genx.dword.atomic2.imax.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0)

  ; CHECK: %cas2 = call <16 x i32> @llvm.vc.internal.lsc.atomic.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 18, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %src1, <16 x i32> %src0, <16 x i32> undef)
  %cas2 = call <16 x i32> @llvm.genx.dword.atomic2.cmpxchg.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 254, <16 x i32> %addr, <16 x i32> %src0, <16 x i32> %src1)

  ret void
}

; CHECK-LABEL: test_ugm
define void @test_ugm(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %src1, <16 x i32> %passthru) {
  ; CHECK: %inc = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 8, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %dec = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 9, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %add = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 12, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %sub = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 13, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %min = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 16, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %max = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 17, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %xchg = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 11, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %and = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 24, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %or = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 25, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %xor = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 26, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %imin = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 14, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %imax = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 15, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src0, <16 x i32> undef, <16 x i32> %passthru)
  ; CHECK: %cas = call <16 x i32> @llvm.vc.internal.lsc.atomic.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 18, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %src1, <16 x i32> %src0, <16 x i32> %passthru)
  %inc = call <16 x i32> @llvm.genx.svm.atomic.inc.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %passthru)
  %dec = call <16 x i32> @llvm.genx.svm.atomic.dec.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %passthru)
  %add = call <16 x i32> @llvm.genx.svm.atomic.add.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %sub = call <16 x i32> @llvm.genx.svm.atomic.sub.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %min = call <16 x i32> @llvm.genx.svm.atomic.min.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %max = call <16 x i32> @llvm.genx.svm.atomic.max.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %xchg = call <16 x i32> @llvm.genx.svm.atomic.xchg.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %and = call <16 x i32> @llvm.genx.svm.atomic.and.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %or = call <16 x i32> @llvm.genx.svm.atomic.or.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %xor = call <16 x i32> @llvm.genx.svm.atomic.xor.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %imin = call <16 x i32> @llvm.genx.svm.atomic.imin.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %imax = call <16 x i32> @llvm.genx.svm.atomic.imax.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %passthru)
  %cas = call <16 x i32> @llvm.genx.svm.atomic.cmpxchg.v16i32.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %src0, <16 x i32> %src1, <16 x i32> %passthru)

  ret void
}

; CHECK-LABEL: test_ugm64
define void @test_ugm64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %src1, <16 x i64> %passthru) {
  ; CHECK: %inc = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 8, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> undef, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %dec = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 9, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> undef, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %add = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 12, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %sub = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 13, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %min = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 16, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %max = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 17, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %xchg = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 11, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %and = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 24, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %or = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 25, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %xor = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 26, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %imin = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 14, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %imax = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 15, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src0, <16 x i64> undef, <16 x i64> %passthru)
  ; CHECK: %cas = call <16 x i64> @llvm.vc.internal.lsc.atomic.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 18, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %src1, <16 x i64> %src0, <16 x i64> %passthru)
  %inc = call <16 x i64> @llvm.genx.svm.atomic.inc.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %passthru)
  %dec = call <16 x i64> @llvm.genx.svm.atomic.dec.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %passthru)
  %add = call <16 x i64> @llvm.genx.svm.atomic.add.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %sub = call <16 x i64> @llvm.genx.svm.atomic.sub.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %min = call <16 x i64> @llvm.genx.svm.atomic.min.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %max = call <16 x i64> @llvm.genx.svm.atomic.max.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %xchg = call <16 x i64> @llvm.genx.svm.atomic.xchg.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %and = call <16 x i64> @llvm.genx.svm.atomic.and.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %or = call <16 x i64> @llvm.genx.svm.atomic.or.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %xor = call <16 x i64> @llvm.genx.svm.atomic.xor.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %imin = call <16 x i64> @llvm.genx.svm.atomic.imin.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %imax = call <16 x i64> @llvm.genx.svm.atomic.imax.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %passthru)
  %cas = call <16 x i64> @llvm.genx.svm.atomic.cmpxchg.v16i64.v16i1.v16i64(<16 x i1> %pred, <16 x i64> %addr, <16 x i64> %src0, <16 x i64> %src1, <16 x i64> %passthru)

  ret void
}
