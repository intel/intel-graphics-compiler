;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-constant-coalescing -S < %s | FileCheck %s
; ------------------------------------------------
; ConstantCoalescing
; ------------------------------------------------
; This test checks that ConstantCoalescing pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_constcoal{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: entry:
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[LOAD1_V:%[A-z0-9]*]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD1_V]] = {{.*}} !dbg [[LOAD1_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[LOAD2_V:%[A-z0-9]*]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD2_V]] = {{.*}} !dbg [[LOAD1_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[LOAD3_V:%[A-z0-9]*]], metadata [[LOAD3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD3_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD3_V]] = {{.*}} !dbg [[LOAD1_LOC]]

define void @test_constcoal(i32* %a) !dbg !10 {
entry:
  %0 = inttoptr i32 4 to i32 addrspace(65536)*, !dbg !21
  call void @llvm.dbg.value(metadata i32 addrspace(65536)* %0, metadata !13, metadata !DIExpression()), !dbg !21
  %1 = inttoptr i32 8 to i32 addrspace(65536)*, !dbg !22
  call void @llvm.dbg.value(metadata i32 addrspace(65536)* %1, metadata !15, metadata !DIExpression()), !dbg !22
  %2 = inttoptr i32 16 to i32 addrspace(65536)*, !dbg !23
  call void @llvm.dbg.value(metadata i32 addrspace(65536)* %2, metadata !16, metadata !DIExpression()), !dbg !23
  %3 = load i32, i32 addrspace(65536)* %0, align 4, !dbg !24
  call void @llvm.dbg.value(metadata i32 %3, metadata !17, metadata !DIExpression()), !dbg !24
  br label %lbl1, !dbg !25

lbl1:                                             ; preds = %entry
  %4 = load i32, i32 addrspace(65536)* %1, align 4, !dbg !26
  call void @llvm.dbg.value(metadata i32 %4, metadata !19, metadata !DIExpression()), !dbg !26
  br label %lbl2, !dbg !27

lbl2:                                             ; preds = %lbl1
  %5 = load i32, i32 addrspace(65536)* %2, align 4, !dbg !28
  call void @llvm.dbg.value(metadata i32 %5, metadata !20, metadata !DIExpression()), !dbg !28
  br label %end, !dbg !29

end:                                              ; preds = %lbl2
  store i32 %3, i32* %a, !dbg !30
  store i32 %4, i32* %a, !dbg !31
  store i32 %5, i32* %a, !dbg !32
  ret void, !dbg !33
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "directcb.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_constcoal", linkageName: "test_constcoal", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD3_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[LOAD3_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32*)* @test_constcoal, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "directcb.ll", directory: "/")
!6 = !{}
!7 = !{i32 13}
!8 = !{i32 6}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_constcoal", linkageName: "test_constcoal", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !17, !19, !20}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !14)
!17 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !18)
!18 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 6, type: !18)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 8, type: !18)
!21 = !DILocation(line: 1, column: 1, scope: !10)
!22 = !DILocation(line: 2, column: 1, scope: !10)
!23 = !DILocation(line: 3, column: 1, scope: !10)
!24 = !DILocation(line: 4, column: 1, scope: !10)
!25 = !DILocation(line: 5, column: 1, scope: !10)
!26 = !DILocation(line: 6, column: 1, scope: !10)
!27 = !DILocation(line: 7, column: 1, scope: !10)
!28 = !DILocation(line: 8, column: 1, scope: !10)
!29 = !DILocation(line: 9, column: 1, scope: !10)
!30 = !DILocation(line: 10, column: 1, scope: !10)
!31 = !DILocation(line: 11, column: 1, scope: !10)
!32 = !DILocation(line: 12, column: 1, scope: !10)
!33 = !DILocation(line: 13, column: 1, scope: !10)
