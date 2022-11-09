;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-error-check -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ErrorCheck
; ------------------------------------------------

; CHECK: error: Unsupported call to llvm.genx.GenISA.dp4a.ss

define void @test_error(i32* %src, i32* %dst, <32 x double> %src3, <32 x i64> %src4, <32 x i64>* %dst2, <256 x i64> %src5) {
entry:
  %0 = load i32, i32* %src, align 4
  %1 = load i32, i32* %dst, align 4
  %2 = call i32 @llvm.genx.GenISA.dp4a.ss(i32 4, i32 %0, i32 %1)
  store i32 %2, i32* %dst, align 4
  %3 = fptoui <32 x double> %src3 to <32 x i64>
  %4 = add <32 x i64> %3, %src4
  store <32 x i64> %4, <32 x i64>* %dst2, align 256
  ret void
}

declare i32 @llvm.genx.GenISA.dp4a.ss(i32, i32, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (i32*, i32*, <32 x double>, <32 x i64>, <32 x i64>*, <256 x i64>)* @test_error, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i32*, i32*, <32 x double>, <32 x i64>, <32 x i64>*, <256 x i64>)* @test_error}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
