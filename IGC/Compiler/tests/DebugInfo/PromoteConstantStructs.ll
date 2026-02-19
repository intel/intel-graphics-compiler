;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-constant-structs -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteConstantStructs
; ------------------------------------------------
; This test checks that PromoteConstantStructs pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata %struct.st* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64* [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64* [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64* [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: store i64 3, i64* [[VAL2_V]], {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK: store i64 2, i64* [[VAL3_V]], {{.*}}, !dbg [[STR2_LOC:![0-9]*]]
; CHECK: store i64 1, i64* [[VAL4_V]], {{.*}}, !dbg [[STR3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 1, metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: store i64 1, {{.*}}, !dbg [[STR4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 2, metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: store i64 2, {{.*}}, !dbg [[STR5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 3, metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: store i64 3, {{.*}}, !dbg [[STR6_LOC:![0-9]*]]

%struct.st = type { i64, i64, i64 }

define void @test(i64* %dst) !dbg !6 {
  %1 = alloca %struct.st, align 8, !dbg !17
  call void @llvm.dbg.value(metadata %struct.st* %1, metadata !9, metadata !DIExpression()), !dbg !17
  %2 = getelementptr inbounds %struct.st, %struct.st* %1, i64 0, i32 0, !dbg !18
  call void @llvm.dbg.value(metadata i64* %2, metadata !11, metadata !DIExpression()), !dbg !18
  %3 = getelementptr inbounds %struct.st, %struct.st* %1, i64 0, i32 2, !dbg !19
  call void @llvm.dbg.value(metadata i64* %3, metadata !12, metadata !DIExpression()), !dbg !19
  %4 = getelementptr inbounds %struct.st, %struct.st* %1, i64 0, i32 1, !dbg !20
  call void @llvm.dbg.value(metadata i64* %4, metadata !13, metadata !DIExpression()), !dbg !20
  store i64 3, i64* %2, align 8, !dbg !21
  store i64 2, i64* %3, align 8, !dbg !22
  store i64 1, i64* %4, align 8, !dbg !23
  %5 = load i64, i64* %4, align 8, !dbg !24
  call void @llvm.dbg.value(metadata i64 %5, metadata !14, metadata !DIExpression()), !dbg !24
  store i64 %5, i64* %dst, !dbg !25
  %6 = load i64, i64* %3, align 8, !dbg !26
  call void @llvm.dbg.value(metadata i64 %6, metadata !15, metadata !DIExpression()), !dbg !26
  store i64 %6, i64* %dst, !dbg !27
  %7 = load i64, i64* %2, align 8, !dbg !28
  call void @llvm.dbg.value(metadata i64 %7, metadata !16, metadata !DIExpression()), !dbg !28
  store i64 %7, i64* %dst, !dbg !29
  ret void, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PromoteConstantStructs.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR4_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR5_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR6_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "PromoteConstantStructs.ll", directory: "/")
!2 = !{}
!3 = !{i32 14}
!4 = !{i32 7}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 8, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 10, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 12, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
!27 = !DILocation(line: 11, column: 1, scope: !6)
!28 = !DILocation(line: 12, column: 1, scope: !6)
!29 = !DILocation(line: 13, column: 1, scope: !6)
!30 = !DILocation(line: 14, column: 1, scope: !6)
