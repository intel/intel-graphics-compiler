;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=4 -regkey InlinedEmulationThreshold=600 --platformmtl --igc-precompiled-import --print-codegencontext -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
; ------------------------------------------------

; Inlined instructions threshold is not high enough to fit all three calls. Check if
; function is correctly split into inlined and noinlined variants.

; CHECK: m_enableSubroutine: 1

define void @test(double addrspace(1)* %p, double %a, double %b, double %c, double %d) #0 {
entry:
; CHECK-LABEL: @test
; CHECK: entry:
; CHECK:   [[TMP0:%.*]] = call double @__igcbuiltin_dp_div_nomadm_ieee_always_inline(double %a, double %b)
; CHECK:   [[TMP1:%.*]] = call double @__igcbuiltin_dp_div_nomadm_ieee_always_inline(double [[TMP0]], double %c)
; CHECK:   [[TMP2:%.*]] = call double @__igcbuiltin_dp_div_nomadm_ieee(double [[TMP1]], double %d)
; CHECK:   store double [[TMP2]], double addrspace(1)* %p
  %div = fdiv double %a, %b
  %div1 = fdiv double %div, %c
  %div2 = fdiv double %div1, %d
  store double %div2, double addrspace(1)* %p, align 8
  ret void
}

; CHECK: define internal double @__igcbuiltin_dp_div_nomadm_ieee{{.*}}[[ATTR0:#[0-9]+]]
; CHECK: define internal double @__igcbuiltin_dp_div_nomadm_ieee_always_inline{{.*}}[[ATTR1:#[0-9]+]]

; CHECK: attributes [[ATTR0]] = { noinline
; CHECK: attributes [[ATTR1]] = { alwaysinline
