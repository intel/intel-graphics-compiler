;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-resolve-inline-locals -S < %s | FileCheck %s
; ------------------------------------------------
; InlineLocalsResolution
; ------------------------------------------------
; This test checks that InlineLocalsResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: void @test_inline{{.*}}!dbg [[SCOPE:![0-9]*]]
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: [[ADD_V:%[A-z0-9]*]] = {{.*}}, !dbg [[ADD_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]
; CHECK: ret{{.*}}, !dbg [[RET_LOC:![0-9]*]]


define spir_kernel void @test_inline(i32 addrspace(3)* %a, i32 addrspace(3)* %b) !dbg !9 {
  %1 = load i32, i32 addrspace(3)* %a, !dbg !16
  call void @llvm.dbg.value(metadata i32 %1, metadata !12, metadata !DIExpression()), !dbg !16
  %2 = load i32, i32 addrspace(3)* %b, !dbg !17
  call void @llvm.dbg.value(metadata i32 %2, metadata !14, metadata !DIExpression()), !dbg !17
  %3 = add i32 %1, %2, !dbg !18
  call void @llvm.dbg.value(metadata i32 %3, metadata !15, metadata !DIExpression()), !dbg !18
  store i32 %3, i32 addrspace(3)* %b, !dbg !19
  ret void, !dbg !20
}
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "args.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_inline", linkageName: "test_inline", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[RET_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i32 addrspace(3)*, i32 addrspace(3)*)* @test_inline, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "args.ll", directory: "/")
!5 = !{}
!6 = !{i32 5}
!7 = !{i32 3}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_inline", linkageName: "test_inline", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocation(line: 1, column: 1, scope: !9)
!17 = !DILocation(line: 2, column: 1, scope: !9)
!18 = !DILocation(line: 3, column: 1, scope: !9)
!19 = !DILocation(line: 4, column: 1, scope: !9)
!20 = !DILocation(line: 5, column: 1, scope: !9)
