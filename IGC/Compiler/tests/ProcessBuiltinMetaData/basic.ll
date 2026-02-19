;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-process-builtin-metaData -check-debugify -igc-serialize-metadata -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ProcessBuiltinMetaData
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_processmd(i32* %src, i32* %dst) {
  call void @user_processmd(i32* %src, i32* %dst)
  ret void
}

define void @user_processmd(i32* %src, i32* %dst) {
entry:
  %0 = load i32, i32* %src
  %1 = load i32, i32* %dst
  %2 = add i32 %0, %1
  store i32 %2, i32* %dst
  ret void
}

; CHECK-DAG: !igc.functions = !{[[TEST_MD:![0-9]*]], [[USER_MD:![0-9]*]]}
; CHECK-DAG: [[TEST_MD]] = !{void (i32*, i32*)* @test_processmd
; CHECK-DAG: [[USER_MD]] = !{void (i32*, i32*)* @user_processmd, [[USER_FT:![0-9]*]]}
; CHECK-DAG: [[USER_FT]] = !{[[USER_FT1:![0-9]*]]}
; CHECK-DAG: [[USER_FT1]] = !{!"function_type", i32 2}
; CHECK-DAG: !{!"FuncMDMap[1]", void (i32*, i32*)* @user_processmd}
; CHECK-DAG: [[FUNC_MD_FT:![0-9]*]] = !{!"functionType", !"UserFunction"}
; CHECK-DAG: [[FUNC_MD_AQ:![0-9]*]] = !{!"m_OpenCLArgAccessQualifiers", [[FUNC_MD_AQ0:![0-9]*]], [[FUNC_MD_AQ1:![0-9]*]]}
; CHECK-DAG: [[FUNC_MD_AQ0]] = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
; CHECK-DAG: [[FUNC_MD_AQ1]] = !{!"m_OpenCLArgAccessQualifiersVec[1]", !"none"}
; CHECK-DAG: [[FUNC_MD_AB:![0-9]*]] = !{!"m_OpenCLArgBaseTypes", [[FUNC_MD_AB0:![0-9]*]], [[FUNC_MD_AB1:![0-9]*]]}
; CHECK-DAG: [[FUNC_MD_AB0]] = !{!"m_OpenCLArgBaseTypesVec[0]", !"i32*"}
; CHECK-DAG: [[FUNC_MD_AB1]] = !{!"m_OpenCLArgBaseTypesVec[1]", !"i32*"}
; CHECK-DAG: [[FUNC_MD_AN:![0-9]*]] = !{!"m_OpenCLArgNames", [[FUNC_MD_AN0:![0-9]*]], [[FUNC_MD_AN1:![0-9]*]]}
; CHECK-DAG: [[FUNC_MD_AN0]] = !{!"m_OpenCLArgNamesVec[0]", !"src"}
; CHECK-DAG: [[FUNC_MD_AN1]] = !{!"m_OpenCLArgNamesVec[1]", !"dst"}
; CHECK-DAG: !{!"FuncMDValue[1]", {{.*}}[[FUNC_MD_FT]]{{.*}}[[FUNC_MD_AQ]]{{.*}}[[FUNC_MD_AB]]{{.*}}[[FUNC_MD_AN]]

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (i32*, i32*)* @test_processmd, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i32*, i32*)* @test_processmd}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
