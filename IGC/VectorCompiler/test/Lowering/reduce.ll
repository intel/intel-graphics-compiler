;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=SIMD16 %s
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=SIMD32 %s
; REQUIRES: llvm_12_or_greater

declare i32 @llvm.vector.reduce.add.v96i32(<96 x i32>)
declare i32 @llvm.vector.reduce.mul.v96i32(<96 x i32>)
declare float @llvm.vector.reduce.fadd.v96f32(float, <96 x float>)
declare float @llvm.vector.reduce.fmul.v96f32(float, <96 x float>)
declare float @llvm.vector.reduce.fmin.v96f32(<96 x float>)
declare float @llvm.vector.reduce.fmax.v96f32(<96 x float>)

declare i32 @llvm.vector.reduce.add.v14i32(<14 x i32>)
declare i32 @llvm.vector.reduce.add.v73i32(<73 x i32>)

define i32 @test_add(<96 x i32> %src) {
; SIMD16-LABEL: @test_add(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC:%.*]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = add <16 x i32> [[TMP1]], [[TMP2]]
; SIMD16-NEXT:    [[TMP4:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 128, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = add <16 x i32> [[TMP3]], [[TMP4]]
; SIMD16-NEXT:    [[TMP6:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 192, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = add <16 x i32> [[TMP5]], [[TMP6]]
; SIMD16-NEXT:    [[TMP8:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 256, i32 undef)
; SIMD16-NEXT:    [[TMP9:%.*]] = add <16 x i32> [[TMP7]], [[TMP8]]
; SIMD16-NEXT:    [[TMP10:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 320, i32 undef)
; SIMD16-NEXT:    [[TMP11:%.*]] = add <16 x i32> [[TMP9]], [[TMP10]]
; SIMD16-NEXT:    [[TMP12:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP11]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP13:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP11]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP14:%.*]] = add <8 x i32> [[TMP12]], [[TMP13]]
; SIMD16-NEXT:    [[TMP15:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP14]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP16:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP14]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP17:%.*]] = add <4 x i32> [[TMP15]], [[TMP16]]
; SIMD16-NEXT:    [[TMP18:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP17]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP19:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP17]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP20:%.*]] = add <2 x i32> [[TMP18]], [[TMP19]]
; SIMD16-NEXT:    [[TMP21:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP20]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP22:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP20]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP23:%.*]] = add <1 x i32> [[TMP21]], [[TMP22]]
; SIMD16-NEXT:    [[TMP24:%.*]] = bitcast <1 x i32> [[TMP23]] to i32
; SIMD16-NEXT:    ret i32 [[TMP24]]
;
; SIMD32-LABEL: @test_add(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v96i32.i16(<96 x i32> [[SRC:%.*]], i32 0, i32 32, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 32, i32 1, i16 128, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = add <32 x i32> [[TMP1]], [[TMP2]]
; SIMD32-NEXT:    [[TMP4:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 32, i32 1, i16 256, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = add <32 x i32> [[TMP3]], [[TMP4]]
; SIMD32-NEXT:    [[TMP6:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[TMP5]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP7:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[TMP5]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = add <16 x i32> [[TMP6]], [[TMP7]]
; SIMD32-NEXT:    [[TMP9:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP8]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP10:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP8]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP11:%.*]] = add <8 x i32> [[TMP9]], [[TMP10]]
; SIMD32-NEXT:    [[TMP12:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP11]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP11]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP14:%.*]] = add <4 x i32> [[TMP12]], [[TMP13]]
; SIMD32-NEXT:    [[TMP15:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP14]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP16:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP14]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP17:%.*]] = add <2 x i32> [[TMP15]], [[TMP16]]
; SIMD32-NEXT:    [[TMP18:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP17]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP19:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP17]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP20:%.*]] = add <1 x i32> [[TMP18]], [[TMP19]]
; SIMD32-NEXT:    [[TMP21:%.*]] = bitcast <1 x i32> [[TMP20]] to i32
; SIMD32-NEXT:    ret i32 [[TMP21]]
;
  %reduce = call i32 @llvm.vector.reduce.add.v96i32(<96 x i32> %src)
  ret i32 %reduce
}

define i32 @test_add_14(<14 x i32> %src) {
; SIMD16-LABEL: @test_add_14(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> [[SRC:%.*]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> [[SRC]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = add <4 x i32> [[TMP1]], [[TMP2]]
; SIMD16-NEXT:    [[TMP4:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> [[SRC]], i32 0, i32 4, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = add <4 x i32> [[TMP3]], [[TMP4]]
; SIMD16-NEXT:    [[TMP6:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP5]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP5]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP8:%.*]] = add <2 x i32> [[TMP6]], [[TMP7]]
; SIMD16-NEXT:    [[TMP9:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v14i32.i16(<14 x i32> [[SRC]], i32 0, i32 2, i32 1, i16 48, i32 undef)
; SIMD16-NEXT:    [[TMP10:%.*]] = add <2 x i32> [[TMP8]], [[TMP9]]
; SIMD16-NEXT:    [[TMP11:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP10]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP12:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP10]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP13:%.*]] = add <1 x i32> [[TMP11]], [[TMP12]]
; SIMD16-NEXT:    [[TMP14:%.*]] = bitcast <1 x i32> [[TMP13]] to i32
; SIMD16-NEXT:    ret i32 [[TMP14]]
;
; SIMD32-LABEL: @test_add_14(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> [[SRC:%.*]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> [[SRC]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = add <4 x i32> [[TMP1]], [[TMP2]]
; SIMD32-NEXT:    [[TMP4:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v14i32.i16(<14 x i32> [[SRC]], i32 0, i32 4, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = add <4 x i32> [[TMP3]], [[TMP4]]
; SIMD32-NEXT:    [[TMP6:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP5]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP7:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP5]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = add <2 x i32> [[TMP6]], [[TMP7]]
; SIMD32-NEXT:    [[TMP9:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v14i32.i16(<14 x i32> [[SRC]], i32 0, i32 2, i32 1, i16 48, i32 undef)
; SIMD32-NEXT:    [[TMP10:%.*]] = add <2 x i32> [[TMP8]], [[TMP9]]
; SIMD32-NEXT:    [[TMP11:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP10]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP12:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP10]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = add <1 x i32> [[TMP11]], [[TMP12]]
; SIMD32-NEXT:    [[TMP14:%.*]] = bitcast <1 x i32> [[TMP13]] to i32
; SIMD32-NEXT:    ret i32 [[TMP14]]
;
  %reduce = call i32 @llvm.vector.reduce.add.v14i32(<14 x i32> %src)
  ret i32 %reduce
}

define i32 @test_add_73(<73 x i32> %src) {
; SIMD16-LABEL: @test_add_73(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v73i32.i16(<73 x i32> [[SRC:%.*]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = add <16 x i32> [[TMP1]], [[TMP2]]
; SIMD16-NEXT:    [[TMP4:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 128, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = add <16 x i32> [[TMP3]], [[TMP4]]
; SIMD16-NEXT:    [[TMP6:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 192, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = add <16 x i32> [[TMP5]], [[TMP6]]
; SIMD16-NEXT:    [[TMP8:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP7]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP9:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP7]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP10:%.*]] = add <8 x i32> [[TMP8]], [[TMP9]]
; SIMD16-NEXT:    [[TMP11:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 8, i32 1, i16 256, i32 undef)
; SIMD16-NEXT:    [[TMP12:%.*]] = add <8 x i32> [[TMP10]], [[TMP11]]
; SIMD16-NEXT:    [[TMP13:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP12]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP14:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP12]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP15:%.*]] = add <4 x i32> [[TMP13]], [[TMP14]]
; SIMD16-NEXT:    [[TMP16:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP15]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP17:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP15]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP18:%.*]] = add <2 x i32> [[TMP16]], [[TMP17]]
; SIMD16-NEXT:    [[TMP19:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP18]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP20:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP18]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP21:%.*]] = add <1 x i32> [[TMP19]], [[TMP20]]
; SIMD16-NEXT:    [[TMP22:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 1, i32 1, i16 288, i32 undef)
; SIMD16-NEXT:    [[TMP23:%.*]] = add <1 x i32> [[TMP21]], [[TMP22]]
; SIMD16-NEXT:    [[TMP24:%.*]] = bitcast <1 x i32> [[TMP23]] to i32
; SIMD16-NEXT:    ret i32 [[TMP24]]
;
; SIMD32-LABEL: @test_add_73(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v73i32.i16(<73 x i32> [[SRC:%.*]], i32 0, i32 32, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 32, i32 1, i16 128, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = add <32 x i32> [[TMP1]], [[TMP2]]
; SIMD32-NEXT:    [[TMP4:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[TMP3]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[TMP3]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD32-NEXT:    [[TMP6:%.*]] = add <16 x i32> [[TMP4]], [[TMP5]]
; SIMD32-NEXT:    [[TMP7:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP6]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP6]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP9:%.*]] = add <8 x i32> [[TMP7]], [[TMP8]]
; SIMD32-NEXT:    [[TMP10:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 8, i32 1, i16 256, i32 undef)
; SIMD32-NEXT:    [[TMP11:%.*]] = add <8 x i32> [[TMP9]], [[TMP10]]
; SIMD32-NEXT:    [[TMP12:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP11]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP11]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP14:%.*]] = add <4 x i32> [[TMP12]], [[TMP13]]
; SIMD32-NEXT:    [[TMP15:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP14]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP16:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP14]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP17:%.*]] = add <2 x i32> [[TMP15]], [[TMP16]]
; SIMD32-NEXT:    [[TMP18:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP17]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP19:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP17]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP20:%.*]] = add <1 x i32> [[TMP18]], [[TMP19]]
; SIMD32-NEXT:    [[TMP21:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v73i32.i16(<73 x i32> [[SRC]], i32 0, i32 1, i32 1, i16 288, i32 undef)
; SIMD32-NEXT:    [[TMP22:%.*]] = add <1 x i32> [[TMP20]], [[TMP21]]
; SIMD32-NEXT:    [[TMP23:%.*]] = bitcast <1 x i32> [[TMP22]] to i32
; SIMD32-NEXT:    ret i32 [[TMP23]]
;
  %reduce = call i32 @llvm.vector.reduce.add.v73i32(<73 x i32> %src)
  ret i32 %reduce
}

define i32 @test_mul(<96 x i32> %src) {
; SIMD16-LABEL: @test_mul(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC:%.*]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = mul <16 x i32> [[TMP1]], [[TMP2]]
; SIMD16-NEXT:    [[TMP4:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 128, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = mul <16 x i32> [[TMP3]], [[TMP4]]
; SIMD16-NEXT:    [[TMP6:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 192, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = mul <16 x i32> [[TMP5]], [[TMP6]]
; SIMD16-NEXT:    [[TMP8:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 256, i32 undef)
; SIMD16-NEXT:    [[TMP9:%.*]] = mul <16 x i32> [[TMP7]], [[TMP8]]
; SIMD16-NEXT:    [[TMP10:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 16, i32 1, i16 320, i32 undef)
; SIMD16-NEXT:    [[TMP11:%.*]] = mul <16 x i32> [[TMP9]], [[TMP10]]
; SIMD16-NEXT:    [[TMP12:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP11]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP13:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP11]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP14:%.*]] = mul <8 x i32> [[TMP12]], [[TMP13]]
; SIMD16-NEXT:    [[TMP15:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP14]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP16:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP14]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP17:%.*]] = mul <4 x i32> [[TMP15]], [[TMP16]]
; SIMD16-NEXT:    [[TMP18:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP17]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP19:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP17]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP20:%.*]] = mul <2 x i32> [[TMP18]], [[TMP19]]
; SIMD16-NEXT:    [[TMP21:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP20]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP22:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP20]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP23:%.*]] = mul <1 x i32> [[TMP21]], [[TMP22]]
; SIMD16-NEXT:    [[TMP24:%.*]] = bitcast <1 x i32> [[TMP23]] to i32
; SIMD16-NEXT:    ret i32 [[TMP24]]
;
; SIMD32-LABEL: @test_mul(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v96i32.i16(<96 x i32> [[SRC:%.*]], i32 0, i32 32, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 32, i32 1, i16 128, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = mul <32 x i32> [[TMP1]], [[TMP2]]
; SIMD32-NEXT:    [[TMP4:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v96i32.i16(<96 x i32> [[SRC]], i32 0, i32 32, i32 1, i16 256, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = mul <32 x i32> [[TMP3]], [[TMP4]]
; SIMD32-NEXT:    [[TMP6:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[TMP5]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP7:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[TMP5]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = mul <16 x i32> [[TMP6]], [[TMP7]]
; SIMD32-NEXT:    [[TMP9:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP8]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP10:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[TMP8]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP11:%.*]] = mul <8 x i32> [[TMP9]], [[TMP10]]
; SIMD32-NEXT:    [[TMP12:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP11]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> [[TMP11]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP14:%.*]] = mul <4 x i32> [[TMP12]], [[TMP13]]
; SIMD32-NEXT:    [[TMP15:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP14]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP16:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> [[TMP14]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP17:%.*]] = mul <2 x i32> [[TMP15]], [[TMP16]]
; SIMD32-NEXT:    [[TMP18:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP17]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP19:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[TMP17]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP20:%.*]] = mul <1 x i32> [[TMP18]], [[TMP19]]
; SIMD32-NEXT:    [[TMP21:%.*]] = bitcast <1 x i32> [[TMP20]] to i32
; SIMD32-NEXT:    ret i32 [[TMP21]]
;
  %reduce = call i32 @llvm.vector.reduce.mul.v96i32(<96 x i32> %src)
  ret i32 %reduce
}

define float @test_fadd(<96 x float> %src) {
; SIMD16-LABEL: @test_fadd(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = fadd <16 x float> [[TMP1]], [[TMP2]]
; SIMD16-NEXT:    [[TMP4:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 128, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = fadd <16 x float> [[TMP3]], [[TMP4]]
; SIMD16-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 192, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = fadd <16 x float> [[TMP5]], [[TMP6]]
; SIMD16-NEXT:    [[TMP8:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 256, i32 undef)
; SIMD16-NEXT:    [[TMP9:%.*]] = fadd <16 x float> [[TMP7]], [[TMP8]]
; SIMD16-NEXT:    [[TMP10:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 320, i32 undef)
; SIMD16-NEXT:    [[TMP11:%.*]] = fadd <16 x float> [[TMP9]], [[TMP10]]
; SIMD16-NEXT:    [[TMP12:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP13:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP14:%.*]] = fadd <8 x float> [[TMP12]], [[TMP13]]
; SIMD16-NEXT:    [[TMP15:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP16:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP17:%.*]] = fadd <4 x float> [[TMP15]], [[TMP16]]
; SIMD16-NEXT:    [[TMP18:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP19:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP20:%.*]] = fadd <2 x float> [[TMP18]], [[TMP19]]
; SIMD16-NEXT:    [[TMP21:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP22:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP23:%.*]] = fadd <1 x float> [[TMP21]], [[TMP22]]
; SIMD16-NEXT:    [[TMP24:%.*]] = bitcast <1 x float> [[TMP23]] to float
; SIMD16-NEXT:    [[TMP25:%.*]] = fadd float [[TMP24]], -0.000000e+00
; SIMD16-NEXT:    ret float [[TMP25]]
;
; SIMD32-LABEL: @test_fadd(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 32, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 128, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = fadd <32 x float> [[TMP1]], [[TMP2]]
; SIMD32-NEXT:    [[TMP4:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 256, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = fadd <32 x float> [[TMP3]], [[TMP4]]
; SIMD32-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP7:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = fadd <16 x float> [[TMP6]], [[TMP7]]
; SIMD32-NEXT:    [[TMP9:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP10:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP11:%.*]] = fadd <8 x float> [[TMP9]], [[TMP10]]
; SIMD32-NEXT:    [[TMP12:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP14:%.*]] = fadd <4 x float> [[TMP12]], [[TMP13]]
; SIMD32-NEXT:    [[TMP15:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP16:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP17:%.*]] = fadd <2 x float> [[TMP15]], [[TMP16]]
; SIMD32-NEXT:    [[TMP18:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP19:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP20:%.*]] = fadd <1 x float> [[TMP18]], [[TMP19]]
; SIMD32-NEXT:    [[TMP21:%.*]] = bitcast <1 x float> [[TMP20]] to float
; SIMD32-NEXT:    [[TMP22:%.*]] = fadd float [[TMP21]], -0.000000e+00
; SIMD32-NEXT:    ret float [[TMP22]]
;
  %reduce = call reassoc float @llvm.vector.reduce.fadd.v96f32(float -0.0, <96 x float> %src)
  ret float %reduce
}

define float @test_fmul(<96 x float> %src) {
; SIMD16-LABEL: @test_fmul(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = fmul <16 x float> [[TMP1]], [[TMP2]]
; SIMD16-NEXT:    [[TMP4:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 128, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = fmul <16 x float> [[TMP3]], [[TMP4]]
; SIMD16-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 192, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = fmul <16 x float> [[TMP5]], [[TMP6]]
; SIMD16-NEXT:    [[TMP8:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 256, i32 undef)
; SIMD16-NEXT:    [[TMP9:%.*]] = fmul <16 x float> [[TMP7]], [[TMP8]]
; SIMD16-NEXT:    [[TMP10:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 320, i32 undef)
; SIMD16-NEXT:    [[TMP11:%.*]] = fmul <16 x float> [[TMP9]], [[TMP10]]
; SIMD16-NEXT:    [[TMP12:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP13:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP14:%.*]] = fmul <8 x float> [[TMP12]], [[TMP13]]
; SIMD16-NEXT:    [[TMP15:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP16:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP17:%.*]] = fmul <4 x float> [[TMP15]], [[TMP16]]
; SIMD16-NEXT:    [[TMP18:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP19:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP20:%.*]] = fmul <2 x float> [[TMP18]], [[TMP19]]
; SIMD16-NEXT:    [[TMP21:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP22:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP23:%.*]] = fmul <1 x float> [[TMP21]], [[TMP22]]
; SIMD16-NEXT:    [[TMP24:%.*]] = bitcast <1 x float> [[TMP23]] to float
; SIMD16-NEXT:    [[TMP25:%.*]] = fmul float [[TMP24]], 1.000000e+00
; SIMD16-NEXT:    ret float [[TMP25]]
;
; SIMD32-LABEL: @test_fmul(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 32, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 128, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = fmul <32 x float> [[TMP1]], [[TMP2]]
; SIMD32-NEXT:    [[TMP4:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 256, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = fmul <32 x float> [[TMP3]], [[TMP4]]
; SIMD32-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP7:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = fmul <16 x float> [[TMP6]], [[TMP7]]
; SIMD32-NEXT:    [[TMP9:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP10:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP11:%.*]] = fmul <8 x float> [[TMP9]], [[TMP10]]
; SIMD32-NEXT:    [[TMP12:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP14:%.*]] = fmul <4 x float> [[TMP12]], [[TMP13]]
; SIMD32-NEXT:    [[TMP15:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP16:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP17:%.*]] = fmul <2 x float> [[TMP15]], [[TMP16]]
; SIMD32-NEXT:    [[TMP18:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP19:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP20:%.*]] = fmul <1 x float> [[TMP18]], [[TMP19]]
; SIMD32-NEXT:    [[TMP21:%.*]] = bitcast <1 x float> [[TMP20]] to float
; SIMD32-NEXT:    [[TMP22:%.*]] = fmul float [[TMP21]], 1.000000e+00
; SIMD32-NEXT:    ret float [[TMP22]]
;
  %reduce = call reassoc float @llvm.vector.reduce.fmul.v96f32(float 1.0, <96 x float> %src)
  ret float %reduce
}

define float @test_fmax(<96 x float> %src) {
; SIMD16-LABEL: @test_fmax(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[TMP1]], <16 x float> [[TMP2]])
; SIMD16-NEXT:    [[TMP4:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 128, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[TMP3]], <16 x float> [[TMP4]])
; SIMD16-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 192, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[TMP5]], <16 x float> [[TMP6]])
; SIMD16-NEXT:    [[TMP8:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 256, i32 undef)
; SIMD16-NEXT:    [[TMP9:%.*]] = call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[TMP7]], <16 x float> [[TMP8]])
; SIMD16-NEXT:    [[TMP10:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 320, i32 undef)
; SIMD16-NEXT:    [[TMP11:%.*]] = call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[TMP9]], <16 x float> [[TMP10]])
; SIMD16-NEXT:    [[TMP12:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP13:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP14:%.*]] = call <8 x float> @llvm.maxnum.v8f32(<8 x float> [[TMP12]], <8 x float> [[TMP13]])
; SIMD16-NEXT:    [[TMP15:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP16:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP17:%.*]] = call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[TMP15]], <4 x float> [[TMP16]])
; SIMD16-NEXT:    [[TMP18:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP19:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP20:%.*]] = call <2 x float> @llvm.maxnum.v2f32(<2 x float> [[TMP18]], <2 x float> [[TMP19]])
; SIMD16-NEXT:    [[TMP21:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP22:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP23:%.*]] = call <1 x float> @llvm.maxnum.v1f32(<1 x float> [[TMP21]], <1 x float> [[TMP22]])
; SIMD16-NEXT:    [[TMP24:%.*]] = bitcast <1 x float> [[TMP23]] to float
; SIMD16-NEXT:    ret float [[TMP24]]
;
; SIMD32-LABEL: @test_fmax(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 32, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 128, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = call <32 x float> @llvm.maxnum.v32f32(<32 x float> [[TMP1]], <32 x float> [[TMP2]])
; SIMD32-NEXT:    [[TMP4:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 256, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = call <32 x float> @llvm.maxnum.v32f32(<32 x float> [[TMP3]], <32 x float> [[TMP4]])
; SIMD32-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP7:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = call <16 x float> @llvm.maxnum.v16f32(<16 x float> [[TMP6]], <16 x float> [[TMP7]])
; SIMD32-NEXT:    [[TMP9:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP10:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP11:%.*]] = call <8 x float> @llvm.maxnum.v8f32(<8 x float> [[TMP9]], <8 x float> [[TMP10]])
; SIMD32-NEXT:    [[TMP12:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP14:%.*]] = call <4 x float> @llvm.maxnum.v4f32(<4 x float> [[TMP12]], <4 x float> [[TMP13]])
; SIMD32-NEXT:    [[TMP15:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP16:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP17:%.*]] = call <2 x float> @llvm.maxnum.v2f32(<2 x float> [[TMP15]], <2 x float> [[TMP16]])
; SIMD32-NEXT:    [[TMP18:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP19:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP20:%.*]] = call <1 x float> @llvm.maxnum.v1f32(<1 x float> [[TMP18]], <1 x float> [[TMP19]])
; SIMD32-NEXT:    [[TMP21:%.*]] = bitcast <1 x float> [[TMP20]] to float
; SIMD32-NEXT:    ret float [[TMP21]]
;
  %reduce = call reassoc float @llvm.vector.reduce.fmax.v96f32(<96 x float> %src)
  ret float %reduce
}

define float @test_fmin(<96 x float> %src) {
; SIMD16-LABEL: @test_fmin(
; SIMD16-NEXT:    [[TMP1:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP2:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD16-NEXT:    [[TMP3:%.*]] = call <16 x float> @llvm.minnum.v16f32(<16 x float> [[TMP1]], <16 x float> [[TMP2]])
; SIMD16-NEXT:    [[TMP4:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 128, i32 undef)
; SIMD16-NEXT:    [[TMP5:%.*]] = call <16 x float> @llvm.minnum.v16f32(<16 x float> [[TMP3]], <16 x float> [[TMP4]])
; SIMD16-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 192, i32 undef)
; SIMD16-NEXT:    [[TMP7:%.*]] = call <16 x float> @llvm.minnum.v16f32(<16 x float> [[TMP5]], <16 x float> [[TMP6]])
; SIMD16-NEXT:    [[TMP8:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 256, i32 undef)
; SIMD16-NEXT:    [[TMP9:%.*]] = call <16 x float> @llvm.minnum.v16f32(<16 x float> [[TMP7]], <16 x float> [[TMP8]])
; SIMD16-NEXT:    [[TMP10:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 16, i32 1, i16 320, i32 undef)
; SIMD16-NEXT:    [[TMP11:%.*]] = call <16 x float> @llvm.minnum.v16f32(<16 x float> [[TMP9]], <16 x float> [[TMP10]])
; SIMD16-NEXT:    [[TMP12:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP13:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP11]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD16-NEXT:    [[TMP14:%.*]] = call <8 x float> @llvm.minnum.v8f32(<8 x float> [[TMP12]], <8 x float> [[TMP13]])
; SIMD16-NEXT:    [[TMP15:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP16:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP14]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD16-NEXT:    [[TMP17:%.*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[TMP15]], <4 x float> [[TMP16]])
; SIMD16-NEXT:    [[TMP18:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP19:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP17]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD16-NEXT:    [[TMP20:%.*]] = call <2 x float> @llvm.minnum.v2f32(<2 x float> [[TMP18]], <2 x float> [[TMP19]])
; SIMD16-NEXT:    [[TMP21:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD16-NEXT:    [[TMP22:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP20]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD16-NEXT:    [[TMP23:%.*]] = call <1 x float> @llvm.minnum.v1f32(<1 x float> [[TMP21]], <1 x float> [[TMP22]])
; SIMD16-NEXT:    [[TMP24:%.*]] = bitcast <1 x float> [[TMP23]] to float
; SIMD16-NEXT:    ret float [[TMP24]]
;
; SIMD32-LABEL: @test_fmin(
; SIMD32-NEXT:    [[TMP1:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC:%.*]], i32 0, i32 32, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP2:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 128, i32 undef)
; SIMD32-NEXT:    [[TMP3:%.*]] = call <32 x float> @llvm.minnum.v32f32(<32 x float> [[TMP1]], <32 x float> [[TMP2]])
; SIMD32-NEXT:    [[TMP4:%.*]] = call <32 x float> @llvm.genx.rdregionf.v32f32.v96f32.i16(<96 x float> [[SRC]], i32 0, i32 32, i32 1, i16 256, i32 undef)
; SIMD32-NEXT:    [[TMP5:%.*]] = call <32 x float> @llvm.minnum.v32f32(<32 x float> [[TMP3]], <32 x float> [[TMP4]])
; SIMD32-NEXT:    [[TMP6:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP7:%.*]] = call <16 x float> @llvm.genx.rdregionf.v16f32.v32f32.i16(<32 x float> [[TMP5]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; SIMD32-NEXT:    [[TMP8:%.*]] = call <16 x float> @llvm.minnum.v16f32(<16 x float> [[TMP6]], <16 x float> [[TMP7]])
; SIMD32-NEXT:    [[TMP9:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP10:%.*]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v16f32.i16(<16 x float> [[TMP8]], i32 0, i32 8, i32 1, i16 32, i32 undef)
; SIMD32-NEXT:    [[TMP11:%.*]] = call <8 x float> @llvm.minnum.v8f32(<8 x float> [[TMP9]], <8 x float> [[TMP10]])
; SIMD32-NEXT:    [[TMP12:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP13:%.*]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v8f32.i16(<8 x float> [[TMP11]], i32 0, i32 4, i32 1, i16 16, i32 undef)
; SIMD32-NEXT:    [[TMP14:%.*]] = call <4 x float> @llvm.minnum.v4f32(<4 x float> [[TMP12]], <4 x float> [[TMP13]])
; SIMD32-NEXT:    [[TMP15:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP16:%.*]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v4f32.i16(<4 x float> [[TMP14]], i32 0, i32 2, i32 1, i16 8, i32 undef)
; SIMD32-NEXT:    [[TMP17:%.*]] = call <2 x float> @llvm.minnum.v2f32(<2 x float> [[TMP15]], <2 x float> [[TMP16]])
; SIMD32-NEXT:    [[TMP18:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; SIMD32-NEXT:    [[TMP19:%.*]] = call <1 x float> @llvm.genx.rdregionf.v1f32.v2f32.i16(<2 x float> [[TMP17]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; SIMD32-NEXT:    [[TMP20:%.*]] = call <1 x float> @llvm.minnum.v1f32(<1 x float> [[TMP18]], <1 x float> [[TMP19]])
; SIMD32-NEXT:    [[TMP21:%.*]] = bitcast <1 x float> [[TMP20]] to float
; SIMD32-NEXT:    ret float [[TMP21]]
;
  %reduce = call reassoc float @llvm.vector.reduce.fmin.v96f32(<96 x float> %src)
  ret float %reduce
}
