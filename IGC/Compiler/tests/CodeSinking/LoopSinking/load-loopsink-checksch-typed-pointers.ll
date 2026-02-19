;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey CodeSinkingLoadSchedulingInstr=10 --regkey LoopSinkMinSave=1 --regkey ForceLoadsLoopSink=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking --verify -S %s | FileCheck %s
define void @foo(<2 x double> addrspace(3)* %in_out, double addrspace(3)* noalias %in1, i32 %count, i32 %offsetIn0) {
; CHECK-LABEL: @foo(
; CHECK:       entry:
; CHECK:         [[IN0_SHIFTED:%.*]] = getelementptr <2 x double>, <2 x double> addrspace(3)* [[IN0:%.*]], i32 [[OFFSETIN0:%.*]]
; CHECK:         [[IN0_AS_INT:%.*]] = ptrtoint <2 x double> addrspace(3)* [[IN0_SHIFTED]] to i32
; CHECK:         [[IN0_NEW:%.*]] = add i32 [[IN0_AS_INT]], 800
; CHECK:         br label [[ENTRY_PREHEADER:%.*]]
; CHECK:       entry_preheader:
; CHECK:         br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK:         [[INDEX:%.*]] = phi i32 [ 0, [[ENTRY_PREHEADER]] ], [ [[INC:%.*]], [[LOOP]] ]
; CHECK:         [[L0_PTR:%.*]] = inttoptr i32 [[IN0_NEW]] to <2 x double> addrspace(3)*
; checking that load is after L0_PTR, ensuring IR correctness that all uses go after the def
; despite the option CodeSinkingLoadSchedulingInstr=10
; CHECK:         [[L0:%.*]] = load <2 x double>, <2 x double> addrspace(3)* [[L0_PTR]], align 16
; CHECK:         [[ADD:%.*]] = add i32 [[IN0_NEW]], 15
; CHECK:         [[BC:%.*]] = bitcast <2 x double> [[L0]] to <4 x float>
; CHECK:         [[L0E0:%.*]] = extractelement <2 x double> [[L0]], i32 0
; CHECK:         [[A0:%.*]] = fadd double [[L0E0]], 2.000000e+00
; CHECK:         [[L0E1:%.*]] = extractelement <2 x double> [[L0]], i32 1
;
entry:
  %in0_shifted = getelementptr <2 x double>, <2 x double> addrspace(3)* %in_out, i32 %offsetIn0
  %in0_as_int = ptrtoint <2 x double> addrspace(3)* %in0_shifted to i32
  %in0_new = add i32 %in0_as_int, 800
  br label %entry_preheader

entry_preheader:                                  ; preds = %entry
  %l0_ptr = inttoptr i32 %in0_new to <2 x double> addrspace(3)*
  %l0 = load <2 x double>, <2 x double> addrspace(3)* %l0_ptr, align 16
  %l0e0 = extractelement <2 x double> %l0, i32 0
  %l0e1 = extractelement <2 x double> %l0, i32 1
  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  %acc = phi double [0.0, %entry_preheader ], [%acc1, %loop]
  %add = add i32 %in0_new, 15
  %bc = bitcast <2 x double> %l0 to <4 x float>
  %a0 = fadd double %l0e0, 2.000000e+00
  %a3 = fadd double %l0e1, 2.000000e+00
  %combine = fadd double %a0, %a3
  %acc1 = fadd double %acc, %combine
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  %out = bitcast <2 x double> addrspace(3)* %in_out to double addrspace(3)*
  %out0_shifted = getelementptr double, double addrspace(3)* %out, i32 2
  store double %acc, double addrspace(3)* %out0_shifted, align 8
  ret void
}

!igc.functions = !{}
