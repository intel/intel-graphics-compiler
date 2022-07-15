;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-PartialEmuI64Ops -S < %s | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : expand call args
; ------------------------------------------------
; This test checks that PartialEmuI64Ops pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_part64{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[MUL_V:%[A-z0-9]*]] = bitcast{{.*}} to i64, !dbg [[MUL_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata i64 [[MUL_V]], metadata [[MUL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[MUL_LOC]]
; CHECK: [[CALL_V:%[A-z0-9]*]] = call{{.*}}, !dbg [[CALL_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata i64 [[CALL_V]], metadata [[CALL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL_LOC]]

define void @test_part64(i64 %a, i64 %b, i64* %dst) !dbg !12 {
  %1 = mul i64 %a, %b, !dbg !18
  call void @llvm.dbg.value(metadata i64 %1, metadata !15, metadata !DIExpression()), !dbg !18
  %2 = call i64 @test_foo(i64 %a, i64 %1), !dbg !19
  call void @llvm.dbg.value(metadata i64 %2, metadata !17, metadata !DIExpression()), !dbg !19
  store i64 %2, i64* %dst, !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "expandargs.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_part64", linkageName: "test_part64", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[MUL_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[MUL_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])

declare i64 @test_foo(i64, i64)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0, !3}
!llvm.dbg.cu = !{!6}
!llvm.debugify = !{!9, !10}
!llvm.module.flags = !{!11}

!0 = !{void (i64, i64, i64*)* @test_part64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i64 (i64, i64)* @test_foo, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 1}
!6 = distinct !DICompileUnit(language: DW_LANG_C, file: !7, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !8)
!7 = !DIFile(filename: "expandargs.ll", directory: "/")
!8 = !{}
!9 = !{i32 4}
!10 = !{i32 2}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = distinct !DISubprogram(name: "test_part64", linkageName: "test_part64", scope: null, file: !7, line: 1, type: !13, scopeLine: 1, unit: !6, retainedNodes: !14)
!13 = !DISubroutineType(types: !8)
!14 = !{!15, !17}
!15 = !DILocalVariable(name: "1", scope: !12, file: !7, line: 1, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "2", scope: !12, file: !7, line: 2, type: !16)
!18 = !DILocation(line: 1, column: 1, scope: !12)
!19 = !DILocation(line: 2, column: 1, scope: !12)
!20 = !DILocation(line: 3, column: 1, scope: !12)
!21 = !DILocation(line: 4, column: 1, scope: !12)
