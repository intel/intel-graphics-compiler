;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey OverrideOCLMaxParamSize=1 --igc-error-check -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ErrorCheck
; ------------------------------------------------

; CHECK: error: Total size of kernel arguments exceeds limit! Total arguments size: 2824, limit: 1

define void @test_error(i32 %src, i32 %dst, <32 x double> %src3, <32 x i64> %src4, <32 x i64> %dst2, <256 x i64> %src5) {
entry:
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (i32, i32, <32 x double>, <32 x i64>, <32 x i64>, <256 x i64>)* @test_error, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i32, i32, <32 x double>, <32 x i64>, <32 x i64>, <256 x i64>)* @test_error}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
