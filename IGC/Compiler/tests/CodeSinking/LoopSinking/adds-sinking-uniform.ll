;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey LoopSinkMinSave=1 --regkey ForceLoadsLoopSink=1 --regkey LoopSinkMinSaveUniform=10 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-wi-analysis --igc-code-loop-sinking -S %s | FileCheck %s
; We set LoopSinkMinSaveUniform=10, but in this test uniform vs non-uniform results in sinking
define spir_kernel void @foo(float addrspace(1)* %in0, float addrspace(1)* %in1, float addrspace(1)* %out0, i32 %count, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
; CHECK-LABEL: @foo(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[LOCALIDX32:%.*]] = zext i16 [[LOCALIDX:%.*]] to i32
; CHECK-NEXT:    [[NON_UNIFORM_ADDR_1:%.*]] = getelementptr float, float addrspace(1)* [[IN0:%.*]], i32 [[LOCALIDX32]]
; CHECK-NEXT:    [[UNIFORM_ADDR_2:%.*]] = getelementptr float, float addrspace(1)* [[IN1:%.*]], i32 0

; CHECK:       entry_preheader:
; CHECK-NEXT:    [[NON_UNIFORM_LOAD_1:%.*]] = load float, float addrspace(1)* [[NON_UNIFORM_ADDR_1]], align 16
; CHECK-NEXT:    [[UNIFORM_LOAD_2:%.*]] = load float, float addrspace(1)* [[UNIFORM_ADDR_2]], align 16

; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:

; this ADDF_1 should be sinked, it becomes free as l_1 is already used in the loop, so we decrease the pressure
; CHECK:         [[ADDF_1:%.*]] = fadd float [[NON_UNIFORM_LOAD_1]], 1.000000e+00

; this ADDFF_1 should be sinked:
; it's i32,i32->i32, but the only parameter that is not used in the loop, is uniform
; and the fadd is not, so we remove register pressure by sinking it

; It wouldn't be sinked if didn't prove it's uniform
; CHECK:         [[ADDFF_1:%.*]] = fadd float [[ADDF_1]], [[UNIFORM_LOAD_2]]

; CHECK:       afterloop:
;
entry:
  %localIdX32 = zext i16 %localIdX to i32
  %addr_1 = getelementptr float, float addrspace(1)* %in0, i32 %localIdX32
  %addr_2 = getelementptr float, float addrspace(1)* %in1, i32 0
  br label %entry_preheader

entry_preheader:                                  ; preds = %entry
  %l_1 = load float, float addrspace(1)* %addr_1, align 16
  %l_2 = load float, float addrspace(1)* %addr_2, align 16
  %addf_1 = fadd float %l_1, 1.0
  %addff_1 = fadd float %addf_1, %l_2
  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  %addf_2 = fadd float %l_1, 2.0
  %acc0 = fadd float %addf_1, %addf_2
  %acc1 = fadd float %addf_2, %addff_1

  %out0_shifted = getelementptr float, float addrspace(1)* %out0, i32 %index
  store float %acc1, float addrspace(1)* %out0_shifted, align 8
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}

!IGCMetadata = !{!2}
!igc.functions = !{!13}

!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, i32, i16, i16, i16)* @foo}
!5 = !{!"FuncMDValue[0]", !6, !7, !11, !12}
!6 = !{!"localOffsets"}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"funcArgs"}
!12 = !{!"functionType", !"KernelFunction"}
!13 = !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, i32, i16, i16, i16)* @foo, !14}
!14 = !{!15, !16}
!15 = !{!"function_type", i32 0}
!16 = !{!"implicit_arg_desc", !17, !18, !19, !20, !21}
!17 = !{i32 0}
!18 = !{i32 1}
!19 = !{i32 8}
!20 = !{i32 9}
!21 = !{i32 10}
