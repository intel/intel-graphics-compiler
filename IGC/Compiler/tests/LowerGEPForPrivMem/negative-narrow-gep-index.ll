;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg --regkey DisableSOAPromotion=0,ForcePromoteI8=0 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------
; Check handling of negative indices and indices of types narrower than i32.

%S = type { float }

; Element GEP with a narrow index: a[5] - 1 is lane 4, not 5 + 255.
define float @narrow_elt_index() {
; CHECK-LABEL: @narrow_elt_index(
; CHECK:       alloca <8 x float>
; CHECK:       extractelement <8 x float> %{{.*}}, i32 4
  %a = alloca [8 x float], align 4
  %a5 = getelementptr [8 x float], ptr %a, i32 0, i32 5
  %p = getelementptr float, ptr %a5, i8 -1
  %v = load float, ptr %p, align 4
  ret float %v
}

; Struct GEP with a negative constant index: a[5] - 1 is lane 4.
define float @negative_struct_index() {
; CHECK-LABEL: @negative_struct_index(
; CHECK:       alloca <8 x float>
; CHECK:       extractelement <8 x float> %{{.*}}, i32 4
  %a = alloca [8 x %S], align 4
  %a5 = getelementptr [8 x %S], ptr %a, i32 0, i32 5
  %p = getelementptr %S, ptr %a5, i32 -1
  %v = load float, ptr %p, align 4
  ret float %v
}

; Byte-offset GEP with a narrow index: byte 8 - 1 is lane 7.
define i8 @narrow_byte_offset() {
; CHECK-LABEL: @narrow_byte_offset(
; CHECK:       alloca <16 x i8>
; CHECK:       extractelement <16 x i8> %{{.*}}, i32 7
  %a = alloca [16 x i8], align 4
  %a8 = getelementptr [16 x i8], ptr %a, i32 0, i32 8
  %p = getelementptr i8, ptr %a8, i8 -1
  %v = load i8, ptr %p, align 1
  ret i8 %v
}

!igc.functions = !{!0, !3, !4}

!0 = !{ptr @narrow_elt_index, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @negative_struct_index, !1}
!4 = !{ptr @narrow_byte_offset, !1}
