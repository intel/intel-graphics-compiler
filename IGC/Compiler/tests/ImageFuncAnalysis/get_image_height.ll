;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-image-func-analysis -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_image_height(i32 %img)

define i32 @foo(i32 %img) nounwind {
  %id = call i32 @__builtin_IB_get_image_height(i32 %img)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

;CHECK: !{!"implicit_arg_desc", ![[A1:[0-9]+]]}
;CHECK: ![[A1]] = !{i32 20, ![[A2:[0-9]+]]}
;CHECK: ![[A2]] = !{!"explicit_arg_num", i32 0}


