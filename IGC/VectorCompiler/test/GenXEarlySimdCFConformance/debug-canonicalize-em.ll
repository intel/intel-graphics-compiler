;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXEarlySimdCFConformance
; ------------------------------------------------
; This test checks that GenXEarlySimdCFConformance pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define dllexport void @test_kernel{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: exit:
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i1> [[BCAST_V:%[A-z0-9]*]], metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC:![0-9]*]]
; And/or results are opted out
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i32> [[SELECT1_V:%[A-z0-9]*]], metadata [[SELECT1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i32> [[SELECT2_V:%[A-z0-9]*]], metadata [[SELECT2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT2_LOC:![0-9]*]]
; CHECK-DAG: [[BCAST_V]] = {{.*}}, !dbg [[BCAST_LOC]]
; CHECK-DAG: [[SELECT1_V]] = {{.*}}, !dbg [[SELECT1_LOC]]
; CHECK-DAG: [[SELECT2_V]] = {{.*}}, !dbg [[SELECT2_LOC]]

define dllexport void @test_kernel(<32 x i32>* %a, <32 x i1> %b) !dbg !6 {
entry:
  %0 = load <32 x i32>, <32 x i32>* %a, !dbg !25
  call void @llvm.dbg.value(metadata <32 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !25
  br label %lbl, !dbg !26

lbl:                                              ; preds = %lbl, %entry
  %1 = call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %b), !dbg !27
  call void @llvm.dbg.value(metadata { <32 x i1>, <32 x i1>, i1 } %1, metadata !11, metadata !DIExpression()), !dbg !27
  %2 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 0, !dbg !28
  call void @llvm.dbg.value(metadata <32 x i1> %2, metadata !13, metadata !DIExpression()), !dbg !28
  %3 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 1, !dbg !29
  call void @llvm.dbg.value(metadata <32 x i1> %3, metadata !15, metadata !DIExpression()), !dbg !29
  %4 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 2, !dbg !30
  call void @llvm.dbg.value(metadata i1 %4, metadata !16, metadata !DIExpression()), !dbg !30
  br i1 %4, label %exit, label %lbl, !dbg !31

exit:                                             ; preds = %lbl
  %5 = bitcast i32 1244252123 to <32 x i1>, !dbg !32
  call void @llvm.dbg.value(metadata <32 x i1> %5, metadata !18, metadata !DIExpression()), !dbg !32
  %6 = and <32 x i1> %2, %5, !dbg !33
  call void @llvm.dbg.value(metadata <32 x i1> %6, metadata !19, metadata !DIExpression()), !dbg !33
  %7 = or <32 x i1> %6, %2, !dbg !34
  call void @llvm.dbg.value(metadata <32 x i1> %7, metadata !20, metadata !DIExpression()), !dbg !34
  %8 = select <32 x i1> %6, <32 x i32> %0, <32 x i32> zeroinitializer, !dbg !35
  call void @llvm.dbg.value(metadata <32 x i32> %8, metadata !21, metadata !DIExpression()), !dbg !35
  %9 = select <32 x i1> %7, <32 x i32> %8, <32 x i32> %0, !dbg !36
  call void @llvm.dbg.value(metadata <32 x i32> %9, metadata !22, metadata !DIExpression()), !dbg !36
  store <32 x i32> %9, <32 x i32>* %a, !dbg !37
  %10 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %2, <32 x i1> %3), !dbg !38
  call void @llvm.dbg.value(metadata { <32 x i1>, i1 } %10, metadata !23, metadata !DIExpression()), !dbg !38
  ret void, !dbg !39
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "canonicalize-em.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT1_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[SELECT1_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT2_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[SELECT2_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>)

declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "canonicalize-em.ll", directory: "/")
!2 = !{}
!3 = !{i32 15}
!4 = !{i32 11}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15, !16, !18, !19, !20, !21, !22, !23}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty1024", size: 1024, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !12)
!12 = !DIBasicType(name: "ty768", size: 768, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !14)
!14 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !14)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !17)
!17 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 8, type: !14)
!19 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 9, type: !14)
!20 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 10, type: !14)
!21 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 11, type: !10)
!22 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 12, type: !10)
!23 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 14, type: !24)
!24 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!25 = !DILocation(line: 1, column: 1, scope: !6)
!26 = !DILocation(line: 2, column: 1, scope: !6)
!27 = !DILocation(line: 3, column: 1, scope: !6)
!28 = !DILocation(line: 4, column: 1, scope: !6)
!29 = !DILocation(line: 5, column: 1, scope: !6)
!30 = !DILocation(line: 6, column: 1, scope: !6)
!31 = !DILocation(line: 7, column: 1, scope: !6)
!32 = !DILocation(line: 8, column: 1, scope: !6)
!33 = !DILocation(line: 9, column: 1, scope: !6)
!34 = !DILocation(line: 10, column: 1, scope: !6)
!35 = !DILocation(line: 11, column: 1, scope: !6)
!36 = !DILocation(line: 12, column: 1, scope: !6)
!37 = !DILocation(line: 13, column: 1, scope: !6)
!38 = !DILocation(line: 14, column: 1, scope: !6)
!39 = !DILocation(line: 15, column: 1, scope: !6)
