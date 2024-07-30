;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass
; ------------------------------------------------
; This test checks that CustomSafeOptPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define spir_kernel void @test_customsafe{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; Note other diexpr than xor could be used
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL2_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value)), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: store i32 [[VAL3_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; Note other diexpr than xor could be used
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL5_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression(DW_OP_constu, 1, DW_OP_xor, DW_OP_stack_value)), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]

define spir_kernel void @test_customsafe(i32 %a, i32 %b, i32* %c) !dbg !6 {
entry:
  %0 = icmp eq i32 %a, %b, !dbg !16
  call void @llvm.dbg.value(metadata i1 %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = xor i1 %0, true, !dbg !17
  call void @llvm.dbg.value(metadata i1 %1, metadata !11, metadata !DIExpression()), !dbg !17
  %2 = select i1 %1, i32 %a, i32 %b, !dbg !18
  call void @llvm.dbg.value(metadata i32 %2, metadata !12, metadata !DIExpression()), !dbg !18
  store i32 %2, i32* %c, !dbg !19
  %3 = icmp slt i32 %a, %b, !dbg !20
  call void @llvm.dbg.value(metadata i1 %3, metadata !14, metadata !DIExpression()), !dbg !20
  %4 = xor i1 %3, true, !dbg !21
  call void @llvm.dbg.value(metadata i1 %4, metadata !15, metadata !DIExpression()), !dbg !21
  br i1 %4, label %tr, label %end, !dbg !22

tr:                                               ; preds = %entry
  store i32 %a, i32* %c, !dbg !23
  br label %end, !dbg !24

end:                                              ; preds = %tr, %entry
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "xor.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "xor.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
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
