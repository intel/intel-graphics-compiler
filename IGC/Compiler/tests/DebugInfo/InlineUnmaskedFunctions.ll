;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -inline-unmasked -S < %s | FileCheck %s
; ------------------------------------------------
; InlineUnmaskedFunctions
; ------------------------------------------------
; This test checks that InlineUnmaskedFunctions pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[FVAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[FVAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[FVAL1_V]], metadata [[FVAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FVAL1_LOC]]
; CHECK: [[FVAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[FVAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[FVAL2_V]], metadata [[FVAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FVAL2_LOC]]
; CHECK: store {{.*}}, !dbg [[FSTR1_LOC:![0-9]*]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: store {{.*}}, !dbg [[STR1_LOC:![0-9]*]]

define void @test(i32 %src, i32* %dst) !dbg !17 {
  %1 = load i32, i32* %dst, !dbg !23
  call void @llvm.dbg.value(metadata i32 %1, metadata !20, metadata !DIExpression()), !dbg !23
  call void @foo(i32 %1, i32* %dst), !dbg !24
  %2 = load i32, i32* %dst, !dbg !25
  call void @llvm.dbg.value(metadata i32 %2, metadata !22, metadata !DIExpression()), !dbg !25
  store i32 %2, i32* %dst, !dbg !26
  ret void, !dbg !27
}

define void @foo(i32 %s1, i32* %d1) #0 !dbg !28 {
  %1 = load i32, i32* %d1, !dbg !32
  call void @llvm.dbg.value(metadata i32 %1, metadata !30, metadata !DIExpression()), !dbg !32
  %2 = add i32 %s1, %1, !dbg !33
  call void @llvm.dbg.value(metadata i32 %2, metadata !31, metadata !DIExpression()), !dbg !33
  store i32 %2, i32* %d1, !dbg !34
  ret void, !dbg !35
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "InlineUnmaskedFunctions.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SCOPE2:![0-9]*]] = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: [[FILE]], line: 6
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[INLINED:![0-9]*]] = distinct !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE1]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[FVAL1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE2]], file: [[FILE]], line: 6
; CHECK-DAG: [[FVAL1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE2]], inlinedAt: [[INLINED]])
; CHECK-DAG: [[FVAL2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE2]], file: [[FILE]], line: 7
; CHECK-DAG: [[FVAL2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE2]], inlinedAt: [[INLINED]])
; CHECK-DAG: [[FSTR1_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE2]], inlinedAt: [[INLINED]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "sycl-unmasked" }
attributes #1 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!11}
!llvm.debugify = !{!14, !15}
!llvm.module.flags = !{!16}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !9, !10}
!2 = !{!"FuncMDMap[0]", void (i32, i32*)* @test}
!3 = !{!"FuncMDValue[0]", !4, !5}
!4 = !{!"localOffsets"}
!5 = !{!"workGroupWalkOrder", !6, !7, !8}
!6 = !{!"dim0", i32 0}
!7 = !{!"dim1", i32 0}
!8 = !{!"dim2", i32 0}
!9 = !{!"FuncMDMap[1]", void (i32, i32*)* @foo}
!10 = !{!"FuncMDValue[1]", !4, !5}
!11 = distinct !DICompileUnit(language: DW_LANG_C, file: !12, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !13)
!12 = !DIFile(filename: "InlineUnmaskedFunctions.ll", directory: "/")
!13 = !{}
!14 = !{i32 9}
!15 = !{i32 4}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !12, line: 1, type: !18, scopeLine: 1, unit: !11, retainedNodes: !19)
!18 = !DISubroutineType(types: !13)
!19 = !{!20, !22}
!20 = !DILocalVariable(name: "1", scope: !17, file: !12, line: 1, type: !21)
!21 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "2", scope: !17, file: !12, line: 3, type: !21)
!23 = !DILocation(line: 1, column: 1, scope: !17)
!24 = !DILocation(line: 2, column: 1, scope: !17)
!25 = !DILocation(line: 3, column: 1, scope: !17)
!26 = !DILocation(line: 4, column: 1, scope: !17)
!27 = !DILocation(line: 5, column: 1, scope: !17)
!28 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !12, line: 6, type: !18, scopeLine: 6, unit: !11, retainedNodes: !29)
!29 = !{!30, !31}
!30 = !DILocalVariable(name: "3", scope: !28, file: !12, line: 6, type: !21)
!31 = !DILocalVariable(name: "4", scope: !28, file: !12, line: 7, type: !21)
!32 = !DILocation(line: 6, column: 1, scope: !28)
!33 = !DILocation(line: 7, column: 1, scope: !28)
!34 = !DILocation(line: 8, column: 1, scope: !28)
!35 = !DILocation(line: 9, column: 1, scope: !28)
