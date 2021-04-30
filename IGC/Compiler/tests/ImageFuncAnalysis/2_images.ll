;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-image-func-analysis -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%opencl.image2d_t = type opaque

declare i32 @__builtin_IB_get_image_width(i32 %img)

define i32 @foo(i32 %img1, i32 %img2) nounwind {
  %id1 = call i32 @__builtin_IB_get_image_width(i32 %img1)
  %id2 = call i32 @__builtin_IB_get_image_width(i32 %img2)
  %res = add i32 %id1, %id2
  ret i32 %res
}

!igc.functions = !{!0}
!0 = !{i32 (i32, i32)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

;CHECK: !{!"implicit_arg_desc", ![[A1:[0-9]+]], ![[A3:[0-9]+]]}
;CHECK: ![[A1]] = !{i32 21, ![[A2:[0-9]+]]}
;CHECK: ![[A2]] = !{!"explicit_arg_num", i32 0}
;CHECK: ![[A3]] = !{i32 21, ![[A4:[0-9]+]]}
;CHECK: ![[A4]] = !{!"explicit_arg_num", i32 1}


