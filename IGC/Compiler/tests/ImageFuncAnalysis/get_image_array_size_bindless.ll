;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Negative test checks that no implicit args are captured in bindless mode


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-analysis -S %s -o - | FileCheck %s

declare i32 @__builtin_IB_get_image1d_array_size(i32 %img)

declare i32 @__builtin_IB_get_image2d_array_size(i32 %img)

define i32 @foo(i32 %img) nounwind {
  %id = call i32 @__builtin_IB_get_image1d_array_size(i32 %img)
  ret i32 %id
}

define i32 @bar(i32 %img) nounwind {
  %id = call i32 @__builtin_IB_get_image2d_array_size(i32 %img)
  ret i32 %id
}

!igc.functions = !{!0, !8}
!IGCMetadata = !{!4}

!0 = !{i32 (i32)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5, !9}
!5 = !{!"compOpt", !6, !7}
!6 = !{!"UseBindlessMode", i1 true}
!7 = !{!"UseLegacyBindlessMode", i1 false}
!8 = !{i32 (i32)* @bar, !1}
!9 = !{!"UseBindlessImage", i1 true}

;CHECK-NOT: !{!"implicit_arg_desc", ![[A1:[0-9]+]]}
;CHECK-NOT: ![[A2:[0-9]+]] = !{i32 28, ![[A3:[0-9]+]]}
;CHECK-NOT: ![[A4:[0-9]+]] = !{!"explicit_arg_num", i32 0}
