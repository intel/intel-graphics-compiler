;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%opencl.image2d_t = type opaque

declare i32 @__builtin_IB_get_image_height(i32 %img)

define i32 @foo(i32 %img, i32 %imageHeigt) nounwind {
  %id = call i32 @__builtin_IB_get_image_height(i32 %img)
  ret i32 %id
}

!igc.input.ir = !{!100}
!100 = !{!"ocl", i32 1, i32 2}

!igc.functions = !{!0}
!0 = !{i32 (i32, i32)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4}
!4 = !{i32 21, !5}
!5 = !{!"explicit_arg_num", i32 0}

; CHECK:         ret i32 %imageHeigt

; CHECK-NOT:     call i32 @__builtin_IB_get_image_height(i32 %img)

