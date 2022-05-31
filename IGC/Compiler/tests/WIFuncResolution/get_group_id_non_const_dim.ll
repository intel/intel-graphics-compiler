;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-wi-func-resolution -instsimplify -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_group_id(i32 %dim)

define i32 @foo(i32 %dim, <8 x i32> %r0, <8 x i32> %payloadHeader) nounwind {
  %id = call i32 @__builtin_IB_get_group_id(i32 %dim)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32, <8 x i32>, <8 x i32>)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5}
!4 = !{i32 0}
!5 = !{i32 1}

; CHECK:         %cmpDim = icmp eq i32 %dim, 0
; CHECK-NEXT:   %tmpOffsetR0 = select i1 %cmpDim, i32 1, i32 5
; CHECK-NEXT:   %offsetR0 = add i32 %dim, %tmpOffsetR0
; CHECK-NEXT:   %groupId = extractelement <8 x i32> %r0, i32 %offsetR0

; CHECK-NOT:     call i32 @__builtin_IB_get_group_id(i32 2)
