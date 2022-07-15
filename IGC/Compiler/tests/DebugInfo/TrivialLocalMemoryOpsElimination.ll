;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -TrivialLocalMemoryOpsElimination -S < %s | FileCheck %s
; ------------------------------------------------
; TrivialLocalMemoryOpsElimination
; ------------------------------------------------
; This test checks that TrivialLocalMemoryOpsElimination pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_load{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: ret {{.*}}, !dbg [[RET1_LOC:![0-9]*]]

define spir_kernel void @test_load(i32 addrspace(3)* %b) !dbg !10 {
  %1 = load i32, i32 addrspace(3)* %b, !dbg !15
  call void @llvm.dbg.value(metadata i32 %1, metadata !13, metadata !DIExpression()), !dbg !15
  ret void, !dbg !16
}

; CHECK: define spir_kernel void @test_store{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: ret {{.*}}, !dbg [[RET2_LOC:![0-9]*]]

define spir_kernel void @test_store(i32 addrspace(3)* %b) !dbg !17 {
  store i32 13, i32 addrspace(3)* %b, !dbg !18
  ret void, !dbg !19
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "TrivialLocalMemoryOpsElimination.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test_load", linkageName: "test_load", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[RET1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test_store", linkageName: "test_store", scope: null, file: [[FILE]], line: 3
; CHECK-DAG: [[RET2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE2]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0, !3}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32 addrspace(3)*)* @test_load, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i32 addrspace(3)*)* @test_store, !1}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "TrivialLocalMemoryOpsElimination.ll", directory: "/")
!6 = !{}
!7 = !{i32 4}
!8 = !{i32 1}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_load", linkageName: "test_load", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocation(line: 1, column: 1, scope: !10)
!16 = !DILocation(line: 2, column: 1, scope: !10)
!17 = distinct !DISubprogram(name: "test_store", linkageName: "test_store", scope: null, file: !5, line: 3, type: !11, scopeLine: 3, unit: !4, retainedNodes: !6)
!18 = !DILocation(line: 3, column: 1, scope: !17)
!19 = !DILocation(line: 4, column: 1, scope: !17)
