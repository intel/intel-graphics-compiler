;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o - | FileCheck %s

@global_false = internal addrspace(3) global i1 false
@global_true = internal addrspace(3) global i1 true

; CHECK:        @global_false = internal addrspace(3) global i8 0
; CHECK:        @global_true = internal addrspace(3) global i8 1
