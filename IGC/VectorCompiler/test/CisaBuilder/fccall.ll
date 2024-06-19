;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s

; Function Attrs: nounwind
declare !spirv.ParameterDecorations !17 spir_func void @AlphaSrcBlendG(<12 x i16>, <7 x i8>, i8 zeroext, i16 zeroext, i16 zeroext, i32, i32, <2 x i32>, <4 x i8>, i8 zeroext, i8 zeroext, i8 zeroext, i8 zeroext, i8 zeroext, i8 zeroext, i8 zeroext, i8 zeroext, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <16 x i16>, <16 x i16>, <16 x i16>, <1536 x i16>, <16 x i16>, <12 x i16>, i16 zeroext, i16 zeroext, i8 zeroext, i8 zeroext, i8 zeroext, i8 zeroext) local_unnamed_addr #0

; Function Attrs: nounwind
declare void @llvm.genx.output.1.v12i16(<12 x i16>) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.v7i8(<7 x i8>) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.i8(i8) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.i16(i16) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.i32(i32) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.v2i32(<2 x i32>) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.v4i8(<4 x i8>) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.v8f32(<8 x float>) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.v16i16(<16 x i16>) #1

; Function Attrs: nounwind
declare void @llvm.genx.output.1.v1536i16(<1536 x i16>) #1

; Function Attrs: noinline nounwind
; CHECK: Call_AlphaSrcBlendG
define dllexport spir_kernel void @Call_AlphaSrcBlendG(<12 x i16> %PrimaryCscCoeff, <7 x i8> %ConstantBlendingAlpha, i8 zeroext %PointerToInlineParameters, i16 zeroext %Dst_Width, i16 zeroext %Dst_Height, i32 %WAFlag, i32 %RotationChromaSitingFlag, <2 x i32> %Reserved1, <4 x i8> %ColorFill, i8 zeroext %LumakeyLowThreshold, i8 zeroext %LumakeyHighThreshold, i8 zeroext %Reserved2, i8 zeroext %Reserved3, i8 zeroext %DestinationPackedYOffset, i8 zeroext %DestinationPackedUOffset, i8 zeroext %DestinationPackedVOffset, i8 zeroext %DestinationRGBFormat, <8 x float> %Delta_X, <8 x float> %Delta_Y, <8 x float> %Start_Y, <8 x float> %Start_X, <16 x i16> %Top_Left, <16 x i16> %Bottom_Right, <16 x i16> %TempMask0, <1536 x i16> %DataBuffer, <16 x i16> %TempMask, <12 x i16> %CscCoeff, i16 zeroext %DstX, i16 zeroext %DstY, i8 zeroext %Buffer_Index, i8 zeroext %Layer_Index, i8 zeroext %CalculationMask, i8 zeroext %ConstAlphaTemp, i64 %impl.arg.private.base) local_unnamed_addr #2 {
entry:
  %cmp.not = icmp eq i8 %Buffer_Index, 0
  br i1 %cmp.not, label %if.end, label %if.then

; CHECK: fccall {{.*}} AlphaSrcBlendG
if.then:                                          ; preds = %entry
  tail call spir_func void @AlphaSrcBlendG(<12 x i16> %PrimaryCscCoeff, <7 x i8> %ConstantBlendingAlpha, i8 zeroext %PointerToInlineParameters, i16 zeroext %Dst_Width, i16 zeroext %Dst_Height, i32 %WAFlag, i32 %RotationChromaSitingFlag, <2 x i32> %Reserved1, <4 x i8> %ColorFill, i8 zeroext %LumakeyLowThreshold, i8 zeroext %LumakeyHighThreshold, i8 zeroext %Reserved2, i8 zeroext %Reserved3, i8 zeroext %DestinationPackedYOffset, i8 zeroext %DestinationPackedUOffset, i8 zeroext %DestinationPackedVOffset, i8 zeroext %DestinationRGBFormat, <8 x float> %Delta_X, <8 x float> %Delta_Y, <8 x float> %Start_Y, <8 x float> %Start_X, <16 x i16> %Top_Left, <16 x i16> %Bottom_Right, <16 x i16> %TempMask0, <1536 x i16> %DataBuffer, <16 x i16> %TempMask, <12 x i16> %CscCoeff, i16 zeroext %DstX, i16 zeroext %DstY, i8 zeroext %Buffer_Index, i8 zeroext %Layer_Index, i8 zeroext %CalculationMask, i8 zeroext %ConstAlphaTemp) #1
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  tail call void @llvm.genx.output.1.v12i16(<12 x i16> %PrimaryCscCoeff)
  tail call void @llvm.genx.output.1.v7i8(<7 x i8> %ConstantBlendingAlpha)
  tail call void @llvm.genx.output.1.i8(i8 %PointerToInlineParameters)
  tail call void @llvm.genx.output.1.i16(i16 %Dst_Width)
  tail call void @llvm.genx.output.1.i16(i16 %Dst_Height)
  tail call void @llvm.genx.output.1.i32(i32 %WAFlag)
  tail call void @llvm.genx.output.1.i32(i32 %RotationChromaSitingFlag)
  tail call void @llvm.genx.output.1.v2i32(<2 x i32> %Reserved1)
  tail call void @llvm.genx.output.1.v4i8(<4 x i8> %ColorFill)
  tail call void @llvm.genx.output.1.i8(i8 %LumakeyLowThreshold)
  tail call void @llvm.genx.output.1.i8(i8 %LumakeyHighThreshold)
  tail call void @llvm.genx.output.1.i8(i8 %Reserved2)
  tail call void @llvm.genx.output.1.i8(i8 %Reserved3)
  tail call void @llvm.genx.output.1.i8(i8 %DestinationPackedYOffset)
  tail call void @llvm.genx.output.1.i8(i8 %DestinationPackedUOffset)
  tail call void @llvm.genx.output.1.i8(i8 %DestinationPackedVOffset)
  tail call void @llvm.genx.output.1.i8(i8 %DestinationRGBFormat)
  tail call void @llvm.genx.output.1.v8f32(<8 x float> %Delta_X)
  tail call void @llvm.genx.output.1.v8f32(<8 x float> %Delta_Y)
  tail call void @llvm.genx.output.1.v8f32(<8 x float> %Start_Y)
  tail call void @llvm.genx.output.1.v8f32(<8 x float> %Start_X)
  tail call void @llvm.genx.output.1.v16i16(<16 x i16> %Top_Left)
  tail call void @llvm.genx.output.1.v16i16(<16 x i16> %Bottom_Right)
  tail call void @llvm.genx.output.1.v16i16(<16 x i16> %TempMask0)
  tail call void @llvm.genx.output.1.v1536i16(<1536 x i16> %DataBuffer)
  tail call void @llvm.genx.output.1.v16i16(<16 x i16> %TempMask)
  tail call void @llvm.genx.output.1.v12i16(<12 x i16> %CscCoeff)
  tail call void @llvm.genx.output.1.i16(i16 %DstX)
  tail call void @llvm.genx.output.1.i16(i16 %DstY)
  tail call void @llvm.genx.output.1.i8(i8 %Buffer_Index)
  tail call void @llvm.genx.output.1.i8(i8 %Layer_Index)
  tail call void @llvm.genx.output.1.i8(i8 %CalculationMask)
  tail call void @llvm.genx.output.1.i8(i8 %ConstAlphaTemp)
  ret void
}

attributes #0 = { nounwind "CMCallable" }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMEntry" "CMGenxMain" "RequiresImplArgsBuffer" "oclrt"="1" }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!llvm.ident = !{!11, !11, !11}
!llvm.module.flags = !{!12}
!genx.kernel.internal = !{!13}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (<12 x i16>, <7 x i8>, i8, i16, i16, i32, i32, <2 x i32>, <4 x i8>, i8, i8, i8, i8, i8, i8, i8, i8, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <16 x i16>, <16 x i16>, <16 x i16>, <1536 x i16>, <16 x i16>, <12 x i16>, i16, i16, i8, i8, i8, i8, i64)* @Call_AlphaSrcBlendG, !"Call_AlphaSrcBlendG", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 96}
!8 = !{i32 136, i32 160, i32 167, i32 168, i32 170, i32 172, i32 176, i32 180, i32 188, i32 192, i32 193, i32 194, i32 195, i32 196, i32 197, i32 198, i32 199, i32 200, i32 256, i32 288, i32 320, i32 352, i32 384, i32 416, i32 448, i32 3520, i32 3552, i32 3576, i32 3578, i32 3580, i32 3581, i32 3582, i32 3583, i32 128}
!9 = !{i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3}
!10 = !{!"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !""}
!11 = !{!"Ubuntu clang version 14.0.6"}
!12 = !{i32 1, !"wchar_size", i32 4}
!13 = !{void (<12 x i16>, <7 x i8>, i8, i16, i16, i32, i32, <2 x i32>, <4 x i8>, i8, i8, i8, i8, i8, i8, i8, i8, <8 x float>, <8 x float>, <8 x float>, <8 x float>, <16 x i16>, <16 x i16>, <16 x i16>, <1536 x i16>, <16 x i16>, <12 x i16>, i16, i16, i8, i8, i8, i8, i64)* @Call_AlphaSrcBlendG, !14, !15, !4, !16}
!14 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!15 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33}
!16 = !{i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 255}
!17 = !{!4, !4, !18, !18, !18, !4, !4, !4, !4, !18, !18, !18, !18, !18, !18, !18, !18, !4, !4, !4, !4, !4, !4, !4, !4, !4, !4, !18, !18, !18, !18, !18, !18}
!18 = !{!19}
!19 = !{i32 38, i32 0}


