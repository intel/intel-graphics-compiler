;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=4 %s -S -o - --platformmtl --igc-precompiled-import | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
;
; ForceEmuKind=4 means EMU_DP_DIV_SQRT
;
; This test checks PreCompiledFuncImport pass replaces fdiv instruction with builtin __igcbuiltin_dp_div_nomadm_ieee call.
; Also it checks the builtin sets Divide by Zero bit in IEEE Exception bits in State Register
; if a divisor is 0.0 and IEEE Exception Trap is enabled in Control Register.
;
; ------------------------------------------------

; CHECK-LABEL: define void @test_ieee_fdiv_emulation
; CHECK: %dres1 = call double @__igcbuiltin_dp_div_nomadm_ieee(double %a, double %b)

; CHECK-LABEL: define internal double @__igcbuiltin_dp_div_nomadm_ieee
; CHECK:(double noundef [[TMP0:%.*]], double noundef [[TMP1:%.*]])
; CHECK: [[TMP3:%.*]] = fcmp oeq double [[TMP1]], 0.000000e+00
; CHECK: br i1 [[TMP3]], label %[[L4:[0-9]*]], label %[[L11:[0-9]*]]

; CHECK: [[L4]]:
; CHECK:  [[TMP5:%.*]] = call i32 @llvm.genx.GenISA.movcr.i32.i32(i32 0)
; CHECK:  [[TMP6:%.*]] = and i32 [[TMP5]], 512
; CHECK:  [[TMP7:%.*]] = icmp eq i32 [[TMP6]], 0
; CHECK:  br i1 [[TMP7]], label %[[L11]], label %[[L8:[0-9]*]]

; CHECK:[[L8]]:
; CHECK:  [[TMP9:%.*]] = call i32 @llvm.genx.GenISA.getSR0.i32.i32(i32 1)
; CHECK:  [[TMP10:%.*]] = or i32 [[TMP9]], 2
; CHECK:  call void @llvm.genx.GenISA.setSR0.isVoid.i32.i32(i32 1, i32 [[TMP10]])
; CHECK:  br label %11

; CHECK:[[L11]]:

define void @test_ieee_fdiv_emulation(i64 addrspace(1)* %out, double %a, double %b) #0 {
entry:
  %dres = fdiv double %a, %b
  store double %dres, double addrspace(1)* %out, align 8
  ret void
}

attributes #0 = { strictfp }
