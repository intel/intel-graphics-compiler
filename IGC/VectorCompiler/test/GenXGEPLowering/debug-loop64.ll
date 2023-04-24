;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXGEPLowering
; ------------------------------------------------
; This test checks that GenXGEPLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Gep loop with 64b pointers

; CHECK: void @lower_gep{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = load{{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
;
; CHECK: loop:
; CHECK: [[VAL2_V:%[A-z0-9]*]] = phi{{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32* [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
define void @lower_gep(i32* %a) !dbg !6 {
entry:
  %0 = load i32, i32* %a, !dbg !18
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !18
  br label %loop, !dbg !19

loop:                                             ; preds = %loop, %entry
  %1 = phi i32 [ 1, %entry ], [ %4, %loop ], !dbg !20
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !20
  %2 = add i32 %0, %1, !dbg !21
  call void @llvm.dbg.value(metadata i32 %2, metadata !12, metadata !DIExpression()), !dbg !21
  %3 = getelementptr inbounds i32, i32* %a, i32 %2, !dbg !22
  call void @llvm.dbg.value(metadata i32* %3, metadata !13, metadata !DIExpression()), !dbg !22
  %4 = load i32, i32* %3, !dbg !23
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !23
  %5 = icmp sge i32 %4, 1, !dbg !24
  call void @llvm.dbg.value(metadata i1 %5, metadata !16, metadata !DIExpression()), !dbg !24
  br i1 %5, label %loop, label %exit, !dbg !25

exit:                                             ; preds = %loop
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "loop_64.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "lower_gep", linkageName: "lower_gep", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "loop_64.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "lower_gep", linkageName: "lower_gep", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !17)
!17 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 7, column: 1, scope: !6)
!25 = !DILocation(line: 8, column: 1, scope: !6)
!26 = !DILocation(line: 9, column: 1, scope: !6)
