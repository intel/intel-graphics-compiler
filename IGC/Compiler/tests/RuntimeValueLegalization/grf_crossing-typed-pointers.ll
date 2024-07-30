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
  ; verify that the vector instruction is GRF aligned
  %0 = call <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32 7)
  %1 = extractelement <2 x i32> %0, i32 %idx
  call void @foo(i32 %1)
  ; CHECK: [[VALUE2:%[a-zA-Z0-9_.%-]+]] = call <9 x i32> @llvm.genx.GenISA.RuntimeValue.v9i32(i32 0)
  ; CHECK-NEXT:[[VALUE3:%[a-zA-Z0-9_.%-]+]] = add i32 %idx, 7
  ; CHECK-NEXT [[VALUE4:%[a-zA-Z0-9_.%-]+]] = extractelement <9 x i32> %[[VALUE3]], i32 %idx
  ; CHECK-NEXT call void @foo(i32 [[VALUE4]])  
  call void @foo(i32 %1)

  ; verify that overlapping scalar and vector instructions are merged and GRF aligned
  %2 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 22)
  %3 = extractelement <4 x i32> %2, i32 %idx
  call void @foo(i32 %3)
  ; CHECK: [[VALUE5:%[a-zA-Z0-9_.%-]+]] = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 16)
  ; CHECK-NEXT: [[VALUE6:%[a-zA-Z0-9_.%-]+]] = add i32 %idx, 6
  ; CHECK-NEXT: [[VALUE7:%[a-zA-Z0-9_.%-]+]] = extractelement <10 x i32> [[VALUE5]], i32 [[VALUE6]]
  ; CHECK-NEXT: call void @foo(i32 [[VALUE7]])
  %4 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 16)
  call void @foo(i32 %4)
  ; CHECK: [[VALUE8:%[a-zA-Z0-9_.%-]+]] = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 16)
  ; CHECK-NEXT: [[VALUE9:%[a-zA-Z0-9_.%-]+]] = extractelement <10 x i32> [[VALUE8]], i32 0
  ; CHECK-NEXT: call void @foo(i32 [[VALUE9]])

  ; verify that overlapping vector instructions are merged and GRF aligned
  %5 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 38)
  %6 = extractelement <4 x i32> %5, i32 %idx
  call void @foo(i32 %6)
  ; CHECK: [[VALUE10:%[a-zA-Z0-9_.%-]+]] = call <18 x i32> @llvm.genx.GenISA.RuntimeValue.v18i32(i32 32)
  ; CHECK-NEXT: [[VALUE11:%[a-zA-Z0-9_.%-]+]] = add i32 %idx, 6
  ; CHECK-NEXT: [[VALUE12:%[a-zA-Z0-9_.%-]+]] = extractelement <18 x i32> [[VALUE10]], i32 [[VALUE11]]
  ; CHECK-NEXT: call void @foo(i32 [[VALUE12]])
  %7 = call <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32 46)
  %8 = extractelement <4 x i32> %7, i32 %idx
  call void @foo(i32 %8)
  ; CHECK: [[VALUE13:%[a-zA-Z0-9_.%-]+]] = call <18 x i32> @llvm.genx.GenISA.RuntimeValue.v18i32(i32 32)
  ; CHECK-NEXT: [[VALUE14:%[a-zA-Z0-9_.%-]+]] = add i32 %idx, 14
  ; CHECK-NEXT: [[VALUE15:%[a-zA-Z0-9_.%-]+]] = extractelement <18 x i32> [[VALUE13]], i32 [[VALUE14]]
  ; CHECK-NEXT: call void @foo(i32 [[VALUE15]])
  
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

; Function Attrs: nounwind readnone
declare <2 x i32> @llvm.genx.GenISA.RuntimeValue.v2i32(i32) #1

; Function Attrs: nounwind readnone
declare <4 x i32> @llvm.genx.GenISA.RuntimeValue.v4i32(i32) #1

; Function Attrs: nounwind readnone
declare <9 x i32> @llvm.genx.GenISA.RuntimeValue.v9i32(i32) #1

; Function Attrs: nounwind readnone
declare <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32) #1

; Function Attrs: nounwind readnone
declare <18 x i32> @llvm.genx.GenISA.RuntimeValue.v18i32(i32) #1

; Function Attrs: noduplicate nounwind
declare void @foo(i32) #2

attributes #0 = { "null-pointer-is-valid"="true" }
attributes #1 = { nounwind readnone }
attributes #2 = { noduplicate nounwind }

