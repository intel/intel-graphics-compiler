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
; RUN: igc_opt --typed-pointers -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=4 -regkey InlinedEmulationThreshold=500 --platformmtl --igc-precompiled-import --print-codegencontext -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
; ------------------------------------------------

; Inlined instructions threshold is not high enough to fit all functions. Check if sqrt function is
; fully inlined and div function split into inlined and noinlined variants.

; CHECK: m_enableSubroutine: 1

define void @test(double addrspace(1)* %p, double %a, double %b, double %c) #0 {
entry:
; CHECK-LABEL: @test
; CHECK: entry:
; CHECK:   [[TMP0:%.*]] = call double @__igcbuiltin_dp_sqrt_nomadm_ieee(double %a)
; CHECK:   [[TMP1:%.*]] = call double @__igcbuiltin_dp_div_nomadm_ieee_always_inline(double [[TMP0]], double %b)
; CHECK:   [[TMP2:%.*]] = call double @__igcbuiltin_dp_sqrt_nomadm_ieee(double [[TMP1]])
; CHECK:   [[TMP3:%.*]] = call double @__igcbuiltin_dp_div_nomadm_ieee(double [[TMP2]], double %c)
; CHECK:   store double [[TMP3]], double addrspace(1)* %p, align 8
  %call.i.i2 = call double @llvm.sqrt.f64(double %a)
  %div = fdiv double %call.i.i2, %b
  %call.i.i13 = call double @llvm.sqrt.f64(double %div)
  %div2 = fdiv double %call.i.i13, %c
  store double %div2, double addrspace(1)* %p, align 8
  ret void
}

declare double @llvm.sqrt.f64(double) #1

; CHECK: define internal double @__igcbuiltin_dp_div_nomadm_ieee{{.*}}[[ATTR0:#[0-9]+]]
; CHECK: define internal double @__igcbuiltin_dp_sqrt_nomadm_ieee{{.*}}[[ATTR1:#[0-9]+]]
; CHECK: define internal double @__igcbuiltin_dp_div_nomadm_ieee_always_inline{{.*}}[[ATTR2:#[0-9]+]]

; CHECK: attributes [[ATTR0]] = { noinline
; CHECK: attributes [[ATTR1]] = { alwaysinline
; CHECK: attributes [[ATTR2]] = { alwaysinline
