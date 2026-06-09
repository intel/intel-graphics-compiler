;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-wi-func-analysis -igc-serialize-metadata -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_num_groups(i32 %dim)
declare i32 @__builtin_IB_get_local_id_x()

define i32 @foo(i32 %dim) nounwind {
  %id1 = call i32 @__builtin_IB_get_num_groups(i32 %dim)
  %id2 = call i32 @__builtin_IB_get_local_id_x()
  %res = add i32 %id1, %id2
  ret i32 %res
}

!igc.functions = !{!0}
!0 = !{i32 (i32)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

;CHECK: !{!"implicitArgInfoList"
;CHECK: !{!"argId", i32 0}
;CHECK: !{!"argId", i32 1}
;CHECK: !{!"argId", i32 4}
;CHECK: !{!"argId", i32 8}
;CHECK: !{!"argId", i32 9}
;CHECK: !{!"argId", i32 10}
