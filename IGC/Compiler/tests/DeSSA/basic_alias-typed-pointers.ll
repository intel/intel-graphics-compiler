;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -regkey PrintToConsole=1 -DeSSA -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DeSSA
; ------------------------------------------------
; Test checks that value %a is registered in AliasMap

; CHECK: ---- AliasMap ----
; CHECK:   Aliasee: i32 %a
; CHECK:        %2 = bitcast i32 %a to float
; CHECK:        %3 = inttoptr i32 %a to float*
; CHECK:        %4 = bitcast float* %3 to <2 x i32>*
; CHECK:        %5 = bitcast float* %3 to [2 x i32]*
; CHECK: ---- InsEltMap ----


define spir_kernel void @dessa(i32 %a) {
entry:
  %0 = insertelement < 2 x i32 > undef, i32 %a, i32 1
  %1 = insertvalue [2 x i32] undef, i32 %a, 0
  %2 = bitcast i32 %a to float
  %3 = inttoptr i32 %a to float*
  store float %2, float* %3
  %4 = bitcast float* %3 to <2 x i32>*
  store <2 x i32> %0, <2 x i32>* %4
  %5 = bitcast float* %3 to [2 x i32]*
  store [2 x i32] %1, [2 x i32]* %5
  ret void
}

!igc.functions = !{!0}
!0 = !{void (i32)* @dessa, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
