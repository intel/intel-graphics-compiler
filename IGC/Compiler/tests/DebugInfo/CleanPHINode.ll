;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-cleanphinode -S < %s | FileCheck %s
; ------------------------------------------------
; CleanPHINode
; ------------------------------------------------
; This test checks that CleanPHINode pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_func void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: end:
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* %0, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]


define spir_func void @test(i1 %src1, i32* %src2) !dbg !6 {
entry:
  %0 = getelementptr i32, i32* %src2, i32 4, !dbg !16
  call void @llvm.dbg.value(metadata i32* %0, metadata !9, metadata !DIExpression()), !dbg !16
  br i1 %src1, label %bb1, label %bb2, !dbg !17

bb1:                                              ; preds = %entry
  br label %end, !dbg !18

bb2:                                              ; preds = %entry
  %1 = getelementptr i32, i32* %src2, i32 2, !dbg !19
  call void @llvm.dbg.value(metadata i32* %1, metadata !11, metadata !DIExpression()), !dbg !19
  br label %end, !dbg !20

end:                                              ; preds = %bb2, %bb1
  %2 = phi i32* [ %0, %bb1 ], [ %0, %bb2 ], !dbg !21
  %3 = phi i32* [ %0, %bb1 ], [ %1, %bb2 ], !dbg !22
  call void @llvm.dbg.value(metadata i32* %2, metadata !12, metadata !DIExpression()), !dbg !21
  call void @llvm.dbg.value(metadata i32* %3, metadata !13, metadata !DIExpression()), !dbg !22
  %4 = load i32, i32* %2, !dbg !23
  call void @llvm.dbg.value(metadata i32 %4, metadata !14, metadata !DIExpression()), !dbg !23
  store i32 %4, i32* %3, !dbg !24
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "CleanPHINode.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "CleanPHINode.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 4, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 6, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 7, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 8, type: !15)
!15 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
!24 = !DILocation(line: 9, column: 1, scope: !6)
!25 = !DILocation(line: 10, column: 1, scope: !6)
