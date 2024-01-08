;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 --platformdg2 --igc-precompiled-import --print-codegencontext -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
; ------------------------------------------------

; Verify that slow DP emu functions are not inlined. These are original ones
; just for passing conformance, not for perf.

; CHECK: m_enableSubroutine: 1

define void @test(double addrspace(1)* %p, double %a, double %b, double %c) #0 {
entry:
; CHECK-LABEL: @test
; CHECK: entry:
; CHECK:   [[TMP0:%.*]] = call double @__igcbuiltin_dp_add(double %a, double %b, i32 0, i32 0, i32 0, i32* %DPEmuFlag)
; CHECK:   [[TMP1:%.*]] = call double @__igcbuiltin_dp_add(double [[TMP0]], double %c, i32 0, i32 0, i32 0, i32* %DPEmuFlag)
; CHECK:   store double [[TMP1]], double addrspace(1)* %p
  %add0 = fadd double %a, %b
  %add1 = fadd double %add0, %c
  store double %add1, double addrspace(1)* %p, align 8
  ret void
}

; CHECK: define internal double @__igcbuiltin_dp_add{{.*}}[[ATTR0:#[0-9]+]]

; CHECK: attributes [[ATTR0]] = {{{.*}} noinline {{.*}}visaStackCall
