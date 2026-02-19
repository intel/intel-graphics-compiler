;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-private-mem-usage-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryUsageAnalysis
; ------------------------------------------------
; This test checks that PrivateMemoryUsageAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; This is analysis pass, sanity check that debug info is not affected

; CHECK: @test_pma1{{.*}} !dbg [[SCOPE1:![0-9]*]]
;
; CHECK: [[ALLOCA_V:%[A-z0-9]*]] = alloca i32{{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32* [[ALLOCA_V]], metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: [[UDIV_V:%[A-z0-9]*]] = udiv i32 {{.*}} !dbg [[UDIV_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[UDIV_V]], metadata [[UDIV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UDIV_LOC]]
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]

%struct._st_foo = type { i32, <4 x float>, [2 x i64] }

define spir_kernel void @test_pma1(i32 %a, i32 %b) !dbg !12 {
  %1 = alloca i32, align 4, !dbg !19
  call void @llvm.dbg.value(metadata i32* %1, metadata !15, metadata !DIExpression()), !dbg !19
  %2 = udiv i32 %a, %b, !dbg !20
  call void @llvm.dbg.value(metadata i32 %2, metadata !17, metadata !DIExpression()), !dbg !20
  store i32 %2, i32* %1, !dbg !21
  ret void, !dbg !22
}

; CHECK: @test_pma2{{.*}} !dbg [[SCOPE2:![0-9]*]]
;
; CHECK: [[ADD_V:%[A-z0-9]*]] = add i32 {{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: store {{.*}} !dbg [[STORE2_LOC:![0-9]*]]

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_pma2(i32 %a, i32 %b, i32* %c) #0 !dbg !23 {
  %1 = add i32 %a, %b, !dbg !26
  call void @llvm.dbg.value(metadata i32 %1, metadata !25, metadata !DIExpression()), !dbg !26
  store i32 %1, i32* %c, !dbg !27
  ret void, !dbg !28
}

; CHECK: @test_pma3{{.*}} !dbg [[SCOPE3:![0-9]*]]
;
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load %struct._st_foo{{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata %struct._st_foo [[LOAD_V]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: [[EXTR_V:%[A-z0-9]*]] = extractvalue{{.*}} !dbg [[EXTR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[EXTR_V]], metadata [[EXTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR_LOC]]
; CHECK: store {{.*}} !dbg [[STORE3_LOC:![0-9]*]]

define spir_kernel void @test_pma3(%struct._st_foo* %a, i32* %b) !dbg !29 {
  %1 = load %struct._st_foo, %struct._st_foo* %a, !dbg !34
  call void @llvm.dbg.value(metadata %struct._st_foo %1, metadata !31, metadata !DIExpression()), !dbg !34
  %2 = extractvalue %struct._st_foo %1, 0, !dbg !35
  call void @llvm.dbg.value(metadata i32 %2, metadata !33, metadata !DIExpression()), !dbg !35
  store i32 %2, i32* %b, !dbg !36
  ret void, !dbg !37
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PrivateMemoryUsageAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test_pma1", linkageName: "test_pma1", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[UDIV_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE1]], file: [[FILE]], line: 2
; CHECK-DAG: [[UDIV_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test_pma2", linkageName: "test_pma2", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE2]], file: [[FILE]], line: 5
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[SCOPE3]] = distinct !DISubprogram(name: "test_pma3", linkageName: "test_pma3", scope: null, file: [[FILE]], line: 8
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE3]], file: [[FILE]], line: 8
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE3]])
; CHECK-DAG: [[EXTR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE3]], file: [[FILE]], line: 9
; CHECK-DAG: [[EXTR_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE3]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE3]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone "visaStackCall" }
attributes #1 = { nounwind readnone speculatable }

!igc.functions = !{!0, !4, !5}
!llvm.dbg.cu = !{!6}
!llvm.debugify = !{!9, !10}
!llvm.module.flags = !{!11}

!0 = !{void (i32, i32)* @test_pma1, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{void (i32, i32, i32*)* @test_pma2, !1}
!5 = !{void (%struct._st_foo*, i32*)* @test_pma3, !1}
!6 = distinct !DICompileUnit(language: DW_LANG_C, file: !7, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !8)
!7 = !DIFile(filename: "PrivateMemoryUsageAnalysis.ll", directory: "/")
!8 = !{}
!9 = !{i32 11}
!10 = !{i32 5}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = distinct !DISubprogram(name: "test_pma1", linkageName: "test_pma1", scope: null, file: !7, line: 1, type: !13, scopeLine: 1, unit: !6, retainedNodes: !14)
!13 = !DISubroutineType(types: !8)
!14 = !{!15, !17}
!15 = !DILocalVariable(name: "1", scope: !12, file: !7, line: 1, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "2", scope: !12, file: !7, line: 2, type: !18)
!18 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!19 = !DILocation(line: 1, column: 1, scope: !12)
!20 = !DILocation(line: 2, column: 1, scope: !12)
!21 = !DILocation(line: 3, column: 1, scope: !12)
!22 = !DILocation(line: 4, column: 1, scope: !12)
!23 = distinct !DISubprogram(name: "test_pma2", linkageName: "test_pma2", scope: null, file: !7, line: 5, type: !13, scopeLine: 5, unit: !6, retainedNodes: !24)
!24 = !{!25}
!25 = !DILocalVariable(name: "3", scope: !23, file: !7, line: 5, type: !18)
!26 = !DILocation(line: 5, column: 1, scope: !23)
!27 = !DILocation(line: 6, column: 1, scope: !23)
!28 = !DILocation(line: 7, column: 1, scope: !23)
!29 = distinct !DISubprogram(name: "test_pma3", linkageName: "test_pma3", scope: null, file: !7, line: 8, type: !13, scopeLine: 8, unit: !6, retainedNodes: !30)
!30 = !{!31, !33}
!31 = !DILocalVariable(name: "4", scope: !29, file: !7, line: 8, type: !32)
!32 = !DIBasicType(name: "ty384", size: 384, encoding: DW_ATE_unsigned)
!33 = !DILocalVariable(name: "5", scope: !29, file: !7, line: 9, type: !18)
!34 = !DILocation(line: 8, column: 1, scope: !29)
!35 = !DILocation(line: 9, column: 1, scope: !29)
!36 = !DILocation(line: 10, column: 1, scope: !29)
!37 = !DILocation(line: 11, column: 1, scope: !29)
