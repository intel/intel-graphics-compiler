;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-layout -S < %s | FileCheck %s
; ------------------------------------------------
; Layout
; ------------------------------------------------
; This test checks that Layout pass follows
; 'How to Update Debug Info' llvm guideline.
;
; This pass does some block reordering, check that Debug stays correct.
;
; ------------------------------------------------

; CHECK: define void @test_layout{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: entry:
; CHECK: [[LOAD_V:%[0-9]*]] = {{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[LOAD_V]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: [[CMP1_V:%[0-9]*]] = {{.*}} !dbg [[CMP1_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i1 [[CMP1_V]], metadata [[CMP1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CMP1_LOC]]
; CHECK: br{{.*}} !dbg [[BR1_LOC:![0-9]*]]
;
; CHECK: lbl1:
; CHECK: [[P2I_V:%[0-9]*]] = {{.*}} !dbg [[P2I_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[P2I_V]], metadata [[P2I_MD:![0-9]*]], metadata !DIExpression()), !dbg [[P2I_LOC]]
; CHECK: [[SHL_V:%[0-9]*]] = {{.*}} !dbg [[SHL_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[SHL_V]], metadata [[SHL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHL_LOC]]
; CHECK: [[CMP2_V:%[0-9]*]] = {{.*}} !dbg [[CMP2_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i1 [[CMP2_V]], metadata [[CMP2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CMP2_LOC]]
; CHECK: [[AOR_V:%[A-z0-9.]*]] = {{.*}} !dbg [[AOR_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[AOR_V]], metadata [[AOR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AOR_LOC]]
; CHECK: br{{.*}} !dbg [[BR2_LOC:![0-9]*]]

; CHECK: lbl2:
; CHECK: [[ADD_V:%[0-9]*]] = {{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: store{{.*}} !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[CMP3_V:%[0-9]*]] = {{.*}} !dbg [[CMP3_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i1 [[CMP3_V]], metadata [[CMP3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CMP3_LOC]]
; CHECK: [[AXCHG_V:%[A-z0-9.]*]] = {{.*}} !dbg [[AXCHG_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[AXCHG_V]], metadata [[AXCHG_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AXCHG_LOC]]
; CHECK: br{{.*}} !dbg [[BR3_LOC:![0-9]*]]

; CHECK: lbl4:
; CHECK: ret{{.*}} !dbg [[RET1_LOC:![0-9]*]]

; CHECK: lbl3:
; CHECK: store{{.*}} !dbg [[STORE2_LOC:![0-9]*]]


define void @test_layout(i32* %a) !dbg !6 {
entry:
  %0 = load i32, i32* %a, !dbg !20
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !20
  %1 = icmp slt i32 %0, 13, !dbg !21
  call void @llvm.dbg.value(metadata i1 %1, metadata !11, metadata !DIExpression()), !dbg !21
  br i1 %1, label %lbl1, label %lbl3, !dbg !22

lbl1:                                             ; preds = %lbl1, %entry
  %2 = ptrtoint i32* %a to i32, !dbg !23
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !23
  %3 = shl i32 %2, 2, !dbg !24
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !24
  %4 = icmp eq i32 %3, %0, !dbg !25
  call void @llvm.dbg.value(metadata i1 %4, metadata !15, metadata !DIExpression()), !dbg !25
  %a.2 = call i32 @llvm.genx.GenISA.intatomicraw.i32.p0i32.i32(i32* %a, i32 %0, i32 0, i32 9), !dbg !26
  call void @llvm.dbg.value(metadata i32 %a.2, metadata !16, metadata !DIExpression()), !dbg !26
  br i1 %4, label %lbl1, label %lbl2, !dbg !27

lbl2:                                             ; preds = %lbl2, %lbl1
  %5 = add i32 %0, 15, !dbg !28
  call void @llvm.dbg.value(metadata i32 %5, metadata !17, metadata !DIExpression()), !dbg !28
  store i32 %5, i32* %a, !dbg !29
  %6 = icmp ne i32 %5, 16, !dbg !30
  call void @llvm.dbg.value(metadata i1 %6, metadata !18, metadata !DIExpression()), !dbg !30
  %a.1 = call i32 @llvm.genx.GenISA.icmpxchgatomicraw.i32.p0i32.i32(i32* %a, i32 %0, i32 0, i32 0), !dbg !31
  call void @llvm.dbg.value(metadata i32 %a.1, metadata !19, metadata !DIExpression()), !dbg !31
  br i1 %6, label %lbl2, label %lbl4, !dbg !32

lbl4:                                             ; preds = %lbl2
  ret void, !dbg !33

lbl3:                                             ; preds = %entry
  store i32 %0, i32* %a, !dbg !34
  ret void, !dbg !35
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "loop_1.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_layout", linkageName: "test_layout", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CMP1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[CMP1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[P2I_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[P2I_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SHL_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[SHL_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CMP2_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[CMP2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[AOR_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[AOR_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR2_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CMP3_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[CMP3_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[AXCHG_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[AXCHG_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR3_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[RET1_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])


declare i32 @llvm.genx.GenISA.icmpxchgatomicraw.i32.p0i32.i32(i32*, i32, i32, i32)

declare i32 @llvm.genx.GenISA.intatomicraw.i32.p0i32.i32(i32*, i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "loop_1.ll", directory: "/")
!2 = !{}
!3 = !{i32 16}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_layout", linkageName: "test_layout", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16, !17, !18, !19}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !12)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !10)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 9, type: !10)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 11, type: !12)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 12, type: !10)
!20 = !DILocation(line: 1, column: 1, scope: !6)
!21 = !DILocation(line: 2, column: 1, scope: !6)
!22 = !DILocation(line: 3, column: 1, scope: !6)
!23 = !DILocation(line: 4, column: 1, scope: !6)
!24 = !DILocation(line: 5, column: 1, scope: !6)
!25 = !DILocation(line: 6, column: 1, scope: !6)
!26 = !DILocation(line: 7, column: 1, scope: !6)
!27 = !DILocation(line: 8, column: 1, scope: !6)
!28 = !DILocation(line: 9, column: 1, scope: !6)
!29 = !DILocation(line: 10, column: 1, scope: !6)
!30 = !DILocation(line: 11, column: 1, scope: !6)
!31 = !DILocation(line: 12, column: 1, scope: !6)
!32 = !DILocation(line: 13, column: 1, scope: !6)
!33 = !DILocation(line: 14, column: 1, scope: !6)
!34 = !DILocation(line: 15, column: 1, scope: !6)
!35 = !DILocation(line: 16, column: 1, scope: !6)
