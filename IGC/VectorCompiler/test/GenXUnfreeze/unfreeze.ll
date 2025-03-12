;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXUnfreeze -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-NOT: freeze

define spir_kernel void @test(i32 %arg) {
  %w = add i32 %arg, undef
  %x = freeze i32 %w
  %y = add i32 %w, %w         ; undef
  %z = add i32 %x, %x         ; even number because all uses of %x observe
                              ; the same value
  %x2 = freeze i32 %w
  %cmp = icmp eq i32 %x, %x2  ; can be true or false

  ; example with vectors
  %v = add <2 x i32> <i32 undef, i32 poison>, zeroinitializer
  %a = extractelement <2 x i32> %v, i32 0    ; undef
  %b = extractelement <2 x i32> %v, i32 1    ; poison
  %add = add i32 %a, %a                      ; undef

  %v.fr = freeze <2 x i32> %v                ; element-wise freeze
  %d = extractelement <2 x i32> %v.fr, i32 0 ; not undef
  %add.f = add i32 %d, %d                    ; even number

  ret void
}
