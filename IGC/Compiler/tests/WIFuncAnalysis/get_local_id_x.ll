;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-wi-func-analysis -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_local_id_x()

define i32 @foo(i32 %dim) nounwind {
  %id = call i32 @__builtin_IB_get_local_id_x()
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32)* @foo, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}

;CHECK: !{!"implicit_arg_desc", ![[A1:[0-9]+]], ![[A2:[0-9]+]], ![[A4:[0-9]+]], ![[A5:[0-9]+]], ![[A6:[0-9]+]]}
;CHECK: ![[A1]] = !{i32 0}
;CHECK: ![[A2]] = !{i32 1}
;CHECK: ![[A4]] = !{i32 8}
;CHECK: ![[A5]] = !{i32 9}
;CHECK: ![[A6]] = !{i32 10}

