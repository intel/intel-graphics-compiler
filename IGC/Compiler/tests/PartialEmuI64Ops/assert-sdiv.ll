;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: system-windows
; REQUIRES: debug
;
; RUN: not igc_opt -platformdg2 --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : assert check
; ------------------------------------------------

; CHECK: There should not be `sdiv` which is already emulated by library call

define void @test_sdiv(i64 %src1, i64 %src2) {
  %1 = sdiv i64 %src1, %src2
  call void @use.i64(i64 %1)
  ret void
}

declare void @use.i64(i64)

!igc.functions = !{!0}

!0 = !{void (i64, i64)* @test_sdiv, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
