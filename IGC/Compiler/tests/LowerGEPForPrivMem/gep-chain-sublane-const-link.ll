;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg --regkey DisableSOAPromotion=0 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------
; Each GEP of a chain is lowered to whole lanes on its own. A chain whose total offset is
; lane-aligned but whose constant parts are not (1 byte, then 3 bytes around a dynamic
; float step) cannot be promoted; the same chain with lane-sized steps can.

%C1 = type { i8 }
%C4 = type { i32 }

define <2 x float> @sublane_links(i32 %n) {
; CHECK-LABEL: @sublane_links(
; CHECK:       alloca [8 x float]
; CHECK-NOT:   alloca <8 x float>
  %a = alloca [8 x float], align 4
  %q = getelementptr %C1, ptr %a, i32 1
  %i = and i32 %n, 3
  %f = getelementptr float, ptr %q, i32 %i
  %r = getelementptr %C1, ptr %f, i32 3
  %v = load <2 x float>, ptr %r, align 4
  ret <2 x float> %v
}

define <2 x float> @lane_links(i32 %n) {
; CHECK-LABEL: @lane_links(
; CHECK:       alloca <8 x float>
; CHECK-NOT:   alloca [8 x float]
  %a = alloca [8 x float], align 4
  %q = getelementptr %C4, ptr %a, i32 1
  %i = and i32 %n, 3
  %f = getelementptr float, ptr %q, i32 %i
  %r = getelementptr %C4, ptr %f, i32 1
  %v = load <2 x float>, ptr %r, align 4
  ret <2 x float> %v
}

!igc.functions = !{!0, !3}

!0 = !{ptr @sublane_links, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @lane_links, !1}
