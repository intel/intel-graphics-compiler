;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -regkey PrintToConsole=1 -DeSSA -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DeSSA
; ------------------------------------------------
; Test checks that value %a is registered in AliasMap

; CHECK: ---- AliasMap ----
; CHECK:   Aliasee: i32 %a
; CHECK:        %2 = bitcast i32 %a to float
; CHECK:        %3 = inttoptr i32 %a to ptr


; CHECK: ---- InsEltMap ----


define spir_kernel void @dessa(i32 %a) {
entry:
  %0 = insertelement < 2 x i32 > undef, i32 %a, i32 1
  %1 = insertvalue [2 x i32] undef, i32 %a, 0
  %2 = bitcast i32 %a to float
  %3 = inttoptr i32 %a to ptr
  store float %2, ptr %3

  store <2 x i32> %0, ptr %3

  store [2 x i32] %1, ptr %3
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @dessa, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
