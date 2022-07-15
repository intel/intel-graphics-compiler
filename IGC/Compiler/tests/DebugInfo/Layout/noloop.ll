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
; CHECK: [[CMP_V:%[0-9]*]] = {{.*}} !dbg [[CMP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i1 [[CMP_V]], metadata [[CMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CMP_LOC]]
; CHECK: br{{.*}} !dbg [[BR1_LOC:![0-9]*]]
;
; CHECK: lbl:
; CHECK: [[P2I_V:%[0-9]*]] = {{.*}} !dbg [[P2I_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[P2I_V]], metadata [[P2I_MD:![0-9]*]], metadata !DIExpression()), !dbg [[P2I_LOC]]
; CHECK: [[SHL_V:%[0-9]*]] = {{.*}} !dbg [[SHL_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[SHL_V]], metadata [[SHL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHL_LOC]]
; CHECK: br{{.*}} !dbg [[BR2_LOC:![0-9]*]]

; CHECK: lbl2:
; CHECK: store{{.*}} !dbg [[STORE1_LOC:![0-9]*]]

; CHECK: end:
; CHECK: [[ADD_V:%[0-9]*]] = {{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: store{{.*}} !dbg [[STORE2_LOC:![0-9]*]]

define void @test_layout(i32* %a) !dbg !6 {
entry:
  %0 = load i32, i32* %a, !dbg !16
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = icmp slt i32 %0, 13, !dbg !17
  call void @llvm.dbg.value(metadata i1 %1, metadata !11, metadata !DIExpression()), !dbg !17
  br i1 %1, label %end, label %lbl, !dbg !18

lbl:                                              ; preds = %entry
  %2 = ptrtoint i32* %a to i32, !dbg !19
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !19
  %3 = shl i32 %2, 2, !dbg !20
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !20
  br label %lbl2, !dbg !21

lbl2:                                             ; preds = %lbl
  store i32 %3, i32* %a, !dbg !22
  ret void, !dbg !23

end:                                              ; preds = %entry
  %4 = add i32 %0, 15, !dbg !24
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !24
  store i32 %4, i32* %a, !dbg !25
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "noloop.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_layout", linkageName: "test_layout", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CMP_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[CMP_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[P2I_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[P2I_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SHL_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[SHL_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "noloop.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_layout", linkageName: "test_layout", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 9, type: !10)
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
!26 = !DILocation(line: 11, column: 1, scope: !6)
