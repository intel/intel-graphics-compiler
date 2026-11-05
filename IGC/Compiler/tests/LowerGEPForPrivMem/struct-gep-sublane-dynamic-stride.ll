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
; A dynamic index over a struct narrower than the promoted lane: { i8, i8 } is 2 bytes,
; the lane is 4. The index (n & 1) * 2 makes the byte offset 0 or 4, so the lane offset
; must be index * 2 / 4, not index * (2 / 4) = 0.

%B = type { i8, i8 }

define i16 @sublane_struct_stride(i32 %n) {
; CHECK-LABEL: @sublane_struct_stride(
; CHECK:       alloca <8 x float>
; CHECK:       [[BYTES:%.*]] = mul i32 %{{.*}}, 2
; CHECK:       [[LANES:%.*]] = sdiv i32 [[BYTES]], 4
; CHECK:       add i32 {{.*}}[[LANES]]
  %a = alloca [8 x float], align 4
  %q = getelementptr [8 x float], ptr %a, i32 0, i32 2
  %m = and i32 %n, 1
  %i = shl i32 %m, 1
  %p = getelementptr %B, ptr %q, i32 %i
  %v = load i16, ptr %p, align 2
  ret i16 %v
}

!igc.functions = !{!0}

!0 = !{ptr @sublane_struct_stride, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
