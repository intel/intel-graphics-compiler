;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o - | FileCheck %s

@global_variable = internal addrspace(3) global i1 false

define void @load_and_store_global_variable(i1 %input) {
  %value = load i1, i1 addrspace(3)* @global_variable
  store i1 false, i1 addrspace(3)* @global_variable
  store i1 true, i1 addrspace(3)* @global_variable
  store i1 %input, i1 addrspace(3)* @global_variable
  ret void
}

; CHECK:        @global_variable = internal addrspace(3) global i8 0

; CHECK:        define void @load_and_store_global_variable(i8 %input) {
; CHECK-NEXT:   load i8, i8 addrspace(3)* @global_variable
; CHECK-NEXT:   store i8 0, i8 addrspace(3)* @global_variable
; CHECK-NEXT:   store i8 1, i8 addrspace(3)* @global_variable
; CHECK-NEXT:   store i8 {{%[a-zA-Z0-9]+}}, i8 addrspace(3)* @global_variable
