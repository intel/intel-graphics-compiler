;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-runtimevalue-legalization-pass -S %s | FileCheck %s

define void @entry(<8 x i32> %r0, i8* %privateBase) #0 {
GlobalScopeInitialization:
  %0 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 20)
  %Temp-24.i = icmp ult i32 %0, 2
  br i1 %Temp-24.i, label %Label-26.i, label %GlobalScopeInitialization.Output_crit_edge

GlobalScopeInitialization.Output_crit_edge:       ; preds = %GlobalScopeInitialization
  br label %Output

Label-26.i:                                       ; preds = %GlobalScopeInitialization
  %1 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
; CHECK: [[RVRESULT:%[a-zA-Z0-9_.%-]+]] = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 0)
; CHECK-NEXT: [[RES:%[a-zA-Z0-9_.%-]+]] = extractelement <10 x i32> [[RVRESULT]], i32 4
  %2 = bitcast i32 %1 to float
  %Temp-33.i = fcmp fast olt float %2, 2.000000e+00
  br i1 %Temp-33.i, label %Label-35.i, label %Label-26.i.Output_crit_edge

Label-26.i.Output_crit_edge:                      ; preds = %Label-26.i
  br label %Output

Label-35.i:                                       ; preds = %Label-26.i
  %3 = and i32 %0, 1073741823
  %4 = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 4)
  %5 = extractelement <6 x i32> %4, i32 %3
; CHECK: [[IDX0:%[a-zA-Z0-9_.%-]+]] = and i32 %0, 1073741823
; CHECK-NEXT: [[RVRESULT:%[a-zA-Z0-9_.%-]+]] = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 0)
; CHECK-NEXT: [[IDX1:%[a-zA-Z0-9_.%-]+]] = add i32 [[IDX0]], 4
; CHECK-NEXT: [[RES:%[a-zA-Z0-9_.%-]+]] = extractelement <10 x i32> [[RVRESULT]], i32 [[IDX1]]
  %6 = bitcast i32 %5 to float
  %7 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 5)
; CHECK: [[RVRESULT:%[a-zA-Z0-9_.%-]+]] = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 0)
; CHECK-NEXT: [[RES:%[a-zA-Z0-9_.%-]+]] = extractelement <10 x i32> [[RVRESULT]], i32 5
  %8 = bitcast i32 %7 to float
  %9 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 6)
  %10 = bitcast i32 %9 to float
  br label %Output

Output:                                           ; preds = %Label-26.i.Output_crit_edge, %GlobalScopeInitialization.Output_crit_edge, %Label-35.i
  %.01 = phi float [ %6, %Label-35.i ], [ 1.000000e+00, %Label-26.i.Output_crit_edge ], [ 1.000000e+00, %GlobalScopeInitialization.Output_crit_edge ]
  %.02 = phi float [ %2, %Label-35.i ], [ 0.000000e+00, %Label-26.i.Output_crit_edge ], [ 0.000000e+00, %GlobalScopeInitialization.Output_crit_edge ]
  %.03 = phi float [ %8, %Label-35.i ], [ 0.000000e+00, %Label-26.i.Output_crit_edge ], [ 0.000000e+00, %GlobalScopeInitialization.Output_crit_edge ]
  %.04 = phi float [ %10, %Label-35.i ], [ 1.000000e+00, %Label-26.i.Output_crit_edge ], [ 1.000000e+00, %GlobalScopeInitialization.Output_crit_edge ]
  call void @llvm.genx.GenISA.OUTPUT.f32(float %.01, float %.02, float %.03, float %.04, i32 0, i32 0)
  ret void
}

; Function Attrs: noduplicate nounwind
declare void @llvm.genx.GenISA.OUTPUT.f32(float, float, float, float, i32, i32) #2

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.GenISA.RuntimeValue.i32(i32) #1

; Function Attrs: nounwind readnone
declare <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32) #1

attributes #0 = { "null-pointer-is-valid"="true" }
attributes #1 = { nounwind readnone }
attributes #2 = { noduplicate nounwind }

