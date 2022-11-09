;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -undefined-references -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; UndefinedReferencesPass
; ------------------------------------------------

; CHECK: error: undefined reference to `foo'

define spir_kernel void @test() {
entry:
  call void @foo()
  ret void
}

declare void @foo()
