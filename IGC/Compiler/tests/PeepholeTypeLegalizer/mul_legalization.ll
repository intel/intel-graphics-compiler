;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S %s | FileCheck %s
; Tests legalization of mul instruction for i48 and i128 types.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_func i48 @test_mul_i48(i48 %src0, i48 %src1) {
  ; CHECK-LABEL: @test_mul_i48(
  ; CHECK: [[TMP1:%.*]] = zext i48 [[SRC0:%.*]] to i64
  ; CHECK: [[TMP2:%.*]] = zext i48 [[SRC1:%.*]] to i64
  ; CHECK: [[TMP3:%.*]] = bitcast i64 [[TMP1]] to <2 x i32>
  ; CHECK: [[TMP4:%.*]] = bitcast i64 [[TMP2]] to <2 x i32>
  ; CHECK: [[TMP5:%.*]] = extractelement <2 x i32> [[TMP3]], i64 0
  ; CHECK: [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i64 0
  ; CHECK: [[TMP7:%.*]] = extractelement <2 x i32> [[TMP3]], i64 1
  ; CHECK: [[TMP8:%.*]] = extractelement <2 x i32> [[TMP4]], i64 1

  ; CHECK: [[TMP9:%.*]] = mul i32 [[TMP5]], [[TMP6]]
  ; CHECK: [[TMP10:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP5]], i32 [[TMP6]])
  ; CHECK: [[TMP11:%.*]] = mul i32 [[TMP5]], [[TMP8]]
  ; CHECK: [[TMP12:%.*]] = add i32 [[TMP10]], [[TMP11]]
  ; CHECK: [[TMP13:%.*]] = mul i32 [[TMP7]], [[TMP6]]
  ; CHECK: [[TMP14:%.*]] = add i32 [[TMP12]], [[TMP13]]
  ; CHECK: [[TMP15:%.*]] = and i32 [[TMP14]], 65535
  ; CHECK: [[TMP16:%.*]] = insertelement <2 x i32> undef, i32 [[TMP9]], i64 0
  ; CHECK: [[TMP17:%.*]] = insertelement <2 x i32> [[TMP16]], i32 [[TMP15]], i64 1
  ; CHECK: [[TMP18:%.*]] = bitcast <2 x i32> [[TMP17]] to i64
  ; CHECK: [[TMP19:%.*]] = trunc i64 [[TMP18]] to i48
  %1 = mul i48 %src0, %src1
  ; CHECK: ret i48 [[TMP19]]
  ret i48 %1
}

define spir_func i128 @test_mul_i128(i128 %src0, i128 %src1) {
  ; CHECK-LABEL: @test_mul_i128(
  ; CHECK: [[TMP1:%.*]] = bitcast i128 [[SRC0:%.*]] to <4 x i32>
  ; CHECK: [[TMP2:%.*]] = bitcast i128 [[SRC1:%.*]] to <4 x i32>
  ; CHECK: [[TMP3:%.*]] = extractelement <4 x i32> [[TMP1]], i64 0
  ; CHECK: [[TMP4:%.*]] = extractelement <4 x i32> [[TMP2]], i64 0
  ; CHECK: [[TMP5:%.*]] = extractelement <4 x i32> [[TMP1]], i64 1
  ; CHECK: [[TMP6:%.*]] = extractelement <4 x i32> [[TMP2]], i64 1
  ; CHECK: [[TMP7:%.*]] = extractelement <4 x i32> [[TMP1]], i64 2
  ; CHECK: [[TMP8:%.*]] = extractelement <4 x i32> [[TMP2]], i64 2
  ; CHECK: [[TMP9:%.*]] = extractelement <4 x i32> [[TMP1]], i64 3
  ; CHECK: [[TMP10:%.*]] = extractelement <4 x i32> [[TMP2]], i64 3
  ; CHECK: [[TMP11:%.*]] = mul i32 [[TMP3]], [[TMP4]]
  ; CHECK: [[TMP12:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP3]], i32 [[TMP4]])
  ; CHECK: [[TMP13:%.*]] = mul i32 [[TMP3]], [[TMP6]]
  ; CHECK: [[TMP14:%.*]] = add i32 [[TMP12]], [[TMP13]]
  ; CHECK: [[TMP15:%.*]] = icmp ult i32 [[TMP14]], [[TMP12]]
  ; CHECK: [[TMP16:%.*]] = zext i1 [[TMP15]] to i32
  ; CHECK: [[TMP17:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP3]], i32 [[TMP6]])
  ; CHECK: [[TMP18:%.*]] = add i32 [[TMP16]], [[TMP17]]
  ; CHECK: [[TMP19:%.*]] = icmp ult i32 [[TMP18]], [[TMP16]]
  ; CHECK: [[TMP20:%.*]] = zext i1 [[TMP19]] to i32
  ; CHECK: [[TMP21:%.*]] = mul i32 [[TMP3]], [[TMP8]]
  ; CHECK: [[TMP22:%.*]] = add i32 [[TMP18]], [[TMP21]]
  ; CHECK: [[TMP23:%.*]] = icmp ult i32 [[TMP22]], [[TMP18]]
  ; CHECK: [[TMP24:%.*]] = zext i1 [[TMP23]] to i32
  ; CHECK: [[TMP25:%.*]] = add i32 [[TMP20]], [[TMP24]]
  ; CHECK: [[TMP26:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP3]], i32 [[TMP8]])
  ; CHECK: [[TMP27:%.*]] = add i32 [[TMP25]], [[TMP26]]
  ; CHECK: [[TMP28:%.*]] = mul i32 [[TMP3]], [[TMP10]]
  ; CHECK: [[TMP29:%.*]] = add i32 [[TMP27]], [[TMP28]]
  ; CHECK: [[TMP30:%.*]] = mul i32 [[TMP5]], [[TMP4]]
  ; CHECK: [[TMP31:%.*]] = add i32 [[TMP14]], [[TMP30]]
  ; CHECK: [[TMP32:%.*]] = icmp ult i32 [[TMP31]], [[TMP14]]
  ; CHECK: [[TMP33:%.*]] = zext i1 [[TMP32]] to i32
  ; CHECK: [[TMP34:%.*]] = add i32 [[TMP22]], [[TMP33]]
  ; CHECK: [[TMP35:%.*]] = icmp ult i32 [[TMP34]], [[TMP22]]
  ; CHECK: [[TMP36:%.*]] = zext i1 [[TMP35]] to i32
  ; CHECK: [[TMP37:%.*]] = add i32 [[TMP29]], [[TMP36]]
  ; CHECK: [[TMP38:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP5]], i32 [[TMP4]])
  ; CHECK: [[TMP39:%.*]] = add i32 [[TMP34]], [[TMP38]]
  ; CHECK: [[TMP40:%.*]] = icmp ult i32 [[TMP39]], [[TMP34]]
  ; CHECK: [[TMP41:%.*]] = zext i1 [[TMP40]] to i32
  ; CHECK: [[TMP42:%.*]] = add i32 [[TMP37]], [[TMP41]]
  ; CHECK: [[TMP43:%.*]] = mul i32 [[TMP5]], [[TMP6]]
  ; CHECK: [[TMP44:%.*]] = add i32 [[TMP39]], [[TMP43]]
  ; CHECK: [[TMP45:%.*]] = icmp ult i32 [[TMP44]], [[TMP39]]
  ; CHECK: [[TMP46:%.*]] = zext i1 [[TMP45]] to i32
  ; CHECK: [[TMP47:%.*]] = add i32 [[TMP42]], [[TMP46]]
  ; CHECK: [[TMP48:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP5]], i32 [[TMP6]])
  ; CHECK: [[TMP49:%.*]] = add i32 [[TMP47]], [[TMP48]]
  ; CHECK: [[TMP50:%.*]] = mul i32 [[TMP5]], [[TMP8]]
  ; CHECK: [[TMP51:%.*]] = add i32 [[TMP49]], [[TMP50]]
  ; CHECK: [[TMP52:%.*]] = mul i32 [[TMP7]], [[TMP4]]
  ; CHECK: [[TMP53:%.*]] = add i32 [[TMP44]], [[TMP52]]
  ; CHECK: [[TMP54:%.*]] = icmp ult i32 [[TMP53]], [[TMP44]]
  ; CHECK: [[TMP55:%.*]] = zext i1 [[TMP54]] to i32
  ; CHECK: [[TMP56:%.*]] = add i32 [[TMP51]], [[TMP55]]
  ; CHECK: [[TMP57:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP7]], i32 [[TMP4]])
  ; CHECK: [[TMP58:%.*]] = add i32 [[TMP56]], [[TMP57]]
  ; CHECK: [[TMP59:%.*]] = mul i32 [[TMP7]], [[TMP6]]
  ; CHECK: [[TMP60:%.*]] = add i32 [[TMP58]], [[TMP59]]
  ; CHECK: [[TMP61:%.*]] = mul i32 [[TMP9]], [[TMP4]]
  ; CHECK: [[TMP62:%.*]] = add i32 [[TMP60]], [[TMP61]]
  ; CHECK: [[TMP63:%.*]] = insertelement <4 x i32> undef, i32 [[TMP11]], i64 0
  ; CHECK: [[TMP64:%.*]] = insertelement <4 x i32> [[TMP63]], i32 [[TMP31]], i64 1
  ; CHECK: [[TMP65:%.*]] = insertelement <4 x i32> [[TMP64]], i32 [[TMP53]], i64 2
  ; CHECK: [[TMP66:%.*]] = insertelement <4 x i32> [[TMP65]], i32 [[TMP62]], i64 3
  ; CHECK: [[TMP67:%.*]] = bitcast <4 x i32> [[TMP66]] to i128
  %1 = mul i128 %src0, %src1
  ; CHECK: ret i128 [[TMP67]]
  ret i128 %1
}
