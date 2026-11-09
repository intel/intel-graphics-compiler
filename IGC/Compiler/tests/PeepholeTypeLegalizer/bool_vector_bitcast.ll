;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S < %s | FileCheck %s

; Bitcasts of bool vectors not built purely from insertelements into an integer.

target datalayout = "n8:16:32:64"

; A vector compare's lanes must each land in their own bit, since codegen can't emit the bitcast.
define i1 @any_of_v8(<8 x float> %a, <8 x float> %b) {
; CHECK-LABEL: @any_of_v8(
; CHECK-NEXT:    %cmp = fcmp une <8 x float> %a, %b
; CHECK-NEXT:    [[BITS:%.*]] = select <8 x i1> %cmp, <8 x i8> <i8 1, i8 2, i8 4, i8 8, i8 16, i8 32, i8 64, i8 -128>, <8 x i8> zeroinitializer
; CHECK-NEXT:    [[E0:%.*]] = extractelement <8 x i8> [[BITS]], i32 0
; CHECK-NEXT:    [[E1:%.*]] = extractelement <8 x i8> [[BITS]], i32 1
; CHECK-NEXT:    [[O1:%.*]] = or i8 [[E0]], [[E1]]
; CHECK-NEXT:    [[E2:%.*]] = extractelement <8 x i8> [[BITS]], i32 2
; CHECK-NEXT:    [[O2:%.*]] = or i8 [[O1]], [[E2]]
; CHECK-NEXT:    [[E3:%.*]] = extractelement <8 x i8> [[BITS]], i32 3
; CHECK-NEXT:    [[O3:%.*]] = or i8 [[O2]], [[E3]]
; CHECK-NEXT:    [[E4:%.*]] = extractelement <8 x i8> [[BITS]], i32 4
; CHECK-NEXT:    [[O4:%.*]] = or i8 [[O3]], [[E4]]
; CHECK-NEXT:    [[E5:%.*]] = extractelement <8 x i8> [[BITS]], i32 5
; CHECK-NEXT:    [[O5:%.*]] = or i8 [[O4]], [[E5]]
; CHECK-NEXT:    [[E6:%.*]] = extractelement <8 x i8> [[BITS]], i32 6
; CHECK-NEXT:    [[O6:%.*]] = or i8 [[O5]], [[E6]]
; CHECK-NEXT:    [[E7:%.*]] = extractelement <8 x i8> [[BITS]], i32 7
; CHECK-NEXT:    [[O7:%.*]] = or i8 [[O6]], [[E7]]
; CHECK-NEXT:    %res = icmp ne i8 [[O7]], 0
; CHECK-NEXT:    ret i1 %res
  %cmp = fcmp une <8 x float> %a, %b
  %packed = bitcast <8 x i1> %cmp to i8
  %res = icmp ne i8 %packed, 0
  ret i1 %res
}

; Wider vectors pack into the matching wider integer, lane i into bit i.
define i16 @pack_v16(<16 x i32> %a) {
; CHECK-LABEL: @pack_v16(
; CHECK-NEXT:    %cmp = icmp eq <16 x i32> %a, zeroinitializer
; CHECK-NEXT:    [[BITS:%.*]] = select <16 x i1> %cmp, <16 x i16> <i16 1, i16 2, i16 4, i16 8, i16 16, i16 32, i16 64, i16 128, i16 256, i16 512, i16 1024, i16 2048, i16 4096, i16 8192, i16 16384, i16 -32768>, <16 x i16> zeroinitializer
; CHECK:         [[E15:%.*]] = extractelement <16 x i16> [[BITS]], i32 15
; CHECK-NEXT:    [[O15:%.*]] = or i16 {{%.*}}, [[E15]]
; CHECK-NEXT:    ret i16 [[O15]]
  %cmp = icmp eq <16 x i32> %a, zeroinitializer
  %packed = bitcast <16 x i1> %cmp to i16
  ret i16 %packed
}

; An insertelement chain on a non-undef vector must pack the whole final vector, not crash on the chain's base.
define i8 @insert_into_cmp(<8 x float> %a, <8 x float> %b, i1 %c) {
; CHECK-LABEL: @insert_into_cmp(
; CHECK-NEXT:    %cmp = fcmp une <8 x float> %a, %b
; CHECK-NEXT:    %v = insertelement <8 x i1> %cmp, i1 %c, i32 0
; CHECK-NEXT:    [[BITS:%.*]] = select <8 x i1> %v, <8 x i8> <i8 1, i8 2, i8 4, i8 8, i8 16, i8 32, i8 64, i8 -128>, <8 x i8> zeroinitializer
; CHECK:         [[E7:%.*]] = extractelement <8 x i8> [[BITS]], i32 7
; CHECK-NEXT:    [[O7:%.*]] = or i8 {{%.*}}, [[E7]]
; CHECK-NEXT:    ret i8 [[O7]]
  %cmp = fcmp une <8 x float> %a, %b
  %v = insertelement <8 x i1> %cmp, i1 %c, i32 0
  %packed = bitcast <8 x i1> %v to i8
  ret i8 %packed
}
