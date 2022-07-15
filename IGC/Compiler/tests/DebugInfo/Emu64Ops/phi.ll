;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-emu64ops -S < %s | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------
; This test checks that Emu64Ops pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_emu64{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[CALL_V:%[0-9A-z]*]] = call {{.*}}, !dbg [[CALL_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata i64 [[CALL_V]], metadata [[CALL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL_LOC]]
; CHECK: lbl:
; CHECK: phi {{.*}} !dbg [[PHI_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[PHI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI_LOC]]
; CHECK: @test_foo{{.*}} !dbg [[SCOPE_FOO:![0-9]*]]
; CHECK: ret {{.*}}, !dbg [[RET_LOC:![0-9]*]]

define void @test_emu64(i64 %a, i64 %b, i64* %dst) !dbg !12 {
entry:
  %0 = call i64 @test_foo(i64 %a, i64 %b), !dbg !21
  call void @llvm.dbg.value(metadata i64 %0, metadata !15, metadata !DIExpression()), !dbg !21
  br label %lbl, !dbg !22

lbl:                                              ; preds = %lbl, %entry
  %1 = phi i64 [ %a, %entry ], [ %2, %lbl ], !dbg !23
  call void @llvm.dbg.value(metadata i64 %1, metadata !17, metadata !DIExpression()), !dbg !23
  %2 = add i64 %1, 13, !dbg !24
  call void @llvm.dbg.value(metadata i64 %2, metadata !18, metadata !DIExpression()), !dbg !24
  %3 = icmp eq i64 %2, %0, !dbg !25
  call void @llvm.dbg.value(metadata i1 %3, metadata !19, metadata !DIExpression()), !dbg !25
  br i1 %3, label %lbl, label %end, !dbg !26

end:                                              ; preds = %lbl
  store i64 %2, i64* %dst, align 1, !dbg !27
  ret void, !dbg !28
}

define i64 @test_foo(i64 %a, i64 %b) !dbg !29 {
  %1 = add i64 %a, %b, !dbg !32
  call void @llvm.dbg.value(metadata i64 %1, metadata !31, metadata !DIExpression()), !dbg !32
  ret i64 %1, !dbg !33
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "phi.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_emu64", linkageName: "test_emu64", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[PHI_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SCOPE_FOO]] = distinct !DISubprogram(name: "test_foo", linkageName: "test_foo", scope: null, file: [[FILE]], line: 9
; CHECK-DAG: [[RET_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE_FOO]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0, !3}
!llvm.dbg.cu = !{!6}
!llvm.debugify = !{!9, !10}
!llvm.module.flags = !{!11}

!0 = !{void (i64, i64, i64*)* @test_emu64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i64 (i64, i64)* @test_foo, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 1}
!6 = distinct !DICompileUnit(language: DW_LANG_C, file: !7, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !8)
!7 = !DIFile(filename: "phi.ll", directory: "/")
!8 = !{}
!9 = !{i32 10}
!10 = !{i32 5}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = distinct !DISubprogram(name: "test_emu64", linkageName: "test_emu64", scope: null, file: !7, line: 1, type: !13, scopeLine: 1, unit: !6, retainedNodes: !14)
!13 = !DISubroutineType(types: !8)
!14 = !{!15, !17, !18, !19}
!15 = !DILocalVariable(name: "1", scope: !12, file: !7, line: 1, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "2", scope: !12, file: !7, line: 3, type: !16)
!18 = !DILocalVariable(name: "3", scope: !12, file: !7, line: 4, type: !16)
!19 = !DILocalVariable(name: "4", scope: !12, file: !7, line: 5, type: !20)
!20 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!21 = !DILocation(line: 1, column: 1, scope: !12)
!22 = !DILocation(line: 2, column: 1, scope: !12)
!23 = !DILocation(line: 3, column: 1, scope: !12)
!24 = !DILocation(line: 4, column: 1, scope: !12)
!25 = !DILocation(line: 5, column: 1, scope: !12)
!26 = !DILocation(line: 6, column: 1, scope: !12)
!27 = !DILocation(line: 7, column: 1, scope: !12)
!28 = !DILocation(line: 8, column: 1, scope: !12)
!29 = distinct !DISubprogram(name: "test_foo", linkageName: "test_foo", scope: null, file: !7, line: 9, type: !13, scopeLine: 9, unit: !6, retainedNodes: !30)
!30 = !{!31}
!31 = !DILocalVariable(name: "5", scope: !29, file: !7, line: 9, type: !16)
!32 = !DILocation(line: 9, column: 1, scope: !29)
!33 = !DILocation(line: 10, column: 1, scope: !29)
