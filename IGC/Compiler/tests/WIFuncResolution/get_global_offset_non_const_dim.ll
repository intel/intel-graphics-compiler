;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-wi-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_global_offset(i32 %dim)

define i32 @foo(i32 %dim, <8 x i32> %r0, <8 x i32> %payloadHeader) nounwind {
  %id = call i32 @__builtin_IB_get_global_offset(i32 %dim)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32, <8 x i32>, <8 x i32>)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5}
!4 = !{i32 0}
!5 = !{i32 1}

; CHECK:         %globalOffset = extractelement <8 x i32> %payloadHeader, i32 %dim

; CHECK-NOT:     call i32 @__builtin_IB_get_global_offset(i32 0)
