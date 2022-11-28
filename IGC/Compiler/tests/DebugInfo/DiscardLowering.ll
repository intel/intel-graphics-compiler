;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-lower-discard -S < %s | FileCheck %s
; ------------------------------------------------
; DiscardLowering
; ------------------------------------------------
; This test checks that DiscardLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: void @test_discardlow
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i1 [[HELP_V:%[A-z0-9]*]], metadata [[HELP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[HELP_LOC:![0-9]*]]
; CHECK-DAG: [[HELP_V]] = {{.*}}, !dbg [[HELP_LOC]]
; CHECK: !dbg [[DISCARD_LOC:![0-9]*]]
; CHECK: ret

define void @test_discardlow(i1 %a) !dbg !10 {
entry:
  %0 = call i1 @llvm.genx.GenISA.IsHelperInvocation(), !dbg !15
  call void @llvm.dbg.value(metadata i1 %0, metadata !13, metadata !DIExpression()), !dbg !15
  call void @llvm.genx.GenISA.discard(i1 %0), !dbg !16
  ret void, !dbg !17
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "DiscardLowering.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_discardlow", linkageName: "test_discardlow", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[HELP_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1, type: !14)
; CHECK-DAG: [[HELP_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[DISCARD_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])


declare void @llvm.genx.GenISA.discard(i1)

declare i1 @llvm.genx.GenISA.IsHelperInvocation()

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i1)* @test_discardlow, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "DiscardLowering.ll", directory: "/")
!6 = !{}
!7 = !{i32 3}
!8 = !{i32 1}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_discardlow", linkageName: "test_discardlow", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!15 = !DILocation(line: 1, column: 1, scope: !10)
!16 = !DILocation(line: 2, column: 1, scope: !10)
!17 = !DILocation(line: 3, column: 1, scope: !10)
