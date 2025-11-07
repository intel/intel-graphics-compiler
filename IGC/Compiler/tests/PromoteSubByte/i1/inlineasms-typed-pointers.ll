;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_func i32 @inlineasm(i1 %input) {
  %result = call i32 asm sideeffect "{\0A.decl P1 v_type=P num_elts=1\0Acmp.eq (M1_NM, 16) P1 $1(0,0)<0;1,0> 0x0:b\0A(P1) sel (M1_NM, 16) $0(0,0)<1> 0x7:d 0x8:d}\0A", "=rw,rw"(i1 %input)
  ret i32 %result
}

; CHECK:        define spir_func i32 @inlineasm(i8 %input)
; CHECK-NEXT:   %1 = call i32 asm sideeffect "{\0A.decl P1 v_type=P num_elts=1\0Acmp.eq (M1_NM, 16) P1 $1(0,0)<0;1,0> 0x0:b\0A(P1) sel (M1_NM, 16) $0(0,0)<1> 0x7:d 0x8:d}\0A", "=rw,rw"(i8 %input)
; CHECK-NEXT:   ret i32 %1
