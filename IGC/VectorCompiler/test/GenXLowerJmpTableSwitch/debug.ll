;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXLowerJmpTableSwitch -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLowerJmpTableSwitch
; ------------------------------------------------
; This test checks that GenXLowerJmpTableSwitch pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Note: table call could also have line loc, but thats improvement

; CHECK: void @test_lowerswitch{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = trunc{{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i16 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: switch i64{{.*}}
; CHECK: ], !dbg [[SW1_LOC:![0-9]*]]
; CHECK: lowered_switch:
; CHECK: br{{.*}}, !dbg [[SW2_LOC:![0-9]*]]
; CHECK: indirectbr{{.*}}, !dbg [[SW2_LOC]]
; CHECK: ending:
; CHECK: br{{.*}}, !dbg [[B1_LOC:![0-9]*]]
; CHECK: ret:
; CHECK: br{{.*}}, !dbg [[B2_LOC:![0-9]*]]
; CHECK: end:
; CHECK: [[VAL2_V:%[A-z0-9]*]] = phi{{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i16 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]

define void @test_lowerswitch(i64 %cond, i16* %dst) !dbg !6 {
entry:
  %0 = trunc i64 %cond to i16, !dbg !13
  call void @llvm.dbg.value(metadata i16 %0, metadata !9, metadata !DIExpression()), !dbg !13
  switch i64 0, label %lowered_switch [
  ], !dbg !14

lowered_switch:                                   ; preds = %ret, %entry
  %1 = phi i16 [ %0, %entry ], [ 13, %ret ], !dbg !15
  call void @llvm.dbg.value(metadata i16 %1, metadata !11, metadata !DIExpression()), !dbg !15
  switch i16 %1, label %end [
    i16 -5, label %ending
    i16 -4, label %ending
    i16 -1, label %ending
    i16 -3, label %ret
    i16 -2, label %ret
  ], !dbg !16

ending:                                           ; preds = %lowered_switch, %lowered_switch, %lowered_switch
  br label %end, !dbg !17

ret:                                              ; preds = %lowered_switch, %lowered_switch
  br label %lowered_switch, !dbg !18

end:                                              ; preds = %ending, %lowered_switch
  %2 = phi i16 [ %1, %lowered_switch ], [ %0, %ending ], !dbg !19
  call void @llvm.dbg.value(metadata i16 %2, metadata !12, metadata !DIExpression()), !dbg !19
  store i16 %2, i16* %dst, !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXLowerJmpTableSwitch.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_lowerswitch", linkageName: "test_lowerswitch", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SW1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SW2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[B1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[B2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXLowerJmpTableSwitch.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_lowerswitch", linkageName: "test_lowerswitch", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 7, type: !10)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = !DILocation(line: 5, column: 1, scope: !6)
!18 = !DILocation(line: 6, column: 1, scope: !6)
!19 = !DILocation(line: 7, column: 1, scope: !6)
!20 = !DILocation(line: 8, column: 1, scope: !6)
!21 = !DILocation(line: 9, column: 1, scope: !6)
