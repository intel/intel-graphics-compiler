;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i16 @llvm.genx.GenISA.simdLaneId()
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare double @llvm.genx.GenISA.WaveShuffleIndex.f64(double, i32, i32)

; Change the call in simple case
define void @test_transformation_simple(i32 %x) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, 1
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor.i, i32 0)
  ret void
}
; CHECK-LABEL: @test_transformation_simple
; CHECK: call i32 @llvm.genx.GenISA.simdShuffleXor{{.*}}(i32 %x, i32 1)

; Change the call in simple case, zext first
define void @test_transformation_simple2(i32 %x) nounwind {
entry:
  %simdLaneId16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %simdLaneId = zext i16 %simdLaneId16 to i32
  %xor = xor i32 %simdLaneId, 2
  %simdShuffle.2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor, i32 0)
  ret void
}
; CHECK-LABEL: @test_transformation_simple2
; CHECK: call i32 @llvm.genx.GenISA.simdShuffleXor{{.*}}(i32 %x, i32 2)

; Change the call in double case too
define void @test_transformation_double(double %x) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, 15
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %x, i32 %xor.i, i32 0)
  ret void
}
; CHECK-LABEL: @test_transformation_double
; CHECK: call double @llvm.genx.GenISA.simdShuffleXor{{.*}}(double %x, i32 15)


; Change both calls when the value is splitted into high and low parts
define void @test_transformation_splitted(i64 %x) nounwind {
entry:
  %vec = bitcast i64 %x to <2 x i32>
  %scalar1 = extractelement <2 x i32> %vec, i32 0
  %scalar2 = extractelement <2 x i32> %vec, i32 1
  %simdLaneId16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId16, 8
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %scalar1, i32 %xor.i, i32 0)
  %simdShuffle2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %scalar2, i32 %xor.i, i32 0)
  %assembled.vect = insertelement <2 x i32> undef, i32 %simdShuffle, i32 0
  %assembled.vect2 = insertelement <2 x i32> %assembled.vect, i32 %simdShuffle2, i32 1
  ret void
}
; CHECK-LABEL: @test_transformation_splitted
; CHECK: [[I1:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.simdShuffleXor{{.*}}(i32 %scalar1, i32 8)
; CHECK: [[I2:%[a-zA-Z0-9.]+]] = call i32 @llvm.genx.GenISA.simdShuffleXor{{.*}}(i32 %scalar2, i32 8)
; CHECK: [[RES:%[a-zA-Z0-9.]+]] = insertelement <2 x i32> undef, i32 [[I1]], i32 0
; CHECK: insertelement <2 x i32> [[RES]], i32 [[I2]], i32 1


; Do not change the call if xor is not constant
define void @test_no_constant(i32 %x, i16 %xor_value) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, %xor_value
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor.i, i32 0)
  ret void
}
; CHECK-LABEL: @test_no_constant
; CHECK: call i32 @llvm.genx.GenISA.WaveShuffleIndex.{{.*}}(i32 %x, i32 %xor.i, i32 0)


; Do not the call if helper lines (3rd param) is non-zero
define void @test_transformation_helper_lines(i32 %x) nounwind {
entry:
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor = xor i16 %simdLaneId, 1
  %xor.i = zext i16 %xor to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %x, i32 %xor.i, i32 1)
  ret void
}
; CHECK-LABEL: @test_transformation_helper_lines
; CHECK: call i32 @llvm.genx.GenISA.WaveShuffleIndex.{{.*}}(i32 %x, i32 %xor.i, i32 1)