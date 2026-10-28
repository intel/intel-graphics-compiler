;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=1 | FileCheck %s

; extractFrom, sub-vector used by llvm.exp2: exp2 reads the 2D block read
; result directly.
; CHECK-LABEL: .kernel "extract_use"
; CHECK:       lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:  exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:  lsc_store_block2d.ugm (M1, 1) {{.*}} [[EXP]]:d32.16x8nn
; CHECK-NEXT:  ret

define spir_kernel void @extract_use(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %sub.e0 = extractelement <16 x float> %blk, i32 8
  %sub.e1 = extractelement <16 x float> %blk, i32 9
  %sub.e2 = extractelement <16 x float> %blk, i32 10
  %sub.e3 = extractelement <16 x float> %blk, i32 11
  %sub.e4 = extractelement <16 x float> %blk, i32 12
  %sub.e5 = extractelement <16 x float> %blk, i32 13
  %sub.e6 = extractelement <16 x float> %blk, i32 14
  %sub.e7 = extractelement <16 x float> %blk, i32 15
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  %exp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %sub)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %exp)
  ret void
}

; extractFrom, base defined by llvm.exp2: the 2D block write stores the upper
; half of the exp2 result directly.
; CHECK-LABEL: .kernel "extract_def"
; CHECK:       lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:  exp (M1, 16) [[EXP:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](2,0)<1> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](3,0)<1> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](4,0)<1> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](5,0)<1> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](6,0)<1> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](7,0)<1> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[EXP]](15,0)<1> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:  lsc_store_block2d.ugm (M1, 1) {{.*}} [[EXP]].512:d32.16x8nn
; CHECK-NEXT:  add (M1, 16) [[ADD:[A-Za-z0-9_]+]](0,0)<1> [[EXP]](0,0)<1;1,0> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](1,0)<1> [[EXP]](1,0)<1;1,0> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](2,0)<1> [[EXP]](2,0)<1;1,0> [[BLK]](2,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](3,0)<1> [[EXP]](3,0)<1;1,0> [[BLK]](3,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](4,0)<1> [[EXP]](4,0)<1;1,0> [[BLK]](4,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](5,0)<1> [[EXP]](5,0)<1;1,0> [[BLK]](5,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](6,0)<1> [[EXP]](6,0)<1;1,0> [[BLK]](6,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](7,0)<1> [[EXP]](7,0)<1;1,0> [[BLK]](7,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](8,0)<1> [[EXP]](8,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](9,0)<1> [[EXP]](9,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](10,0)<1> [[EXP]](10,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](11,0)<1> [[EXP]](11,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](12,0)<1> [[EXP]](12,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](13,0)<1> [[EXP]](13,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](14,0)<1> [[EXP]](14,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:  add (M1, 16) [[ADD]](15,0)<1> [[EXP]](15,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:  lsc_store_block2d.ugm (M1, 1) {{.*}} [[ADD]]:d32.16x16nn
; CHECK-NEXT:  ret

define spir_kernel void @extract_def(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %exp = call <16 x float> @llvm.exp2.v16f32(<16 x float> %blk)
  %sub.e0 = extractelement <16 x float> %exp, i32 8
  %sub.e1 = extractelement <16 x float> %exp, i32 9
  %sub.e2 = extractelement <16 x float> %exp, i32 10
  %sub.e3 = extractelement <16 x float> %exp, i32 11
  %sub.e4 = extractelement <16 x float> %exp, i32 12
  %sub.e5 = extractelement <16 x float> %exp, i32 13
  %sub.e6 = extractelement <16 x float> %exp, i32 14
  %sub.e7 = extractelement <16 x float> %exp, i32 15
  %sub.v0 = insertelement <8 x float> undef, float %sub.e0, i32 0
  %sub.v1 = insertelement <8 x float> %sub.v0, float %sub.e1, i32 1
  %sub.v2 = insertelement <8 x float> %sub.v1, float %sub.e2, i32 2
  %sub.v3 = insertelement <8 x float> %sub.v2, float %sub.e3, i32 3
  %sub.v4 = insertelement <8 x float> %sub.v3, float %sub.e4, i32 4
  %sub.v5 = insertelement <8 x float> %sub.v4, float %sub.e5, i32 5
  %sub.v6 = insertelement <8 x float> %sub.v5, float %sub.e6, i32 6
  %sub = insertelement <8 x float> %sub.v6, float %sub.e7, i32 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 %base, i32 63, i32 7, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x float> %sub)
  %add = fadd <16 x float> %exp, %blk
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %add)
  ret void
}

; insertTo, parts defined by llvm.maxnum and llvm.exp2: both write into the
; vector stored by the 2D block write.
; CHECK-LABEL: .kernel "insert_def"
; CHECK:       lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x16nn
; CHECK-NEXT:  max (M1, 16) [[CAT:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:  max (M1, 16) [[CAT]](1,0)<1> [[BLK]](1,0)<1;1,0> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:  max (M1, 16) [[CAT]](2,0)<1> [[BLK]](2,0)<1;1,0> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:  max (M1, 16) [[CAT]](3,0)<1> [[BLK]](3,0)<1;1,0> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:  max (M1, 16) [[CAT]](4,0)<1> [[BLK]](4,0)<1;1,0> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:  max (M1, 16) [[CAT]](5,0)<1> [[BLK]](5,0)<1;1,0> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:  max (M1, 16) [[CAT]](6,0)<1> [[BLK]](6,0)<1;1,0> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:  max (M1, 16) [[CAT]](7,0)<1> [[BLK]](7,0)<1;1,0> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](8,0)<1> [[BLK]](8,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](9,0)<1> [[BLK]](9,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](10,0)<1> [[BLK]](10,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](11,0)<1> [[BLK]](11,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](12,0)<1> [[BLK]](12,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](13,0)<1> [[BLK]](13,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](14,0)<1> [[BLK]](14,0)<1;1,0>
; CHECK-NEXT:  exp (M1, 16) [[CAT]](15,0)<1> [[BLK]](15,0)<1;1,0>
; CHECK-NEXT:  lsc_store_block2d.ugm (M1, 1) {{.*}} [[CAT]]:d32.16x16nn
; CHECK-NEXT:  ret

define spir_kernel void @insert_def(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %lo.e0 = extractelement <16 x float> %blk, i32 0
  %lo.e1 = extractelement <16 x float> %blk, i32 1
  %lo.e2 = extractelement <16 x float> %blk, i32 2
  %lo.e3 = extractelement <16 x float> %blk, i32 3
  %lo.e4 = extractelement <16 x float> %blk, i32 4
  %lo.e5 = extractelement <16 x float> %blk, i32 5
  %lo.e6 = extractelement <16 x float> %blk, i32 6
  %lo.e7 = extractelement <16 x float> %blk, i32 7
  %lo.v0 = insertelement <8 x float> undef, float %lo.e0, i32 0
  %lo.v1 = insertelement <8 x float> %lo.v0, float %lo.e1, i32 1
  %lo.v2 = insertelement <8 x float> %lo.v1, float %lo.e2, i32 2
  %lo.v3 = insertelement <8 x float> %lo.v2, float %lo.e3, i32 3
  %lo.v4 = insertelement <8 x float> %lo.v3, float %lo.e4, i32 4
  %lo.v5 = insertelement <8 x float> %lo.v4, float %lo.e5, i32 5
  %lo.v6 = insertelement <8 x float> %lo.v5, float %lo.e6, i32 6
  %lo = insertelement <8 x float> %lo.v6, float %lo.e7, i32 7
  %hi.e0 = extractelement <16 x float> %blk, i32 8
  %hi.e1 = extractelement <16 x float> %blk, i32 9
  %hi.e2 = extractelement <16 x float> %blk, i32 10
  %hi.e3 = extractelement <16 x float> %blk, i32 11
  %hi.e4 = extractelement <16 x float> %blk, i32 12
  %hi.e5 = extractelement <16 x float> %blk, i32 13
  %hi.e6 = extractelement <16 x float> %blk, i32 14
  %hi.e7 = extractelement <16 x float> %blk, i32 15
  %hi.v0 = insertelement <8 x float> undef, float %hi.e0, i32 0
  %hi.v1 = insertelement <8 x float> %hi.v0, float %hi.e1, i32 1
  %hi.v2 = insertelement <8 x float> %hi.v1, float %hi.e2, i32 2
  %hi.v3 = insertelement <8 x float> %hi.v2, float %hi.e3, i32 3
  %hi.v4 = insertelement <8 x float> %hi.v3, float %hi.e4, i32 4
  %hi.v5 = insertelement <8 x float> %hi.v4, float %hi.e5, i32 5
  %hi.v6 = insertelement <8 x float> %hi.v5, float %hi.e6, i32 6
  %hi = insertelement <8 x float> %hi.v6, float %hi.e7, i32 7
  %max = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %lo, <8 x float> %hi)
  %exp = call <8 x float> @llvm.exp2.v8f32(<8 x float> %hi)
  %cat.a0 = extractelement <8 x float> %max, i32 0
  %cat.a1 = extractelement <8 x float> %max, i32 1
  %cat.a2 = extractelement <8 x float> %max, i32 2
  %cat.a3 = extractelement <8 x float> %max, i32 3
  %cat.a4 = extractelement <8 x float> %max, i32 4
  %cat.a5 = extractelement <8 x float> %max, i32 5
  %cat.a6 = extractelement <8 x float> %max, i32 6
  %cat.a7 = extractelement <8 x float> %max, i32 7
  %cat.c0 = extractelement <8 x float> %exp, i32 0
  %cat.c1 = extractelement <8 x float> %exp, i32 1
  %cat.c2 = extractelement <8 x float> %exp, i32 2
  %cat.c3 = extractelement <8 x float> %exp, i32 3
  %cat.c4 = extractelement <8 x float> %exp, i32 4
  %cat.c5 = extractelement <8 x float> %exp, i32 5
  %cat.c6 = extractelement <8 x float> %exp, i32 6
  %cat.c7 = extractelement <8 x float> %exp, i32 7
  %cat.v0 = insertelement <16 x float> undef, float %cat.a0, i32 0
  %cat.v1 = insertelement <16 x float> %cat.v0, float %cat.a1, i32 1
  %cat.v2 = insertelement <16 x float> %cat.v1, float %cat.a2, i32 2
  %cat.v3 = insertelement <16 x float> %cat.v2, float %cat.a3, i32 3
  %cat.v4 = insertelement <16 x float> %cat.v3, float %cat.a4, i32 4
  %cat.v5 = insertelement <16 x float> %cat.v4, float %cat.a5, i32 5
  %cat.v6 = insertelement <16 x float> %cat.v5, float %cat.a6, i32 6
  %cat.v7 = insertelement <16 x float> %cat.v6, float %cat.a7, i32 7
  %cat.v8 = insertelement <16 x float> %cat.v7, float %cat.c0, i32 8
  %cat.v9 = insertelement <16 x float> %cat.v8, float %cat.c1, i32 9
  %cat.v10 = insertelement <16 x float> %cat.v9, float %cat.c2, i32 10
  %cat.v11 = insertelement <16 x float> %cat.v10, float %cat.c3, i32 11
  %cat.v12 = insertelement <16 x float> %cat.v11, float %cat.c4, i32 12
  %cat.v13 = insertelement <16 x float> %cat.v12, float %cat.c5, i32 13
  %cat.v14 = insertelement <16 x float> %cat.v13, float %cat.c6, i32 14
  %cat = insertelement <16 x float> %cat.v14, float %cat.c7, i32 15
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <16 x float> %cat)
  ret void
}

; insertTo, scalar parts defined by llvm.sqrt and llvm.floor.
; CHECK-LABEL: .kernel "scalar_ok"
; CHECK:       lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x2nn
; CHECK-NEXT:  sqrt (M1, 16) [[V:[A-Za-z0-9_]+]](0,0)<1> [[BLK]](0,0)<1;1,0>
; CHECK-NEXT:  rndd (M1, 16) [[V]](1,0)<1> [[BLK]](1,0)<1;1,0>
; CHECK-NEXT:  lsc_store_block2d.ugm (M1, 1) {{.*}} [[V]]:d32.16x2nn
; CHECK-NEXT:  ret

define spir_kernel void @scalar_ok(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <2 x float> @llvm.genx.GenISA.LSC2DBlockRead.v2f32(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
  %a = extractelement <2 x float> %blk, i32 0
  %b = extractelement <2 x float> %blk, i32 1
  %s0 = call float @llvm.sqrt.f32(float %a)
  %s1 = call float @llvm.floor.f32(float %b)
  %v0 = insertelement <2 x float> undef, float %s0, i32 0
  %v1 = insertelement <2 x float> %v0, float %s1, i32 1
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2f32(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0, <2 x float> %v1)
  ret void
}

; llvm.ctpop is not in the list of safe intrinsics, so it is still SKIP and
; its results are copied into the stored vector.
; CHECK-LABEL: .kernel "scalar_skip"
; CHECK:       lsc_load_block2d.ugm (M1, 1)  [[BLK:[A-Za-z0-9_]+]]:d32.16x2nn
; CHECK-NEXT:  cbit (M1, 16) [[S0A:[A-Za-z0-9_]+]](0,0)<1> [[BLKA:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NEXT:  cbit (M1, 16) [[S1A:[A-Za-z0-9_]+]](0,0)<1> [[BLKA]](1,0)<1;1,0>
; CHECK-NEXT:  mov (M1, 16) [[V:[A-Za-z0-9_]+]](0,0)<1> [[S0:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NEXT:  mov (M1, 16) [[V]](1,0)<1> [[S1:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NEXT:  lsc_store_block2d.ugm (M1, 1) {{.*}} [[V]]:d32.16x2nn
; CHECK-NEXT:  ret
; CHECK-DAG:   .decl [[BLKA]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[BLK]], 0>
; CHECK-DAG:   .decl [[S0]] v_type=G type=d num_elts=16 align=wordx32{{$}}
; CHECK-DAG:   .decl [[S0A]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[S0]], 0>
; CHECK-DAG:   .decl [[S1]] v_type=G type=d num_elts=16 align=wordx32{{$}}
; CHECK-DAG:   .decl [[S1A]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[S1]], 0>
; CHECK-DAG:   .decl [[V]] v_type=G type=d num_elts=32 align=wordx32{{$}}

define spir_kernel void @scalar_skip(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %blk = call <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0)
  %a = extractelement <2 x i32> %blk, i32 0
  %b = extractelement <2 x i32> %blk, i32 1
  %s0 = call i32 @llvm.ctpop.i32(i32 %a)
  %s1 = call i32 @llvm.ctpop.i32(i32 %b)
  %v0 = insertelement <2 x i32> undef, i32 %s0, i32 0
  %v1 = insertelement <2 x i32> %v0, i32 %s1, i32 1
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v2i32(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 2, i32 1, i1 false, i1 false, i32 0, <2 x i32> %v1)
  ret void
}

declare <16 x float> @llvm.genx.GenISA.LSC2DBlockRead.v16f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x float>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v16f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <16 x float>)
declare <8 x float> @llvm.exp2.v8f32(<8 x float>)
declare <16 x float> @llvm.exp2.v16f32(<16 x float>)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>)

declare <2 x float> @llvm.genx.GenISA.LSC2DBlockRead.v2f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <2 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v2i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v2f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <2 x float>)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v2i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <2 x i32>)
declare float @llvm.sqrt.f32(float)
declare float @llvm.floor.f32(float)
declare i32 @llvm.ctpop.i32(i32)

!igc.functions = !{!0, !1, !2, !3, !4}
!0 = !{ptr @extract_use, !5}
!1 = !{ptr @extract_def, !5}
!2 = !{ptr @insert_def, !5}
!3 = !{ptr @scalar_ok, !5}
!4 = !{ptr @scalar_skip, !5}
!5 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!10}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13, !14, !15, !16, !17, !18, !19, !20, !21}
!12 = !{!"FuncMDMap[0]", ptr @extract_use}
!13 = !{!"FuncMDValue[0]", !30, !33}
!14 = !{!"FuncMDMap[1]", ptr @extract_def}
!15 = !{!"FuncMDValue[1]", !30, !33}
!16 = !{!"FuncMDMap[2]", ptr @insert_def}
!17 = !{!"FuncMDValue[2]", !30, !33}
!18 = !{!"FuncMDMap[3]", ptr @scalar_ok}
!19 = !{!"FuncMDValue[3]", !30, !33}
!20 = !{!"FuncMDMap[4]", ptr @scalar_skip}
!21 = !{!"FuncMDValue[4]", !30, !33}
!30 = !{!"implicitArgInfoList", !31, !32}
!31 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!32 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!33 = !{!"requiredSubGroupSize", i32 16}
