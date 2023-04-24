;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXRematerializationWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXRematerialization
; ------------------------------------------------
; This test checks that GenXRematerialization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; This pass does some cloning

; CHECK: define dllexport void @test_remat{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x i32> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x double> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x double> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x double> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x double> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x double> [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x double> [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <512 x double> [[VAL8_V:%[A-z0-9]*]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}, !dbg [[VAL8_LOC]]
; CHECK: store <512 x double> [[VAL8_V]]{{.*}}, !dbg [[STORE_LOC:![0-9]*]]

define dllexport void @test_remat(<512 x i32>* %a, <512 x double>* %b) #0 !dbg !6 {
entry:
  %0 = load <512 x i32>, <512 x i32>* %a, !dbg !19
  call void @llvm.dbg.value(metadata <512 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !19
  %1 = uitofp <512 x i32> %0 to <512 x double>, !dbg !20
  call void @llvm.dbg.value(metadata <512 x double> %1, metadata !11, metadata !DIExpression()), !dbg !20
  %2 = sitofp <512 x i32> %0 to <512 x double>, !dbg !21
  call void @llvm.dbg.value(metadata <512 x double> %2, metadata !13, metadata !DIExpression()), !dbg !21
  %3 = fadd <512 x double> %1, %2, !dbg !22
  call void @llvm.dbg.value(metadata <512 x double> %3, metadata !14, metadata !DIExpression()), !dbg !22
  %4 = fsub <512 x double> %1, %2, !dbg !23
  call void @llvm.dbg.value(metadata <512 x double> %4, metadata !15, metadata !DIExpression()), !dbg !23
  %5 = fdiv <512 x double> %3, %1, !dbg !24
  call void @llvm.dbg.value(metadata <512 x double> %5, metadata !16, metadata !DIExpression()), !dbg !24
  %6 = fdiv <512 x double> %4, %2, !dbg !25
  call void @llvm.dbg.value(metadata <512 x double> %6, metadata !17, metadata !DIExpression()), !dbg !25
  %7 = fadd <512 x double> %5, %6, !dbg !26
  call void @llvm.dbg.value(metadata <512 x double> %7, metadata !18, metadata !DIExpression()), !dbg !26
  store <512 x double> %7, <512 x double>* %b, !dbg !27
  ret void, !dbg !28
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXRematerialization.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_remat", linkageName: "test_remat", scope: null, file: [[FILE]], line: 1
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
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXRematerialization.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 8}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_remat", linkageName: "test_remat", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16, !17, !18}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty16384", size: 16384, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32768", size: 32768, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !12)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !12)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
!19 = !DILocation(line: 1, column: 1, scope: !6)
!20 = !DILocation(line: 2, column: 1, scope: !6)
!21 = !DILocation(line: 3, column: 1, scope: !6)
!22 = !DILocation(line: 4, column: 1, scope: !6)
!23 = !DILocation(line: 5, column: 1, scope: !6)
!24 = !DILocation(line: 6, column: 1, scope: !6)
!25 = !DILocation(line: 7, column: 1, scope: !6)
!26 = !DILocation(line: 8, column: 1, scope: !6)
!27 = !DILocation(line: 9, column: 1, scope: !6)
!28 = !DILocation(line: 10, column: 1, scope: !6)
