;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey LoopSinkMinSave=4 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey CodeSinkingMinSize=10 --basic-aa --igc-code-sinking --igc-code-loop-sinking -S %s | FileCheck %s
define void @foo(float addrspace(1)* %in0, double addrspace(1)* %in1, float addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, i32 %offsetIn2) {
; CHECK-LABEL: @foo(
; CHECK:       entry:
; CHECK:         [[ADDR_1:%.*]] = getelementptr float, float addrspace(1)* [[IN0:%.*]], i32 0
; CHECK:         [[ADDR_3:%.*]] = getelementptr float, float addrspace(1)* [[IN0]], i32 2
; CHECK:         br label [[ENTRY_PREHEADER:%.*]]
; CHECK:       entry_preheader:
; CHECK:         [[L_1:%.*]] = load float, float addrspace(1)* [[ADDR_1]], align 16
; CHECK:         [[L_3:%.*]] = load float, float addrspace(1)* [[ADDR_3]], align 16

; this add is not beneficial to sink

; CHECK:         [[ADDFF_1:%.*]] = fadd float [[L_3]], 1.000000e+00
; CHECK:         br label [[LOOP:%.*]]
; CHECK:       loop:

; These 5 adds are beneficial to sink at once, because now only one value is alive in the loop (L_1), instead of 5

; CHECK:         [[INDEX:%.*]] = phi i32 [ 0, [[ENTRY_PREHEADER]] ], [ [[INC:%.*]], [[LOOP]] ]
; CHECK:         [[ADDF_1:%.*]] = fadd float [[L_1]], 1.000000e+00
; CHECK:         [[ADDF_2:%.*]] = fadd float [[L_1]], 2.000000e+00
; CHECK:         [[ACC0:%.*]] = fadd float [[ADDF_1]], [[ADDF_2]]
; CHECK:         [[ADDF_3:%.*]] = fadd float [[L_1]], 3.000000e+00
; CHECK:         [[ACC1:%.*]] = fadd float [[ACC0]], [[ADDF_3]]
; CHECK:         [[ADDF_4:%.*]] = fadd float [[L_1]], 4.000000e+00
; CHECK:         [[ACC2:%.*]] = fadd float [[ACC1]], [[ADDF_4]]
; CHECK:         [[ADDF_5:%.*]] = fadd float [[L_1]], 5.000000e+00
; CHECK:         [[ACC3:%.*]] = fadd float [[ACC2]], [[ADDF_5]]
; CHECK:         [[ACC4:%.*]] = fadd float [[ACC2]], [[ADDFF_1]]
; CHECK:         [[OUT0_SHIFTED:%.*]] = getelementptr float, float addrspace(1)* [[OUT0:%.*]], i32 [[INDEX]]
; CHECK:         store float [[ACC4]], float addrspace(1)* [[OUT0_SHIFTED]], align 8
; CHECK:         [[INC]] = add i32 [[INDEX]], 1
; CHECK:         [[CMPTMP:%.*]] = icmp ult i32 [[INDEX]], [[COUNT:%.*]]
; CHECK:         br i1 [[CMPTMP]], label [[LOOP]], label [[AFTERLOOP:%.*]]
; CHECK:       afterloop:
; CHECK:         ret void
;
entry:
  %addr_1 = getelementptr float, float addrspace(1)* %in0, i32 0
  %addr_3 = getelementptr float, float addrspace(1)* %in0, i32 2

  %l_1 = load float, float addrspace(1)* %addr_1, align 16
  %l_3 = load float, float addrspace(1)* %addr_3, align 16

  br label %entry_preheader

entry_preheader:                                  ; preds = %entry
  %addf_1 = fadd float %l_1, 1.0
  %addf_2 = fadd float %l_1, 2.0
  %addf_3 = fadd float %l_1, 3.0
  %addf_4 = fadd float %l_1, 4.0
  %addf_5 = fadd float %l_1, 5.0

  %addff_1 = fadd float %l_3, 1.0

  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  %acc0 = fadd float %addf_1, %addf_2
  %acc1 = fadd float %acc0, %addf_3
  %acc2 = fadd float %acc1, %addf_4
  %acc3 = fadd float %acc2, %addf_5

  %acc4 = fadd float %acc2, %addff_1

  %out0_shifted = getelementptr float, float addrspace(1)* %out0, i32 %index
  store float %acc4, float addrspace(1)* %out0_shifted, align 8
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}

!igc.functions = !{}
