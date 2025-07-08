;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

%__2D_DIM_Resource = type opaque
%"class.RWTexture2D<vector<float, 4> >" = type { <4 x float> }

; RUN: igc_opt -wave-ballot-cse -S %s | FileCheck %s

define i32 @test_basic_cse() {
; CHECK:       %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:  %value1 = add i32 %mask1, 1
; CHECK-NEXT:  %value2 = add i32 %mask1, 2
; CHECK-NEXT:  %result = add i32 %value1, %value2
; CHECK-NEXT:  ret i32 %result
entry:
  %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %value1 = add i32 %mask1, 1
  %mask2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %value2 = add i32 %mask2, 2
  %result = add i32 %value1, %value2
  ret i32 %result
}

define i32 @test_different_args_preserved() {
; CHECK:       %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:  %mask2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 0)
; CHECK-NEXT:  %mask3 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 1)
; CHECK-NEXT:  %result1 = add i32 %mask1, %mask2
; CHECK-NEXT:  %result2 = add i32 %result1, %mask3
; CHECK-NEXT:  ret i32 %result2
entry:
  %mask1 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %mask2 = call i32 @llvm.genx.GenISA.WaveBallot(i1 false, i32 0)
  %mask3 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 1)
  %result1 = add i32 %mask1, %mask2
  %result2 = add i32 %result1, %mask3
  ret i32 %result2
}

define i32 @test_preserves_first_call() {
; CHECK:       %important_first = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CHECK-NEXT:  %some_work = add i32 %important_first, 42
; CHECK-NEXT:  %result = add i32 %important_first, %some_work
; CHECK-NEXT:  ret i32 %result
entry:
  %important_first = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %some_work = add i32 %important_first, 42
  %redundant_second = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %result = add i32 %redundant_second, %some_work
  ret i32 %result
}

define void @test_wave_is_first_lane_cse() {
; CHECK-LABEL: define void @test_wave_is_first_lane_cse
; CECK-NEXT:     [[TMP1:%.*]] = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
; CECK-NEXT:     [[GROUPX:%.*]] = bitcast float [[TMP1]] to i32
; CECK-NEXT:     [[TMP2:%.*]] = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
; CECK-NEXT:     [[GROUPY:%.*]] = bitcast float [[TMP2]] to i32
; CECK-NEXT:     [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
; CECK-NEXT:     [[U0:%.*]] = inttoptr i32 [[TMP3]] to %__2D_DIM_Resource.0 addrspace(2490368)*
; CECK-NEXT:     [[TMP4:%.*]] = shl i32 [[GROUPX]], 4
; CECK-NEXT:     [[LOCALX:%.*]] = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
; CECK-NEXT:     [[THREADX:%.*]] = add i32 [[TMP4]], [[LOCALX]]
; CECK-NEXT:     [[TMP5:%.*]] = shl i32 [[GROUPY]], 4
; CECK-NEXT:     [[LOCALY:%.*]] = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
; CECK-NEXT:     [[THREADY:%.*]] = add i32 [[TMP5]], [[LOCALY]]
; CECK-NEXT:     [[TMP6:%.*]] = and i32 [[THREADX]], 1
; CECK-NEXT:     [[TMP7:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CECK-NEXT:     [[TMP8:%.*]] = zext i16 [[TMP7]] to i32
; CECK-NEXT:     [[TMP9:%.*]] = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
; CECK-NEXT:     [[TMP10:%.*]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[TMP9]])
; CECK-NEXT:     [[TMP11:%.*]] = icmp eq i32 [[TMP10]], [[TMP8]]
; CECK-NEXT:     [[TMP12:%.*]] = zext i1 [[TMP11]] to i32
; CECK-NEXT:     [[TMP13:%.*]] = or i32 [[TMP6]], [[TMP12]]
; CECK-NEXT:     [[TMP14:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CECK-NEXT:     [[TMP15:%.*]] = zext i16 [[TMP14]] to i32
; CECK-NEXT:     [[TMP16:%.*]] = call i32 @llvm.genx.GenISA.firstbitLo(i32 [[TMP9]])
; CECK-NEXT:     [[TMP17:%.*]] = icmp eq i32 [[TMP16]], [[TMP15]]
; CECK-NEXT:     [[TMP18:%.*]] = zext i1 [[TMP17]] to i32
; CECK-NEXT:     [[TMP19:%.*]] = or i32 [[TMP13]], [[TMP18]]
; CECK-NEXT:     [[TMP20:%.*]] = uitofp i32 [[TMP19]] to float
; CECK-NEXT:     [[TMP21:%.*]] = fmul fast float [[TMP20]], 1.562500e-02
; CECK-NEXT:     call void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource.0 addrspace(2490368)* [[U0]], i32 [[THREADX]], i32 [[THREADY]], i32 0, i32 0, float [[TMP21]], float 1.562500e-02, float 7.812500e-03, float 1.000000e+00)
; CECK-NEXT:     ret void
;
  %1 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %1 to i32
  %2 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
  %GroupID_Y = bitcast float %2 to i32
  %3 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  %u0 = inttoptr i32 %3 to %__2D_DIM_Resource addrspace(2490368)*
  %4 = shl i32 %GroupID_X, 4
  %LocalID_X = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ThreadID_X = add i32 %4, %LocalID_X
  %5 = shl i32 %GroupID_Y, 4
  %LocalID_Y = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
  %ThreadID_Y = add i32 %5, %LocalID_Y
  %6 = and i32 %ThreadID_X, 1
  %7 = call i16 @llvm.genx.GenISA.simdLaneId()
  %8 = zext i16 %7 to i32
  %9 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %10 = call i32 @llvm.genx.GenISA.firstbitLo(i32 %9)
  %11 = icmp eq i32 %10, %8
  %12 = zext i1 %11 to i32
  %13 = or i32 %6, %12
  %14 = call i16 @llvm.genx.GenISA.simdLaneId()
  %15 = zext i16 %14 to i32
  %16 = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  %17 = call i32 @llvm.genx.GenISA.firstbitLo(i32 %16)
  %18 = icmp eq i32 %17, %15
  %19 = zext i1 %18 to i32
  %20 = or i32 %13, %19
  %21 = uitofp i32 %20 to float
  %22 = fmul fast float %21, 1.562500e-02
  call void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)* %u0, i32 %ThreadID_X, i32 %ThreadID_Y, i32 0, i32 0, float %22, float 1.562500e-02, float 7.812500e-03, float 1.000000e+00)
  ret void
}

declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32)
declare i16 @llvm.genx.GenISA.simdLaneId()
declare i32 @llvm.genx.GenISA.WaveBallot(i1, i32)
declare i32 @llvm.genx.GenISA.firstbitLo(i32)
declare void @llvm.genx.GenISA.typedwrite.p2490368__2D_DIM_Resource(%__2D_DIM_Resource addrspace(2490368)*, i32, i32, i32, i32, float, float, float, float)
declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32)
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32)