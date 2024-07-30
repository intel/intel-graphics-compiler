;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-runtimevalue-legalization-pass -S %s | FileCheck %s

define void @main(i32 %idx) #0 {
entry:
  ; verify that overlapping float scalar and int vector instructions are merged
  %0 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 0)
  %1 = extractelement <4 x i32> %0, i32 %idx
  call void @foo(i32 %1)
  ; CHECK: [[VALUE5:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 0)
  ; CHECK-NEXT: [[VALUE6:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE5]], i32 %idx
  ; CHECK-NEXT: call void @foo(i32 [[VALUE6]])
  %2 = call float @llvm.genx.GenISA.RuntimeValue.float(i32 1)
  call void @bar(float %2)
  ; CHECK: [[VALUE7:%[a-zA-Z0-9_.%-]+]] = call <4 x float> @llvm.genx.GenISA.RuntimeValue.v4f32(i32 0)
  ; CHECK-NEXT: [[VALUE8:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x float> [[VALUE7]], i32 1
  ; CHECK-NEXT: call void @bar(float [[VALUE8]])

  ; verify that overlapping i64 scalar and int vector instructions are merged
  %3 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 4)
  %4 = extractelement <4 x i32> %3, i32 %idx
  call void @foo(i32 %4)
  ; CHECK: [[VALUE9:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 4)
  ; CHECK-NEXT: [[VALUE10:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE9]], i32 %idx
  ; CHECK-NEXT: call void @foo(i32 [[VALUE10]])
  %5 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 4)
  call void @baz(i64 %5)
  ; CHECK: [[VALUE11:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 4)
  ; CHECK-NEXT: [[VALUE12:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE11]], i32 0
  ; CHECK-NEXT: [[VALUE13:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> undef, i32 [[VALUE12]], i32 0
  ; CHECK-NEXT: [[VALUE14:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE11]], i32 1
  ; CHECK-NEXT: [[VALUE15:%[a-zA-Z0-9_.%-]+]] = insertelement <2 x i32> [[VALUE13]], i32 [[VALUE14]], i32 1
  ; CHECK-NEXT: [[VALUE16:%[a-zA-Z0-9_.%-]+]] = bitcast <2 x i32> [[VALUE15]] to i64
  ; CHECK-NEXT: call void @baz(i64 [[VALUE16]])

  ret void
}

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

; Function Attrs: nounwind readnone
declare i64 @llvm.genx.GenISA.RuntimeValue.i64(i32) #1

; Function Attrs: nounwind readnone
declare float @llvm.genx.GenISA.RuntimeValue.float(i32) #1

; Function Attrs: nounwind readnone
declare <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32) #1

; Function Attrs: nounwind readnone
declare <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32) #1

; Function Attrs: nounwind readnone
declare <4 x float> @llvm.genx.GenISA.RuntimeValue.v4f32(i32) #1

; Function Attrs: noduplicate nounwind
declare void @foo(i32) #2

; Function Attrs: noduplicate nounwind
declare void @bar(float) #2

; Function Attrs: noduplicate nounwind
declare void @baz(i64) #2

attributes #0 = { "null-pointer-is-valid"="true" }
attributes #1 = { nounwind readnone }
attributes #2 = { noduplicate nounwind }

