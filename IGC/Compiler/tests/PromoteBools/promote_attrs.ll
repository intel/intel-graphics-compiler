;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: system-linux
; RUN: igc_opt -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK-NOT: .unpromoted

%struct = type { [4 x <8 x i1*>], [4 x <8 x i1>*]* }

define spir_func void @prom_attr(%struct* byval(%struct) align 8 %0) {
  ret void
}
