;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-remove-nonposition-output -S < %s | FileCheck %s
; ------------------------------------------------
; RemoveNonPositionOutput
; ------------------------------------------------
; This test checks that RemoveNonPositionOutput pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_remove{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: call void{{.*}} !dbg [[CALL1_LOC:![0-9]*]]
; CHECK: call void{{.*}} !dbg [[CALL2_LOC:![0-9]*]]

define void @test_remove(i32 %src) !dbg !16 {
entry:
  call void @llvm.genx.GenISA.OUTPUT.f32(float 1.500000e+00, float 2.500000e+00, float 3.500000e+00, float 4.500000e+00, i32 1, i32 1), !dbg !18
  call void @llvm.genx.GenISA.OUTPUT.f32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, i32 0, i32 1), !dbg !19
  call void @llvm.genx.GenISA.OUTPUT.f32(float 1.250000e+00, float 2.250000e+00, float 3.250000e+00, float 4.250000e+00, i32 8, i32 1), !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "RemoveNonPositionOutput.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_remove", linkageName: "test_remove", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

declare void @llvm.genx.GenISA.OUTPUT.f32(float, float, float, float, i32, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!llvm.dbg.cu = !{!10}
!llvm.debugify = !{!13, !14}
!llvm.module.flags = !{!15}

!0 = !{void (i32)* @test_remove, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i32)* @test_remove}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = distinct !DICompileUnit(language: DW_LANG_C, file: !11, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !12)
!11 = !DIFile(filename: "RemoveNonPositionOutput.ll", directory: "/")
!12 = !{}
!13 = !{i32 4}
!14 = !{i32 0}
!15 = !{i32 2, !"Debug Info Version", i32 3}
!16 = distinct !DISubprogram(name: "test_remove", linkageName: "test_remove", scope: null, file: !11, line: 1, type: !17, scopeLine: 1, unit: !10, retainedNodes: !12)
!17 = !DISubroutineType(types: !12)
!18 = !DILocation(line: 1, column: 1, scope: !16)
!19 = !DILocation(line: 2, column: 1, scope: !16)
!20 = !DILocation(line: 3, column: 1, scope: !16)
!21 = !DILocation(line: 4, column: 1, scope: !16)
