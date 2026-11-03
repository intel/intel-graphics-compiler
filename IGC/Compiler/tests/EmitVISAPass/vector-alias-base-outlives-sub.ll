;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=1,VectorAliasBaseOutlivesSub=1 | FileCheck %s --check-prefixes=CHECK,CHECK-ON
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=1,VectorAliasBaseOutlivesSub=0 | FileCheck %s --check-prefixes=CHECK,CHECK-OFF

; VectorAliasBaseOutlivesSub: sub-vector aliasing (extractFrom) is allowed
; without a target (dpas, load, store, ...) if the base vector is live wherever
; the sub-vector is live. In all kernels below, both the base (%exp, defined by
; llvm.exp2) and the sub-vector (%sub = %exp[8..15]) are only defined and used
; by ALU instructions. With the flag off, the sub-vector is always copied.

; Positive: %exp (base) is used by the fadd after the last use of %sub, so
; %exp is live wherever %sub is live. %sub is aliased to %exp(8..15) and the
; second exp2 reads %exp directly.
; CHECK-LABEL:     .kernel "base_outlives"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
;
; CHECK-OFF-NEXT:  mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[SUB]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](1,0)<1> [[SUB]](1,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](2,0)<1> [[SUB]](2,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](3,0)<1> [[SUB]](3,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](4,0)<1> [[SUB]](4,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](5,0)<1> [[SUB]](5,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](6,0)<1> [[SUB]](6,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](7,0)<1> [[SUB]](7,0)<1;1,0>
;
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](1,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](2,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](3,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](4,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](5,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](6,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](7,0)<1> [[EXP]](15,0)<1;1,0>
;
; CHECK-NEXT:      add (M1, 16) [[ADD:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[SEXP]]:d32.16x8nn
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ADD]]:d32.16x16nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK-OFF:       .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @base_outlives(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %blk)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %sexp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %sub)
  %add = fadd <16 x float> %exp, %blk
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %sexp)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  ret void
}

; Negative: %sub is used after the last use of %exp, so aliasing would make
; %exp live longer. %sub is copied.
; CHECK-LABEL:     .kernel "sub_outlives"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ADD]]:d32.16x16nn
; CHECK-NEXT:      exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[SUB]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](1,0)<1> [[SUB]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](2,0)<1> [[SUB]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](3,0)<1> [[SUB]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](4,0)<1> [[SUB]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](5,0)<1> [[SUB]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](6,0)<1> [[SUB]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](7,0)<1> [[SUB]](7,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[SEXP]]:d32.16x8nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK:           .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @sub_outlives(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %blk)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %add = fadd <16 x float> %exp, %blk
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  %sexp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %sub)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %sexp)
  ret void
}

; Positive, loop: %sub is used in the loop, and %exp is used after the loop.
; %exp is live through the loop, so %sub is aliased to %exp(8..15).
; CHECK-LABEL:     .kernel "loop_base_outlives"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
;
; CHECK-OFF-NEXT:  mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
;
; CHECK-NEXT:      mov (M1_NM, 1) [[I:[A-Za-z0-9_]+]](0,0)<1> 0x0:d
; CHECK-NEXT:      mov (M1, 16) [[ACC:[A-Za-z0-9_]+]](0,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](1,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](2,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](3,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](4,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](5,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](6,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](7,0)<1> 0x0:f
; CHECK:           {{^}}[[LOOP:[A-Za-z0-9_]+]]:
;
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](0,0)<1> [[ACC]](0,0)<1;1,0> [[SUB]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](1,0)<1> [[ACC]](1,0)<1;1,0> [[SUB]](1,0)<1;1,0>
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](2,0)<1> [[ACC]](2,0)<1;1,0> [[SUB]](2,0)<1;1,0>
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](3,0)<1> [[ACC]](3,0)<1;1,0> [[SUB]](3,0)<1;1,0>
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](4,0)<1> [[ACC]](4,0)<1;1,0> [[SUB]](4,0)<1;1,0>
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](5,0)<1> [[ACC]](5,0)<1;1,0> [[SUB]](5,0)<1;1,0>
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](6,0)<1> [[ACC]](6,0)<1;1,0> [[SUB]](6,0)<1;1,0>
; CHECK-OFF-NEXT:  add (M1, 16) [[ACC]](7,0)<1> [[ACC]](7,0)<1;1,0> [[SUB]](7,0)<1;1,0>
;
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](0,0)<1> [[ACC]](0,0)<1;1,0> [[EXP]](8,0)<1;1,0>
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](1,0)<1> [[ACC]](1,0)<1;1,0> [[EXP]](9,0)<1;1,0>
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](2,0)<1> [[ACC]](2,0)<1;1,0> [[EXP]](10,0)<1;1,0>
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](3,0)<1> [[ACC]](3,0)<1;1,0> [[EXP]](11,0)<1;1,0>
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](4,0)<1> [[ACC]](4,0)<1;1,0> [[EXP]](12,0)<1;1,0>
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](5,0)<1> [[ACC]](5,0)<1;1,0> [[EXP]](13,0)<1;1,0>
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](6,0)<1> [[ACC]](6,0)<1;1,0> [[EXP]](14,0)<1;1,0>
; CHECK-ON-NEXT:   add (M1, 16) [[ACC]](7,0)<1> [[ACC]](7,0)<1;1,0> [[EXP]](15,0)<1;1,0>
;
; CHECK-NEXT:      add (M1_NM, 1) [[I]](0,0)<1> [[I]](0,0)<0;1,0> 0x1:w
; CHECK-NEXT:      cmp.lt (M1_NM, 16) [[P:[A-Za-z0-9_]+]] {{.*}}
; CHECK-NEXT:      ([[P]]) goto (M1, 1) [[LOOP]]
; CHECK:           {{^[A-Za-z0-9_]+}}:
; CHECK-NEXT:      add (M1, 16) [[ADD:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ACC]]:d32.16x8nn
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ADD]]:d32.16x16nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK-OFF:       .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @loop_base_outlives(i64 %base, i32 %x, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %blk)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi <8 x float> [ zeroinitializer, %entry ], [ %acc.next, %loop ]
  %acc.next = fadd <8 x float> %acc, %sub
  %i.next = add i32 %i, 1
  %cond = icmp ult i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  %add = fadd <16 x float> %exp, %blk
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %acc.next)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  ret void
}

; Negative, loop: %exp dies before the loop, but %sub is used in the loop.
; Aliasing would keep all of %exp live through the loop. %sub is copied.
; CHECK-LABEL:     .kernel "loop_sub_outlives"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ADD]]:d32.16x16nn
; CHECK-NEXT:      mov (M1_NM, 1) [[I:[A-Za-z0-9_]+]](0,0)<1> 0x0:d
; CHECK-NEXT:      mov (M1, 16) [[ACC:[A-Za-z0-9_]+]](0,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](1,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](2,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](3,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](4,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](5,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](6,0)<1> 0x0:f
; CHECK-NEXT:      mov (M1, 16) [[ACC]](7,0)<1> 0x0:f
; CHECK:           {{^}}[[LOOP:[A-Za-z0-9_]+]]:
; CHECK-NEXT:      add (M1, 16) [[ACC]](0,0)<1> [[ACC]](0,0)<1;1,0> [[SUB]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ACC]](1,0)<1> [[ACC]](1,0)<1;1,0> [[SUB]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ACC]](2,0)<1> [[ACC]](2,0)<1;1,0> [[SUB]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ACC]](3,0)<1> [[ACC]](3,0)<1;1,0> [[SUB]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ACC]](4,0)<1> [[ACC]](4,0)<1;1,0> [[SUB]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ACC]](5,0)<1> [[ACC]](5,0)<1;1,0> [[SUB]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ACC]](6,0)<1> [[ACC]](6,0)<1;1,0> [[SUB]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ACC]](7,0)<1> [[ACC]](7,0)<1;1,0> [[SUB]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1_NM, 1) [[I]](0,0)<1> [[I]](0,0)<0;1,0> 0x1:w
; CHECK-NEXT:      cmp.lt (M1_NM, 16) [[P:[A-Za-z0-9_]+]] {{.*}}
; CHECK-NEXT:      ([[P]]) goto (M1, 1) [[LOOP]]
; CHECK:           {{^[A-Za-z0-9_]+}}:
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ACC]]:d32.16x8nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK:           .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @loop_sub_outlives(i64 %base, i32 %x, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %blk)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %add = fadd <16 x float> %exp, %blk
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi <8 x float> [ zeroinitializer, %entry ], [ %acc.next, %loop ]
  %acc.next = fadd <8 x float> %acc, %sub
  %i.next = add i32 %i, 1
  %cond = icmp ult i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %acc.next)
  ret void
}

; Positive, loop body: %exp and %sub are both defined in the loop, and %exp is
; used after %sub in the same iteration. %sub is aliased to %exp(8..15).
; The loop phi %acc is coalesced with %blk.
; CHECK-LABEL:     .kernel "loop_body"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      mov (M1_NM, 1) [[I:[A-Za-z0-9_]+]](0,0)<1> 0x0:d
; CHECK:           {{^}}[[LOOP:[A-Za-z0-9_]+]]:
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
;
; CHECK-OFF-NEXT:  mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[SUB]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](1,0)<1> [[SUB]](1,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](2,0)<1> [[SUB]](2,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](3,0)<1> [[SUB]](3,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](4,0)<1> [[SUB]](4,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](5,0)<1> [[SUB]](5,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](6,0)<1> [[SUB]](6,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](7,0)<1> [[SUB]](7,0)<1;1,0>
;
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](1,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](2,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](3,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](4,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](5,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](6,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](7,0)<1> [[EXP]](15,0)<1;1,0>
;
; CHECK-NEXT:      add (M1, 16) [[BLK]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[SEXP]]:d32.16x8nn
; CHECK-NEXT:      add (M1_NM, 1) [[I]](0,0)<1> [[I]](0,0)<0;1,0> 0x1:w
; CHECK-NEXT:      cmp.lt (M1_NM, 16) [[P:[A-Za-z0-9_]+]] {{.*}}
; CHECK-NEXT:      ([[P]]) goto (M1, 1) [[LOOP]]
; CHECK:           {{^[A-Za-z0-9_]+}}:
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[BLK]]:d32.16x16nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK-OFF:       .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @loop_body(i64 %base, i32 %x, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi <16 x float> [ %blk, %entry ], [ %add, %loop ]
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %acc)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %sexp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %sub)
  %add = fadd <16 x float> %exp, %acc
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %sexp)
  %i.next = add i32 %i, 1
  %cond = icmp ult i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  ret void
}

; Negative, loop body: like loop_body, but %sub is used after the last use of
; %exp in the iteration. %sub is copied.
; CHECK-LABEL:     .kernel "loop_body_sub_outlives"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      mov (M1_NM, 1) [[I:[A-Za-z0-9_]+]](0,0)<1> 0x0:d
; CHECK:           {{^}}[[LOOP:[A-Za-z0-9_]+]]:
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[BLK]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[SUB]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](1,0)<1> [[SUB]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](2,0)<1> [[SUB]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](3,0)<1> [[SUB]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](4,0)<1> [[SUB]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](5,0)<1> [[SUB]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](6,0)<1> [[SUB]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](7,0)<1> [[SUB]](7,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[SEXP]]:d32.16x8nn
; CHECK-NEXT:      add (M1_NM, 1) [[I]](0,0)<1> [[I]](0,0)<0;1,0> 0x1:w
; CHECK-NEXT:      cmp.lt (M1_NM, 16) [[P:[A-Za-z0-9_]+]] {{.*}}
; CHECK-NEXT:      ([[P]]) goto (M1, 1) [[LOOP]]
; CHECK:           {{^[A-Za-z0-9_]+}}:
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[BLK]]:d32.16x16nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK:           .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @loop_body_sub_outlives(i64 %base, i32 %x, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi <16 x float> [ %blk, %entry ], [ %add, %loop ]
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %acc)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %add = fadd <16 x float> %exp, %acc
  %sexp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %sub)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %sexp)
  %i.next = add i32 %i, 1
  %cond = icmp ult i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  ret void
}

; Positive, shared extract: %sub.e0 has another use (%e0x), but it is before
; the last use of %exp. %sub and %sub.e0 are aliased to %exp.
; CHECK-LABEL:     .kernel "extract_used_base_outlives"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
;
; CHECK-OFF-NEXT:  mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-OFF-NEXT:  mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[SUB]](0,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](1,0)<1> [[SUB]](1,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](2,0)<1> [[SUB]](2,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](3,0)<1> [[SUB]](3,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](4,0)<1> [[SUB]](4,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](5,0)<1> [[SUB]](5,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](6,0)<1> [[SUB]](6,0)<1;1,0>
; CHECK-OFF-NEXT:  exp (M1, 16) [[SEXP]](7,0)<1> [[SUB]](7,0)<1;1,0>
; CHECK-OFF-NEXT:  mul (M1, 16) [[E0X:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0> 0x40000000:f
;
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](1,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](2,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](3,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](4,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](5,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](6,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-ON-NEXT:   exp (M1, 16) [[SEXP]](7,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-ON-NEXT:   mul (M1, 16) [[E0X:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0> 0x40000000:f
;
; CHECK-NEXT:      add (M1, 16) [[ADD:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ADD]]:d32.16x16nn
; CHECK-NEXT:      mov (M1, 16) [[SEXP]](0,0)<1> [[E0X]](0,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[SEXP]]:d32.16x8nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK-OFF:       .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @extract_used_base_outlives(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %blk)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %sexp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %sub)
  %e0x = fmul float %sub.e0, 2.0
  %add = fadd <16 x float> %exp, %blk
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  %res = insertelement <8 x float> %sexp, float %e0x, i32 0
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %res)
  ret void
}

; Negative, shared extract: like extract_used_base_outlives, but %sub.e0 is
; also used after the last use of %exp. Aliasing %sub.e0 would make %exp live
; longer, so %sub is copied even though %exp outlives %sub.
; CHECK-LABEL:     .kernel "extract_outlives"
; CHECK:           lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:      exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](8,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](9,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](1,0)<1> [[E1]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](10,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](2,0)<1> [[E2]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](11,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](3,0)<1> [[E3]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](12,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](4,0)<1> [[E4]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](13,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](5,0)<1> [[E5]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](14,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](6,0)<1> [[E6]](0,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](15,0)<1;1,0>
; CHECK-NEXT:      mov (M1, 16) [[SUB]](7,0)<1> [[E7]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP:[A-Za-z0-9_]+]](0,0)<1> [[SUB]](0,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](1,0)<1> [[SUB]](1,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](2,0)<1> [[SUB]](2,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](3,0)<1> [[SUB]](3,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](4,0)<1> [[SUB]](4,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](5,0)<1> [[SUB]](5,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](6,0)<1> [[SUB]](6,0)<1;1,0>
; CHECK-NEXT:      exp (M1, 16) [[SEXP]](7,0)<1> [[SUB]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:      add (M1, 16) [[ADD]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[ADD]]:d32.16x16nn
; CHECK-NEXT:      mul (M1, 16) [[E0X:[A-Za-z0-9_]+]](0,0)<1> [[E0]](0,0)<1;1,0> 0x40000000:f
; CHECK-NEXT:      mov (M1, 16) [[SEXP]](0,0)<1> [[E0X]](0,0)<1;1,0>
; CHECK-NEXT:      lsc_store_block2d.ugm (M1, 1) {{.*}} [[SEXP]]:d32.16x8nn
; CHECK-NEXT:      ret (M1, 1)
; CHECK:           .decl [[SUB]] v_type=G type=f num_elts=128 align=wordx32{{$}}

define spir_kernel void @extract_outlives(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %blk)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %sexp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %sub)
  %add = fadd <16 x float> %exp, %blk
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 16, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  %e0x = fmul float %sub.e0, 2.0
  %res = insertelement <8 x float> %sexp, float %e0x, i32 0
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %res)
  ret void
}

declare <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x float>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <16 x float>)
declare <8 x float> @llvm.exp2.v8f32(<8 x float>)
declare <16 x float> @llvm.exp2.v16f32(<16 x float>)

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7}
!0 = !{ptr @base_outlives, !9}
!1 = !{ptr @sub_outlives, !9}
!2 = !{ptr @loop_base_outlives, !9}
!3 = !{ptr @loop_sub_outlives, !9}
!4 = !{ptr @loop_body, !9}
!5 = !{ptr @loop_body_sub_outlives, !9}
!6 = !{ptr @extract_used_base_outlives, !9}
!7 = !{ptr @extract_outlives, !9}
!9 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!10}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27}
!12 = !{!"FuncMDMap[0]", ptr @base_outlives}
!13 = !{!"FuncMDValue[0]", !30, !33}
!14 = !{!"FuncMDMap[1]", ptr @sub_outlives}
!15 = !{!"FuncMDValue[1]", !30, !33}
!16 = !{!"FuncMDMap[2]", ptr @loop_base_outlives}
!17 = !{!"FuncMDValue[2]", !30, !33}
!18 = !{!"FuncMDMap[3]", ptr @loop_sub_outlives}
!19 = !{!"FuncMDValue[3]", !30, !33}
!20 = !{!"FuncMDMap[4]", ptr @loop_body}
!21 = !{!"FuncMDValue[4]", !30, !33}
!22 = !{!"FuncMDMap[5]", ptr @loop_body_sub_outlives}
!23 = !{!"FuncMDValue[5]", !30, !33}
!24 = !{!"FuncMDMap[6]", ptr @extract_used_base_outlives}
!25 = !{!"FuncMDValue[6]", !30, !33}
!26 = !{!"FuncMDMap[7]", ptr @extract_outlives}
!27 = !{!"FuncMDValue[7]", !30, !33}
!30 = !{!"implicitArgInfoList", !31, !32}
!31 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!32 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!33 = !{!"requiredSubGroupSize", i32 16}
