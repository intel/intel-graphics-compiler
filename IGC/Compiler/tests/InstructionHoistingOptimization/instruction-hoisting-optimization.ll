;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt -platformbmg --typed-pointers --regkey EnableInstructionHoistingOptimization --igc-instruction-hoisting-optimization -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; EnableInstructionHoistingOptimization:
; ------------------------------------------------

%__2D_DIM_Resource = type opaque

define void @test() {
; CHECK-LABEL: @test(
; CHECK-NEXT: entry:
; CHECK-NEXT:  [[TMP0:%.*]] = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
; CHECK-NEXT:  [[GROUPX:%.*]] = bitcast float [[TMP0]] to i32
; CHECK-NEXT:  [[TMP1:%.*]] = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
; CHECK-NEXT:  [[GROUPY:%.*]] = bitcast float [[TMP1]] to i32
; CHECK-NEXT:  [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
; CHECK-NEXT:  [[TMP3:%.*]] = add i32 [[TMP2]], 128
; CHECK-NEXT:  [[U0_6:%.*]] = inttoptr i32 [[TMP3]] to %__2D_DIM_Resource.0 addrspace(2490369)*
; CHECK-NEXT:  [[TMP4:%.*]] = add i32 [[TMP2]], 64
; CHECK-NEXT:  [[U0_61:%.*]] = inttoptr i32 [[TMP4]] to %__2D_DIM_Resource.0 addrspace(2490368)*
; CHECK-NEXT:  [[TMP5:%.*]] = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
; CHECK-NEXT:  [[TMP6:%.*]] = add i32 [[TMP5]], 384
; CHECK-NEXT:  [[T0_1:%.*]] = inttoptr i32 [[TMP6]] to <4 x float> addrspace(2621442)*
; CHECK-NEXT:  [[TMP7:%.*]] = add i32 [[TMP5]], 640
; CHECK-NEXT:  [[T0_12:%.*]] = inttoptr i32 [[TMP7]] to %__2D_DIM_Resource.0 addrspace(2621446)*
; CHECK-NEXT:  [[B0_1:%.*]] = inttoptr i32 [[TMP5]] to <4 x float> addrspace(2555904)*
; CHECK-NEXT:  [[TMP8:%.*]] = shl i32 [[GROUPX]], 3
; CHECK-NEXT:  [[LOCALIDX3:%.*]] = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
; CHECK-NEXT:  [[THREADIDX:%.*]] = add i32 [[TMP8]], [[LOCALIDX3]]
; CHECK-NEXT:  [[TMP9:%.*]] = shl i32 [[GROUPY]], 3
; CHECK-NEXT:  [[LOCALIDY4:%.*]] = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
; CHECK-NEXT:  [[THREADIDY:%.*]] = add i32 [[TMP9]], [[LOCALIDY4]]
; CHECK-NEXT:  [[TMP10:%.*]] = shl i32 [[LOCALIDY4]], 2
; CHECK-NEXT:  [[TMP11:%.*]] = lshr i32 [[LOCALIDX3]], 1
; CHECK-NEXT:  [[TMP12:%.*]] = add i32 [[TMP10]], [[TMP11]]
; CHECK-NEXT:  [[TMP13:%.*]] = shl i32 [[THREADIDX]], 2
; CHECK-NEXT:  [[TMP14:%.*]] = shl i32 [[THREADIDY]], 2
; CHECK-NEXT:  [[TMP15:%.*]] = uitofp i32 [[TMP14]] to float
; CHECK-NEXT:  [[TMP16:%.*]] = fadd fast float [[TMP15]], 5.000000e-01
; CHECK-NEXT:  [[TMP17:%.*]] = call fast <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904v4f32(<4 x float> addrspace(2555904)* [[B0_1]], i32 832, i32 4, i1 false)
; CHECK-NEXT:  [[TMP18:%.*]] = extractelement <4 x float> [[TMP17]], i64 0
; CHECK-NEXT:  [[TMP19:%.*]] = extractelement <4 x float> [[TMP17]], i64 1
; CHECK-NEXT:  [[TMP20:%.*]] = fmul fast float [[TMP19]], [[TMP16]]
; CHECK-NEXT:  [[TMP21:%.*]] = or i32 [[TMP14]], 1
; CHECK-NEXT:  [[TMP22:%.*]] = uitofp i32 [[TMP21]] to float
; CHECK-NEXT:  [[TMP23:%.*]] = fadd fast float [[TMP22]], 5.000000e-01
; CHECK-NEXT:  [[TMP24:%.*]] = fmul fast float [[TMP19]], [[TMP23]]
; CHECK-NEXT:  [[TMP25:%.*]] = or i32 [[TMP14]], 2
; CHECK-NEXT:  [[TMP26:%.*]] = uitofp i32 [[TMP25]] to float
; CHECK-NEXT:  [[TMP27:%.*]] = fadd fast float [[TMP26]], 5.000000e-01
; CHECK-NEXT:  [[TMP28:%.*]] = fmul fast float [[TMP19]], [[TMP27]]
; CHECK-NEXT:  [[TMP29:%.*]] = or i32 [[TMP14]], 3
; CHECK-NEXT:  [[TMP30:%.*]] = uitofp i32 [[TMP29]] to float
; CHECK-NEXT:  [[TMP31:%.*]] = fadd fast float [[TMP30]], 5.000000e-01
; CHECK-NEXT:  [[TMP32:%.*]] = fmul fast float [[TMP19]], [[TMP31]]
; CHECK-NEXT:  [[TMP33:%.*]] = uitofp i32 [[TMP13]] to float
; CHECK-NEXT:  [[TMP34:%.*]] = fadd fast float [[TMP33]], 5.000000e-01
; CHECK-NEXT:  [[TMP35:%.*]] = fmul fast float [[TMP34]], [[TMP18]]
; CHECK-NEXT:  [[TMP36:%.*]] = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float [[TMP35]], float [[TMP20]], float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource.0 addrspace(2621446)* undef, %__2D_DIM_Resource.0 addrspace(2621446)* [[T0_12]], <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
; CHECK-NEXT:  [[TMP37:%.*]] = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float [[TMP35]], float [[TMP24]], float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource.0 addrspace(2621446)* undef, %__2D_DIM_Resource.0 addrspace(2621446)* [[T0_12]], <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
; CHECK-NEXT:  [[TMP38:%.*]] = extractelement <4 x float> [[TMP36]], i64 3
;
; We are done here, as the smpl (%71) is hoisted up at tmp37 after tmp36 and before tmp38
;
entry:
  %0 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 14)
  %GroupID_X = bitcast float %0 to i32
  %1 = call fast float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 15)
  %GroupID_Y = bitcast float %1 to i32
  %2 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %3 = add i32 %2, 128
  %"u0,6" = inttoptr i32 %3 to %__2D_DIM_Resource addrspace(2490369)*
  %4 = add i32 %2, 64
  %"u0,61" = inttoptr i32 %4 to %__2D_DIM_Resource addrspace(2490368)*
  %5 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  %6 = add i32 %5, 384
  %"t0,1" = inttoptr i32 %6 to <4 x float> addrspace(2621442)*
  %7 = add i32 %5, 640
  %"t0,12" = inttoptr i32 %7 to %__2D_DIM_Resource addrspace(2621446)*
  %"b0,1" = inttoptr i32 %5 to <4 x float> addrspace(2555904)*
  %8 = shl i32 %GroupID_X, 3
  %LocalID_X3 = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ThreadID_X = add i32 %8, %LocalID_X3
  %9 = shl i32 %GroupID_Y, 3
  %LocalID_Y4 = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 18)
  %ThreadID_Y = add i32 %9, %LocalID_Y4
  %10 = shl i32 %LocalID_Y4, 2
  %11 = lshr i32 %LocalID_X3, 1
  %12 = add i32 %10, %11
  %13 = shl i32 %ThreadID_X, 2
  %14 = shl i32 %ThreadID_Y, 2
  %15 = uitofp i32 %14 to float
  %16 = fadd fast float %15, 5.000000e-01
  %17 = call fast <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904v4f32(<4 x float> addrspace(2555904)* %"b0,1", i32 832, i32 4, i1 false)
  %18 = extractelement <4 x float> %17, i64 0
  %19 = extractelement <4 x float> %17, i64 1
  %20 = fmul fast float %19, %16
  %21 = or i32 %14, 1
  %22 = uitofp i32 %21 to float
  %23 = fadd fast float %22, 5.000000e-01
  %24 = fmul fast float %19, %23
  %25 = or i32 %14, 2
  %26 = uitofp i32 %25 to float
  %27 = fadd fast float %26, 5.000000e-01
  %28 = fmul fast float %19, %27
  %29 = or i32 %14, 3
  %30 = uitofp i32 %29 to float
  %31 = fadd fast float %30, 5.000000e-01
  %32 = fmul fast float %19, %31
  %33 = uitofp i32 %13 to float
  %34 = fadd fast float %33, 5.000000e-01
  %35 = fmul fast float %34, %18
  %36 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %20, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
  %37 = extractelement <4 x float> %36, i64 3
  %38 = fmul fast float %37, 2.550000e+02
  %39 = fptoui float %38 to i32
  %40 = mul i32 %39, 60
  %41 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %"t0,1", i32 %40, i32 4, i1 false)
  %42 = add i32 %40, 4
  %43 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %"t0,1", i32 %42, i32 4, i1 false)
  %44 = add i32 %40, 8
  %45 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %"t0,1", i32 %44, i32 4, i1 false)
  %46 = add i32 %40, 12
  %47 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %"t0,1", i32 %46, i32 4, i1 false)
  %48 = add i32 %40, 44
  %49 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %"t0,1", i32 %48, i32 4, i1 false)
  %50 = add i32 %40, 52
  %51 = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)* %"t0,1", i32 %50, i32 4, i1 false)
  %52 = icmp ne i32 %49, 0
  %53 = zext i1 %52 to i32
  %.not = icmp eq i32 %43, 0
  %54 = or i32 %53, 2
  %55 = select i1 %.not, i32 %53, i32 %54
  %.not5 = icmp eq i32 %47, 0
  %56 = or i32 %55, 4
  %57 = select i1 %.not5, i32 %55, i32 %56
  %58 = icmp ne i32 %51, 0
  %59 = or i32 %57, 8
  %60 = zext i1 %58 to i32
  %61 = select i1 %58, i32 %59, i32 %57
  %Pivot19 = icmp slt i32 %41, 1
  br i1 %Pivot19, label %LeafBlock, label %NodeBlock

NodeBlock:                                        ; preds = %entry
  %Pivot = icmp eq i32 %41, 1
  br i1 %Pivot, label %64, label %LeafBlock16

LeafBlock16:                                      ; preds = %NodeBlock
  %SwitchLeaf17 = icmp eq i32 %41, 2
  br i1 %SwitchLeaf17, label %68, label %NewDefault

LeafBlock:                                        ; preds = %entry
  %SwitchLeaf = icmp eq i32 %41, 0
  br i1 %SwitchLeaf, label %66, label %NewDefault

NewDefault:                                       ; preds = %LeafBlock, %LeafBlock16
  %.not6 = icmp eq i32 %41, -1
  %62 = or i32 %61, 16
  %63 = select i1 %.not6, i32 %61, i32 %62
  br label %NodeBlock27

64:                                               ; preds = %NodeBlock
  %65 = or i32 %61, 32
  br label %NodeBlock27

66:                                               ; preds = %LeafBlock
  %67 = or i32 %61, 128
  br label %NodeBlock27

68:                                               ; preds = %LeafBlock16
  %69 = or i32 %61, 64
  br label %NodeBlock27

NodeBlock27:                                      ; preds = %NewDefault, %64, %66, %68
  %70 = phi i32 [ %69, %68 ], [ %67, %66 ], [ %65, %64 ], [ %63, %NewDefault ]
  %71 = call fast <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float 0.000000e+00, float %35, float %24, float 0.000000e+00, float 0.000000e+00, %__2D_DIM_Resource addrspace(2621446)* undef, %__2D_DIM_Resource addrspace(2621446)* %"t0,12", <4 x float> addrspace(655360)* null, i32 0, i32 0, i32 0)
  %72 = extractelement <4 x float> %71, i64 3
  %73 = fmul fast float %72, 2.550000e+02
  %74 = fptoui float %73 to i32
  %75 = mul i32 %74, 60
  ret void
}

declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #0
declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32) #0
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.v4f32.p2555904v4f32(<4 x float> addrspace(2555904)*, i32, i32, i1) #1
declare <4 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v4i32.p2621442v4f32(<4 x float> addrspace(2621442)*, i32, i32, i1) #1
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p2621446__2D_DIM_Resource.p2621446__2D_DIM_Resource.p655360v4f32(float, float, float, float, float, %__2D_DIM_Resource addrspace(2621446)*, %__2D_DIM_Resource addrspace(2621446)*, <4 x float> addrspace(655360)*, i32, i32, i32) #0
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621442v4f32(<4 x float> addrspace(2621442)*, i32, i32, i1) #1
