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
  ; verify that two scalar instructions with consecutive offsets are not changed
  ; to vector access
  %0 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 0)
  ; CHECK: %0 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 0)
  call void @foo(i32 %0)
  %1 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  ; CHECK: %1 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 1)
  call void @foo(i32 %1)

  ; verify that the vector instruction is not affected by the scalar 
  ; instructions with offsets immediately beofre and after
  %2 = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 2)
  %3 = extractelement <2 x i32> %2, i32 %idx
  ; CHECK: %2 = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 2)
  ; CHECK-NEXT: %3 = extractelement <2 x i32> %2, i32 %idx
  call void @foo(i32 %3)
  %4 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
  ; CHECK: %4 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
  call void @foo(i32 %4)

  ; verify that the two vector instructions with consecutive offsets are not 
  ; modified
  %5 = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 6)
  %6 = extractelement <2 x i32> %5, i32 %idx
  ; CHECK: %5 = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 6)
  ; CHECK-NEXT: %6 = extractelement <2 x i32> %5, i32 %idx
  call void @foo(i32 %6)
  %7 = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 8)
  %8 = extractelement <2 x i32> %7, i32 %idx
  ; CHECK: %7 = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 8)
  ; CHECK-NEXT: %8 = extractelement <2 x i32> %7, i32 %idx
  call void @foo(i32 %6)

  ; verify that overlapping vector instructions are merged
  %9 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 10)
  %10 = extractelement <4 x i32> %9, i32 %idx
  call void @foo(i32 %10)
  ; CHECK: [[VALUE0:%[a-zA-Z0-9_.%-]+]] = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 10)
  ; CHECK-NEXT: [[VALUE1:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE0]], i32 %idx
  ; CHECK-NEXT: call void @foo(i32 [[VALUE1]])
  %11 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 12)
  %12 = extractelement <4 x i32> %11, i32 %idx
  call void @foo(i32 %12)
  ; CHECK: [[VALUE2:%[a-zA-Z0-9_.%-]+]] = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 10)
  ; CHECK-NEXT: [[VALUE3:%[a-zA-Z0-9_.%-]+]] = add i32 %idx, 2
  ; CHECK-NEXT: [[VALUE4:%[a-zA-Z0-9_.%-]+]] = extractelement <6 x i32> [[VALUE2]], i32 [[VALUE3]]
  ; CHECK-NEXT: call void @foo(i32 [[VALUE4]])
  
  ; verify that overlapping scalar and vector instructions are merged
  %13 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 16)
  %14 = extractelement <4 x i32> %13, i32 %idx
  call void @foo(i32 %14)
  ; CHECK: [[VALUE5:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 16)
  ; CHECK-NEXT: [[VALUE6:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE5]], i32 %idx
  ; CHECK-NEXT: call void @foo(i32 [[VALUE6]])
  %15 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 17)
  call void @foo(i32 %15)
  ; CHECK: [[VALUE7:%[a-zA-Z0-9_.%-]+]] = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 16)
  ; CHECK-NEXT: [[VALUE8:%[a-zA-Z0-9_.%-]+]] = extractelement <4 x i32> [[VALUE7]], i32 1
  ; CHECK-NEXT: call void @foo(i32 [[VALUE8]])

  ret void
}

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

; Function Attrs: nounwind readnone
declare <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32) #1

; Function Attrs: nounwind readnone
declare <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32) #1

declare <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32) #1

; Function Attrs: noduplicate nounwind
declare void @foo(i32) #2

attributes #0 = { "null-pointer-is-valid"="true" }
attributes #1 = { nounwind readnone }
attributes #2 = { noduplicate nounwind }

