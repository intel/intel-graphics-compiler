;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows, llvm-17-plus
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=6 < %s 2>&1 | FileCheck %s

define void @test() {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 1, i32* %a, align 4
  %val = load i32, i32* %a, align 4
  store i32 %val, i32* %b, align 4
  ret void
}

; CHECK: block: entry function: test
; CHECK: N: UP: 0 NP: 1     32 (1)        %a = alloca i32, align 4
; CHECK: N: UP: 0 NP: 2     64 (2)        %b = alloca i32, align 4
; CHECK: N: UP: 0 NP: 2     64 (2)        store i32 1, i32* %a, align 4
; CHECK: N: UP: 0 NP: 2     64 (2)        %val = load i32, i32* %a, align 4
; CHECK: N: UP: 0 NP: 0     0 (0)         store i32 %val, i32* %b, align 4
; CHECK: N: UP: 0 NP: 0     0 (0)         ret void
; CHECK: MaxPressure In Function: test --> 2
; CHECK: Max Uniform Pressure In Function: test --> 0
; CHECK: Max NonUniform Pressure In Function: test --> 2
