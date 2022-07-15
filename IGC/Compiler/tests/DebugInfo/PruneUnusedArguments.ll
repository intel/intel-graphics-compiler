;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -PruneUnusedArguments -S < %s | FileCheck %s
; ------------------------------------------------
; PruneUnusedArguments
; ------------------------------------------------
; This test checks that PruneUnusedArguments pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_func void @foo{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 %a, metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]

; CHECK: define spir_kernel void @test{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 %src2, metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: call spir_func void @foo{{.*}} !dbg [[CALL_LOC:![0-9]*]]


define spir_func void @foo(i32* %b, i32 %a) !dbg !6 {
entry:
  call void @llvm.dbg.value(metadata i32 %a, metadata !9, metadata !DIExpression()), !dbg !12
  %0 = load i32, i32* %b, !dbg !12
  %1 = add i32 %0, 13, !dbg !13
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !13
  store i32 %1, i32* %b, !dbg !14
  ret void, !dbg !15
}

define spir_kernel void @test(i32* %src1, i32 %src2) !dbg !16 {
  call void @llvm.dbg.value(metadata i32 %src2, metadata !18, metadata !DIExpression()), !dbg !19
  call spir_func void @foo(i32* %src1, i32 %src2), !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PruneUnusedArguments.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE1]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE2]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE2]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "PruneUnusedArguments.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocation(line: 1, column: 1, scope: !6)
!13 = !DILocation(line: 2, column: 1, scope: !6)
!14 = !DILocation(line: 3, column: 1, scope: !6)
!15 = !DILocation(line: 4, column: 1, scope: !6)
!16 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 5, type: !7, scopeLine: 5, unit: !0, retainedNodes: !17)
!17 = !{!18}
!18 = !DILocalVariable(name: "3", scope: !16, file: !1, line: 5, type: !10)
!19 = !DILocation(line: 5, column: 1, scope: !16)
!20 = !DILocation(line: 6, column: 1, scope: !16)
!21 = !DILocation(line: 7, column: 1, scope: !16)
