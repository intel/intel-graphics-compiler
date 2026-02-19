;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey LoopSinkMinSave=1 --regkey LoopSinkMinSaveUniform=3 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-wi-analysis --igc-code-loop-sinking -S %s | FileCheck %s
; We set LoopSinkMinSaveUniform=3, and check that only the case with saving 3 scalars is being sinked, when all the values are uniform
define spir_kernel void @foo(float addrspace(1)* %in0, float addrspace(1)* %in1, float addrspace(1)* %out0, i32 %count, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
; CHECK-LABEL: @foo(
; CHECK:  entry:
; CHECK:    [[ADDR_1:%.*]] = getelementptr float, float addrspace(1)* [[IN0:%.*]], i32 0
; CHECK:    [[ADDR_2:%.*]] = getelementptr float, float addrspace(1)* [[IN1:%.*]], i32 0
; CHECK:    br label [[ENTRY_PREHEADER:%.*]]
; CHECK:  entry_preheader:
; CHECK:    [[L_1:%.*]] = load float, float addrspace(1)* [[ADDR_1]], align 16
; CHECK:    [[L_2:%.*]] = load float, float addrspace(1)* [[ADDR_2]], align 16
; not sinked
; CHECK:    [[ADDF2_1:%.*]] = fadd float [[L_2]], 1.000000e+00
; CHECK:    [[ADDF2_2:%.*]] = fadd float [[L_2]], 2.000000e+00
; CHECK:    [[ADDF2_3:%.*]] = fadd float [[L_2]], 3.000000e+00
; CHECK:  loop:
; sinked:
; CHECK:    [[ADDF1_1:%.*]] = fadd float [[L_1]], 1.000000e+00
; CHECK:    [[ADDF1_2:%.*]] = fadd float [[L_1]], 2.000000e+00
; CHECK:    [[ADDF1_3:%.*]] = fadd float [[L_1]], 3.000000e+00
; CHECK:    [[ADDF1_4:%.*]] = fadd float [[L_1]], 4.000000e+00
; CHECK:  afterloop:
; CHECK:    ret void
;
entry:
  %addr_1 = getelementptr float, float addrspace(1)* %in0, i32 0
  %addr_2 = getelementptr float, float addrspace(1)* %in1, i32 0
  br label %entry_preheader

entry_preheader:                                  ; preds = %entry
  %l_1 = load float, float addrspace(1)* %addr_1, align 16
  %l_2 = load float, float addrspace(1)* %addr_2, align 16
  %addf1_1 = fadd float %l_1, 1.0
  %addf1_2 = fadd float %l_1, 2.0
  %addf1_3 = fadd float %l_1, 3.0
  %addf1_4 = fadd float %l_1, 4.0
  %addf2_1 = fadd float %l_2, 1.0
  %addf2_2 = fadd float %l_2, 2.0
  %addf2_3 = fadd float %l_2, 3.0
  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  %addff_1 = fadd float %addf1_1, %addf2_1
  %addff_2 = fadd float %addff_1, %addf1_2
  %addff_3 = fadd float %addff_2, %addf1_3
  %addff_4 = fadd float %addff_3, %addf1_4
  %addff_5 = fadd float %addff_4, %addf2_2
  %res = fadd float %addff_5, %addf2_3

  %out0_shifted = getelementptr float, float addrspace(1)* %out0, i32 %index
  store float %res, float addrspace(1)* %out0_shifted, align 8
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
