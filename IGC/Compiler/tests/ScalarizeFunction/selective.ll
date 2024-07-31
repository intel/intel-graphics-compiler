;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --igc-scalarize -S --regkey=EnableSelectiveScalarizer=1 < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------
; This test checks if selective scalarization leaves vectorial instructions un-scalarized.
; ------------------------------------------------

define spir_kernel void @test_selective_1(i64 %addr) #0 {
; CHECK-LABEL: @test_selective_1(
; CHECK:    [[VECT_INT:%.*]] = add <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, zeroinitializer
; CHECK:    [[VECT_FLOAT:%.*]] = bitcast <8 x i32> [[VECT_INT]] to <8 x float>
; CHECK:    [[VECT_INT_2:%.*]] = bitcast <8 x float> [[VECT_FLOAT]] to <8 x i32>
; CHECK:    call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 [[ADDR:%.*]], i32 1023, i32 511, i32 1023, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> [[VECT_INT_2]])
; CHECK:    ret void
;

; define a vector and do some bitcasts
; nothing should get scalarized here

    %vectint = add <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, zeroinitializer
    %vectfloat = bitcast <8 x i32> %vectint to <8 x float>
    %vectcast = bitcast <8 x float> %vectfloat to <8 x i32>
    call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %addr, i32 1023, i32 511, i32 1023, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %vectcast)

    ret void
}

define spir_kernel void @test_selective_2(i64 %addr) #0 {
; CHECK-LABEL: @test_selective_2(
; CHECK:    [[VECT_INT:%.*]] = add <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, zeroinitializer
; CHECK:    [[VECT_FLOAT:%.*]] = bitcast <8 x i32> [[VECT_INT]] to <8 x float>
; CHECK:    [[VECT_INT_2:%.*]] = bitcast <8 x float> [[VECT_FLOAT]] to <8 x i32>
; CHECK:    call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 [[ADDR:%.*]], i32 1023, i32 511, i32 1023, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> [[VECT_INT_2]])
; CHECK:    [[CAST:%.*]] = bitcast <8 x float> [[VECT_FLOAT]] to <8 x i32>
; CHECK:    [[SCALAR_0:%.*]] = extractelement <8 x i32> [[CAST]], i32 0
; CHECK:    [[SCALAR_1:%.*]] = extractelement <8 x i32> [[CAST]], i32 1
; CHECK:    [[SCALAR_2:%.*]] = extractelement <8 x i32> [[CAST]], i32 2
; CHECK:    [[SCALAR_3:%.*]] = extractelement <8 x i32> [[CAST]], i32 3
; CHECK:    [[SCALAR_4:%.*]] = extractelement <8 x i32> [[CAST]], i32 4
; CHECK:    [[SCALAR_5:%.*]] = extractelement <8 x i32> [[CAST]], i32 5
; CHECK:    [[SCALAR_6:%.*]] = extractelement <8 x i32> [[CAST]], i32 6
; CHECK:    [[SCALAR_7:%.*]] = extractelement <8 x i32> [[CAST]], i32 7
; CHECK:    [[ADD:%.*]] = add i32 [[SCALAR_3]], [[SCALAR_5]]
; CHECK:    ret void
;
; same as before, but %vectfloat is used in another branch of the code
    %vectint = add <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, zeroinitializer
    %vectfloat = bitcast <8 x i32> %vectint to <8 x float>
    %vectcast = bitcast <8 x float> %vectfloat to <8 x i32>
    call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %addr, i32 1023, i32 511, i32 1023, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %vectcast)
; so scalarization should happen here
    %anothercast = bitcast <8 x float> %vectfloat to <8 x i32>
    %v1 = extractelement <8 x i32> %anothercast, i32 3
    %v2 = extractelement <8 x i32> %anothercast, i32 5
    %v3 = add i32 %v1, %v2
    ret void
}

define spir_kernel void @test_selective_3() {
; CHECK-LABEL: @test_selective_3(
; CHECK:    br label %[[LOOP:.*]]
; CHECK:    [[LOOP]]:
; CHECK:    [[OFFSET:%.*]] = phi i32 [ 0, [[INIT0:%.*]] ], [ [[NEWOFFSET:%.*]], %[[LOOP]] ]
; CHECK:    [[DATA:%.*]] = phi <8 x i32> [ zeroinitializer, [[INIT0]] ], [ [[NEWDATA:%.*]], %[[LOOP]] ]
; CHECK:    [[NEWDATA]] = call <8 x i32> @do_math_v8i32_v8i32(<8 x i32> [[DATA]])
; CHECK:    [[NEWOFFSET]] = add i32 [[OFFSET]], 1
; CHECK:    [[CMP:%.*]] = icmp eq i32 [[NEWOFFSET]], 10
; CHECK:    br i1 [[CMP]], label %[[END:.*]], label %[[LOOP]]
; CHECK:    [[END]]:
; CHECK:    ret void
;
; no scalarization happens here because the vectors %data and %newdata are used as whole
  br label %loop

loop:
  %offset  = phi i32 [ 0, %0 ], [ %newoffset, %loop ]

  %data    = phi <8 x i32> [ zeroinitializer, %0 ], [ %newdata, %loop ]
  %newdata = call <8 x i32> @do_math_v8i32_v8i32(<8 x i32> %data)

  %newoffset = add i32 %offset, 1
  %1 = icmp eq i32 %newoffset, 10
  br i1 %1, label %end, label %loop

end:
  ret void
}

define spir_kernel void @test_selective_4(i64 %addr) #0 {
; CHECK-LABEL: @test_selective_4(
; CHECK:    br label %[[LOOP:.*]]
; CHECK:    [[LOOP]]:
; CHECK:    [[OFFSET:%.*]] = phi i32 [ 0, [[INIT0:%.*]] ], [ [[NEWOFFSET:%.*]], %[[LOOP]] ]
; CHECK:    [[FLOAT_VECT:%.*]] = phi <8 x float> [ zeroinitializer, [[INIT0]] ], [ [[NEW_FLOAT_VECT:%.*]], %[[LOOP]] ]
; CHECK:    [[INT_VECT:%.*]] = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 [[ADDR:%.*]], i32 1023, i32 511, i32 1023, i32 [[OFFSET]], i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
; CHECK:    [[NEW_FLOAT_VECT]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[FLOAT_VECT]], <8 x i16> <i16 0, i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7>, <8 x i32> [[INT_VECT]], i32 11, i32 11, i32 8, i32 8, i1 false)
; CHECK:    [[NEWOFFSET]] = add i32 [[OFFSET]], 16
; CHECK:    [[CMP:%.*]] = icmp eq i32 [[NEWOFFSET]], 256
; CHECK:    br i1 [[CMP]], label %[[END:.*]], label %[[LOOP]]
; CHECK:    [[END]]:
; CHECK:    ret void
;
; same here: no scalarization
  br label %loop

loop:
  %offset = phi i32 [ 0, %0 ], [ %newoffset, %loop ]
  %float_vector = phi <8 x float> [ zeroinitializer, %0 ], [ %new_float_vector, %loop ]
  %int_vector = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %addr, i32 1023, i32 511, i32 1023, i32 %offset, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %new_float_vector = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %float_vector, <8 x i16> <i16 0, i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7>, <8 x i32> %int_vector, i32 11, i32 11, i32 8, i32 8, i1 false)
  %newoffset = add i32 %offset, 16
  %1 = icmp eq i32 %newoffset, 256
  br i1 %1, label %end, label %loop

end:
  ret void
}


define spir_kernel void @test_selective_5() {
; CHECK-LABEL: @test_selective_5(
; CHECK:    br label %[[LOOP:.*]]
; CHECK:    [[LOOP]]:
; CHECK:    [[OFFSET:%.*]] = phi i32 [ 0, [[INIT0:%.*]] ], [ [[NEWOFFSET:%.*]], %[[LOOP]] ]
; CHECK:    [[DATA1:%.*]] = phi i32 [ 0, [[INIT0]] ], [ [[NEWDATA_SCALAR:%.*]], %[[LOOP]] ]
; CHECK:    [[DATA3:%.*]] = phi i32 [ 0, [[INIT0]] ], [ [[NEWDATA_SCALAR10:%.*]], %[[LOOP]] ]
; CHECK:    [[DATA4:%.*]] = phi i32 [ 0, [[INIT0]] ], [ [[NEWDATA_SCALAR11:%.*]], %[[LOOP]] ]
; CHECK:    [[DATA5:%.*]] = phi i32 [ 0, [[INIT0]] ], [ [[NEWDATA_SCALAR12:%.*]], %[[LOOP]] ]
; CHECK:    [[DATA6:%.*]] = phi i32 [ 0, [[INIT0]] ], [ undef, %[[LOOP]] ]
; CHECK:    [[DATA7:%.*]] = phi i32 [ 0, [[INIT0]] ], [ undef, %[[LOOP]] ]
; CHECK:    [[DATA8:%.*]] = phi i32 [ 0, [[INIT0]] ], [ undef, %[[LOOP]] ]
; CHECK:    [[DATA9:%.*]] = phi i32 [ 0, [[INIT0]] ], [ undef, %[[LOOP]] ]
; CHECK:    [[VECT:%.*]] = insertelement <4 x i32> undef, i32 [[DATA1]], i32 0
; CHECK:    [[VECT13:%.*]] = insertelement <4 x i32> [[VECT]], i32 [[DATA3]], i32 1
; CHECK:    [[VECT14:%.*]] = insertelement <4 x i32> [[VECT13]], i32 [[DATA4]], i32 2
; CHECK:    [[VECT15:%.*]] = insertelement <4 x i32> [[VECT14]], i32 [[DATA5]], i32 3
; CHECK:    [[NEWDATA:%.*]] = call <4 x i32> @do_math_v4i32_v4i32(<4 x i32> [[VECT15]])
; CHECK:    [[NEWDATA_SCALAR]] = extractelement <4 x i32> [[NEWDATA]], i32 0
; CHECK:    [[NEWDATA_SCALAR10]] = extractelement <4 x i32> [[NEWDATA]], i32 1
; CHECK:    [[NEWDATA_SCALAR11]] = extractelement <4 x i32> [[NEWDATA]], i32 2
; CHECK:    [[NEWDATA_SCALAR12]] = extractelement <4 x i32> [[NEWDATA]], i32 3
; CHECK:    [[NEWOFFSET]] = add i32 [[OFFSET]], 1
; CHECK:    [[CMP:%.*]] = icmp eq i32 [[NEWOFFSET]], 10
; CHECK:    br i1 [[CMP]], label %[[END:.*]], label %[[LOOP]]
; CHECK:    [[END]]:
; CHECK:    ret void
;
; here shufflevectors break vectorial nature of the arguments
; scalarization should be done
  br label %loop

loop:
  %offset   = phi i32 [ 0, %0 ], [ %newoffset, %loop ]

  %data     = phi <8 x i32> [ zeroinitializer, %0 ], [ %newdata2, %loop ]
  %data2    = shufflevector <8 x i32> %data, <8 x i32> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %newdata  = call <4 x i32> @do_math_v4i32_v4i32(<4 x i32> %data2)
  %newdata2 = shufflevector <4 x i32> %newdata, <4 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef>

  %newoffset = add i32 %offset, 1
  %1 = icmp eq i32 %newoffset, 10
  br i1 %1, label %end, label %loop

end:
  ret void
}

declare spir_func void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>) #1
declare spir_func <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1
declare spir_func <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1
declare spir_func <4 x i32> @do_math_v4i32_v4i32(<4 x i32>) #1
declare spir_func <8 x i32> @do_math_v8i32_v8i32(<8 x i32>) #1

attributes #0 = { convergent nounwind }
attributes #1 = { nounwind }
