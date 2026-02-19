;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey CodeSinkingLoadSchedulingInstr=1 --regkey LoopSinkMinSave=1 --regkey ForceLoadsLoopSink=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s | FileCheck %s
define void @foo(<2 x double> addrspace(3)* %in0, double addrspace(3)* %in1, double addrspace(3)* noalias %out0, i32 %count, i32 %offsetIn0, i32 %offsetIn2) {
; CHECK-LABEL: @foo(
;
; CHECK-NEXT:  entry:
entry:
; CHECK-NEXT:    [[IN0_SHIFTED:%.*]] = getelementptr <2 x double>, <2 x double> addrspace(3)* [[IN0:%.*]], i32 [[OFFSETIN0:%.*]]
  %in0_shifted = getelementptr <2 x double>, <2 x double> addrspace(3)* %in0, i32 %offsetIn0
; CHECK-NEXT:    [[IN2_SHIFTED:%.*]] = getelementptr <2 x double>, <2 x double> addrspace(3)* [[IN0]], i32 [[OFFSETIN2:%.*]]
  %in2_shifted = getelementptr <2 x double>, <2 x double> addrspace(3)* %in0, i32 %offsetIn2
  %in0_add = add i32 %offsetIn0, 10
  %in0_shifted_for_store = getelementptr <2 x double>, <2 x double> addrspace(3)* %in0, i32 %in0_add
  %in0_bc_for_store = bitcast <2 x double> addrspace(3)* %in0_shifted_for_store to double addrspace(3)*
  br label %entry_preheader

; CHECK:       entry_preheader:
entry_preheader:                                  ; preds = %entry
  %l0 = load <2 x double>, <2 x double> addrspace(3)* %in0_shifted, align 16
  %l0e0 = extractelement <2 x double> %l0, i32 0
  %l0e1 = extractelement <2 x double> %l0, i32 1
; CHECK:         [[L1:%.*]] = load double, double addrspace(3)* %[[_:.*]], align 8
  %l1 = load double, double addrspace(3)* %in1, align 8
  store double 5.6, double addrspace(3)* %out0, align 8
; check not sinked
; CHECK:         [[L2:%.*]] = load <2 x double>, <2 x double> addrspace(3)* [[IN2_SHIFTED]], align 16
  %l2 = load <2 x double>, <2 x double> addrspace(3)* %in2_shifted, align 16
  %l2e0 = extractelement <2 x double> %l2, i32 0
  %l2e1 = extractelement <2 x double> %l2, i32 1
  br label %loop

; CHECK:       loop:
loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
; CHECK:         store double 8.600000e+00, double addrspace(3)* [[IN0_BC_FOR_STORE:%.*]], align 8
  store double 8.6, double addrspace(3)* %in0_bc_for_store, align 8

; check that sinked here
; CHECK:         [[L0:%.*]] = load <2 x double>, <2 x double> addrspace(3)* [[IN0_SHIFTED]], align 16

; CHECK:         [[L0E0:%.*]] = extractelement <2 x double> [[L0]], i32 0
; CHECK:         [[L0E1:%.*]] = extractelement <2 x double> [[L0]], i32 1

  %a0 = fadd double %l0e0, 2.000000e+00
  %a1 = fadd double %l1, 2.000000e+00
  %a3 = fadd double %l0e1, 2.000000e+00
  %a4 = fadd double %l2e0, 2.000000e+00
  %a5 = fadd double %l2e1, 4.000000e+00
  %combine = fadd double %a0, %a1
  %combine2 = fadd double %a3, %combine
  %combine3 = fadd double %a4, %a5
  %toStore = fadd double %combine2, %combine3
  %out0_shifted = getelementptr double, double addrspace(3)* %out0, i32 %index
  store double %toStore, double addrspace(3)* %out0_shifted, align 8
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}

!igc.functions = !{}
