;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-memopt2 -S < %s | FileCheck %s
; ------------------------------------------------
; MemOpt2
; ------------------------------------------------
; This test checks that MemOpt2 pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata i32 addrspace(1)* [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 addrspace(1)* [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]

define void @test(i32 addrspace(1)* %addr) !dbg !11 {
entry:
  %0 = getelementptr i32, i32 addrspace(1)* %addr, i32 0, !dbg !23
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %0, metadata !14, metadata !DIExpression()), !dbg !23
  %1 = load i32, i32 addrspace(1)* %0, !dbg !24
  call void @llvm.dbg.value(metadata i32 %1, metadata !16, metadata !DIExpression()), !dbg !24
  %2 = getelementptr i32, i32 addrspace(1)* %addr, i32 1, !dbg !25
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %2, metadata !18, metadata !DIExpression()), !dbg !25
  %3 = load i32, i32 addrspace(1)* %2, !dbg !26
  call void @llvm.dbg.value(metadata i32 %3, metadata !19, metadata !DIExpression()), !dbg !26
  %4 = add i32 %3, 32, !dbg !27
  call void @llvm.dbg.value(metadata i32 %4, metadata !20, metadata !DIExpression()), !dbg !27
  %5 = load i32, i32 addrspace(1)* %2, !dbg !28
  call void @llvm.dbg.value(metadata i32 %5, metadata !21, metadata !DIExpression()), !dbg !28
  %6 = add i32 %5, 13, !dbg !29
  call void @llvm.dbg.value(metadata i32 %6, metadata !22, metadata !DIExpression()), !dbg !29
  store i32 %1, i32 addrspace(1)* %addr, !dbg !30
  store i32 %4, i32 addrspace(1)* %addr, !dbg !31
  store i32 %6, i32 addrspace(1)* %addr, !dbg !32
  ret void, !dbg !33
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "load.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.debugify = !{!4, !5, !6, !4}
!llvm.module.flags = !{!7}
!llvm.dbg.cu = !{!8}

!0 = !{void (i32 addrspace(1)*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i32 7}
!5 = !{i32 3}
!6 = !{i32 11}
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = distinct !DICompileUnit(language: DW_LANG_C, file: !9, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !10)
!9 = !DIFile(filename: "load.ll", directory: "/")
!10 = !{}
!11 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !9, line: 1, type: !12, scopeLine: 1, unit: !8, retainedNodes: !13)
!12 = !DISubroutineType(types: !10)
!13 = !{!14, !16, !18, !19, !20, !21, !22}
!14 = !DILocalVariable(name: "1", scope: !11, file: !9, line: 1, type: !15)
!15 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "2", scope: !11, file: !9, line: 2, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "3", scope: !11, file: !9, line: 3, type: !15)
!19 = !DILocalVariable(name: "4", scope: !11, file: !9, line: 4, type: !17)
!20 = !DILocalVariable(name: "5", scope: !11, file: !9, line: 5, type: !17)
!21 = !DILocalVariable(name: "6", scope: !11, file: !9, line: 6, type: !17)
!22 = !DILocalVariable(name: "7", scope: !11, file: !9, line: 7, type: !17)
!23 = !DILocation(line: 1, column: 1, scope: !11)
!24 = !DILocation(line: 2, column: 1, scope: !11)
!25 = !DILocation(line: 3, column: 1, scope: !11)
!26 = !DILocation(line: 4, column: 1, scope: !11)
!27 = !DILocation(line: 5, column: 1, scope: !11)
!28 = !DILocation(line: 6, column: 1, scope: !11)
!29 = !DILocation(line: 7, column: 1, scope: !11)
!30 = !DILocation(line: 8, column: 1, scope: !11)
!31 = !DILocation(line: 9, column: 1, scope: !11)
!32 = !DILocation(line: 10, column: 1, scope: !11)
!33 = !DILocation(line: 11, column: 1, scope: !11)
