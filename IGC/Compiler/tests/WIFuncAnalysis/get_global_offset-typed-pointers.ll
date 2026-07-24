;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-wi-func-analysis -igc-serialize-metadata -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i32 @__builtin_IB_get_global_offset(i32 %dim)

define i32 @foo(i32 %dim) nounwind {
  %id = call i32 @__builtin_IB_get_global_offset(i32 %dim)
  ret i32 %id
}

!IGCMetadata = !{!7}
!igc.functions = !{!0}
!0 = !{i32 (i32)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"functionType", !"KernelFunction"}
!4 = !{!"FuncMDMap[0]", i32 (i32)* @foo}
!5 = !{!"FuncMDValue[0]", !3}
!6 = !{!"FuncMD", !4, !5}
!7 = !{!"ModuleMD", !6}

;CHECK: !{!"implicitArgInfoList"
;CHECK: !{!"argId", i32 0}
;CHECK: !{!"argId", i32 1}

