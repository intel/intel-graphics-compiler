;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-pre-ra-remat-flag -S < %s | FileCheck %s
; ------------------------------------------------
; PreRARematFlag
; ------------------------------------------------
; This test checks that PreRARematFlag pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; This pass adds copies of cmp and logical operations, check that they and their
; uses have dbg info preserved.


; CHECK: @test_preraremat{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[ALLOC_V:%[A-z0-9]*]] = alloca i32{{.*}} !dbg [[ALLOC_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32* [[ALLOC_V]], metadata [[ALLOC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC_LOC]]
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: [[ICMPE_V:%[A-z0-9]*]] = icmp eq i32{{.*}} !dbg [[ICMPE_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i1 [[ICMPE_V]], metadata [[ICMPE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMPE_LOC]]
; CHECK: [[ICMPGT_V:%[A-z0-9]*]] = icmp ugt i32{{.*}} !dbg [[ICMPGT_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i1 [[ICMPGT_V]], metadata [[ICMPGT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMPGT_LOC]]
; CHECK: [[AND1_V:%[A-z0-9]*]] = and i1{{.*}} !dbg [[AND1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i1 [[AND1_V]], metadata [[AND1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AND1_LOC]]

; CHECK-DAG: [[ICMPGTC1_V:%[A-z0-9]*]] = icmp ugt i32{{.*}} !dbg [[ICMPGT_LOC]]
; CHECK-DAG: [[ICMPGTC2_V:%[A-z0-9]*]] = icmp ugt i32{{.*}} !dbg [[ICMPGT_LOC]]
; CHECK-DAG: [[ICMPEQC1_V:%[A-z0-9]*]] = icmp eq i32{{.*}} !dbg [[ICMPE_LOC]]
; CHECK: [[AND1C1_V:%[A-z0-9]*]] = and i1 [[ICMPEQC1_V]], [[ICMPGTC2_V]], !dbg [[AND1_LOC]]
; CHECK: br i1 [[AND1C1_V]]{{.*}} !dbg [[BR1_LOC:![0-9]*]]
;
; CHECK: lbl1:
; CHECK: [[PHI32_V:%[A-z0-9]*]] = phi i32{{.*}} !dbg [[PHI32_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[PHI32_V]], metadata [[PHI32_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI32_LOC]]
;
; CHECK: lbl2:
; CHECK: [[PHI1_V:%[A-z0-9]*]] = phi i1{{.*}} !dbg [[PHI1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i1 [[PHI1_V]], metadata [[PHI1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI1_LOC]]
; CHECK: [[SELECT_V:%[A-z0-9]*]] = select i1{{.*}} !dbg [[SELECT_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[SELECT_V]], metadata [[SELECT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT_LOC]]

define void @test_preraremat(i32* %src1, i32* %src2) !dbg !10 {
entry:
  %0 = alloca i32, align 4, !dbg !33
  call void @llvm.dbg.value(metadata i32* %0, metadata !13, metadata !DIExpression()), !dbg !33
  %1 = load i32, i32* %src1, !dbg !34
  call void @llvm.dbg.value(metadata i32 %1, metadata !15, metadata !DIExpression()), !dbg !34
  %2 = load i32, i32* %src2, !dbg !35
  call void @llvm.dbg.value(metadata i32 %2, metadata !17, metadata !DIExpression()), !dbg !35
  %3 = icmp eq i32 %1, %2, !dbg !36
  call void @llvm.dbg.value(metadata i1 %3, metadata !18, metadata !DIExpression()), !dbg !36
  %4 = icmp ugt i32 %1, %2, !dbg !37
  call void @llvm.dbg.value(metadata i1 %4, metadata !20, metadata !DIExpression()), !dbg !37
  %5 = and i1 %3, %4, !dbg !38
  call void @llvm.dbg.value(metadata i1 %5, metadata !21, metadata !DIExpression()), !dbg !38
  br i1 %5, label %lbl2, label %lbl1, !dbg !39

lbl1:                                             ; preds = %lbl2, %entry
  %6 = phi i32 [ %1, %entry ], [ %16, %lbl2 ], !dbg !40
  call void @llvm.dbg.value(metadata i32 %6, metadata !22, metadata !DIExpression()), !dbg !40
  %7 = inttoptr i32 %6 to i32*, !dbg !41
  call void @llvm.dbg.value(metadata i32* %7, metadata !23, metadata !DIExpression()), !dbg !41
  %8 = load i32, i32* %7, !dbg !42
  call void @llvm.dbg.value(metadata i32 %8, metadata !24, metadata !DIExpression()), !dbg !42
  %9 = icmp slt i32 %8, %6, !dbg !43
  call void @llvm.dbg.value(metadata i1 %9, metadata !25, metadata !DIExpression()), !dbg !43
  %10 = icmp ult i32 %8, %1, !dbg !44
  call void @llvm.dbg.value(metadata i1 %10, metadata !26, metadata !DIExpression()), !dbg !44
  %11 = and i1 %9, %5, !dbg !45
  call void @llvm.dbg.value(metadata i1 %11, metadata !27, metadata !DIExpression()), !dbg !45
  %12 = and i1 %10, %3, !dbg !46
  call void @llvm.dbg.value(metadata i1 %12, metadata !28, metadata !DIExpression()), !dbg !46
  %13 = or i1 %11, %12, !dbg !47
  call void @llvm.dbg.value(metadata i1 %13, metadata !29, metadata !DIExpression()), !dbg !47
  br i1 %13, label %lbl2, label %end, !dbg !48

lbl2:                                             ; preds = %lbl1, %entry
  %14 = phi i1 [ %4, %entry ], [ %13, %lbl1 ], !dbg !49
  call void @llvm.dbg.value(metadata i1 %14, metadata !30, metadata !DIExpression()), !dbg !49
  %15 = xor i1 %14, %5, !dbg !50
  call void @llvm.dbg.value(metadata i1 %15, metadata !31, metadata !DIExpression()), !dbg !50
  %16 = select i1 %3, i32 %1, i32 %2, !dbg !51
  call void @llvm.dbg.value(metadata i32 %16, metadata !32, metadata !DIExpression()), !dbg !51
  br i1 %15, label %lbl1, label %end, !dbg !52

end:                                              ; preds = %lbl2, %lbl1
  ret void, !dbg !53
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PreRARematFlag.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_preraremat", linkageName: "test_preraremat", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOC_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOC_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ICMPE_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[ICMPE_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ICMPGT_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[ICMPGT_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[AND1_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[AND1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR1_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI32_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[PHI32_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI1_MD]] = !DILocalVariable(name: "15", scope: [[SCOPE]], file: [[FILE]], line: 17
; CHECK-DAG: [[PHI1_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT_MD]] = !DILocalVariable(name: "17", scope: [[SCOPE]], file: [[FILE]], line: 19
; CHECK-DAG: [[SELECT_LOC]] = !DILocation(line: 19, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32*, i32*)* @test_preraremat, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "PreRARematFlag.ll", directory: "/")
!6 = !{}
!7 = !{i32 21}
!8 = !{i32 17}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_preraremat", linkageName: "test_preraremat", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17, !18, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !16)
!16 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !16)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !19)
!19 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 5, type: !19)
!21 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 6, type: !19)
!22 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 8, type: !16)
!23 = !DILocalVariable(name: "8", scope: !10, file: !5, line: 9, type: !14)
!24 = !DILocalVariable(name: "9", scope: !10, file: !5, line: 10, type: !16)
!25 = !DILocalVariable(name: "10", scope: !10, file: !5, line: 11, type: !19)
!26 = !DILocalVariable(name: "11", scope: !10, file: !5, line: 12, type: !19)
!27 = !DILocalVariable(name: "12", scope: !10, file: !5, line: 13, type: !19)
!28 = !DILocalVariable(name: "13", scope: !10, file: !5, line: 14, type: !19)
!29 = !DILocalVariable(name: "14", scope: !10, file: !5, line: 15, type: !19)
!30 = !DILocalVariable(name: "15", scope: !10, file: !5, line: 17, type: !19)
!31 = !DILocalVariable(name: "16", scope: !10, file: !5, line: 18, type: !19)
!32 = !DILocalVariable(name: "17", scope: !10, file: !5, line: 19, type: !16)
!33 = !DILocation(line: 1, column: 1, scope: !10)
!34 = !DILocation(line: 2, column: 1, scope: !10)
!35 = !DILocation(line: 3, column: 1, scope: !10)
!36 = !DILocation(line: 4, column: 1, scope: !10)
!37 = !DILocation(line: 5, column: 1, scope: !10)
!38 = !DILocation(line: 6, column: 1, scope: !10)
!39 = !DILocation(line: 7, column: 1, scope: !10)
!40 = !DILocation(line: 8, column: 1, scope: !10)
!41 = !DILocation(line: 9, column: 1, scope: !10)
!42 = !DILocation(line: 10, column: 1, scope: !10)
!43 = !DILocation(line: 11, column: 1, scope: !10)
!44 = !DILocation(line: 12, column: 1, scope: !10)
!45 = !DILocation(line: 13, column: 1, scope: !10)
!46 = !DILocation(line: 14, column: 1, scope: !10)
!47 = !DILocation(line: 15, column: 1, scope: !10)
!48 = !DILocation(line: 16, column: 1, scope: !10)
!49 = !DILocation(line: 17, column: 1, scope: !10)
!50 = !DILocation(line: 18, column: 1, scope: !10)
!51 = !DILocation(line: 19, column: 1, scope: !10)
!52 = !DILocation(line: 20, column: 1, scope: !10)
!53 = !DILocation(line: 21, column: 1, scope: !10)
