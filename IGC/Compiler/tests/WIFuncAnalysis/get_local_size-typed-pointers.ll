;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-wi-func-analysis -igc-serialize-metadata -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_local_size(i32 %dim)

define i32 @foo(i32 %dim) nounwind {
  %id = call i32 @__builtin_IB_get_local_size(i32 %dim)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (i32)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

;CHECK: !{!"implicitArgInfoList"
;CHECK: !{!"argId", i32 0}
;CHECK: !{!"argId", i32 1}
;CHECK: !{!"argId", i32 6}

