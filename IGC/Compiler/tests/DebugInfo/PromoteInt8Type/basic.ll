;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-promoteint8type  -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : instructions
; ------------------------------------------------
; This test checks that PromoteInt8Type pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;

; CHECK: @test_promote{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[ADD_V:%[A-z0-9.]*]] = add {{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: [[SDIV_V:%[A-z0-9.]*]] = sdiv {{.*}} !dbg [[SDIV_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[SDIV_V]], metadata [[SDIV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SDIV_LOC]]
; CHECK: [[UDIV_V:%[A-z0-9.]*]] = udiv {{.*}} !dbg [[UDIV_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[UDIV_V]], metadata [[UDIV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UDIV_LOC]]
; CHECK: [[ASHR_V:%[A-z0-9.]*]] = ashr {{.*}} !dbg [[ASHR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[ASHR_V]], metadata [[ASHR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ASHR_LOC]]
; CHECK: [[LSHR_V:%[A-z0-9.]*]] = lshr {{.*}} !dbg [[LSHR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[LSHR_V]], metadata [[LSHR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LSHR_LOC]]
; CHECK: [[SHL_V:%[A-z0-9.]*]] = shl {{.*}} !dbg [[SHL_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[SHL_V]], metadata [[SHL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHL_LOC]]
; CHECK: [[TRUNC_V:%[A-z0-9.]*]] = trunc {{.*}} to i1, !dbg [[TRUNC_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i1 [[TRUNC_V]], metadata [[TRUNC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TRUNC_LOC]]
; CHECK: [[ZEXT_V:%[A-z0-9.]*]] = select {{.*}} !dbg [[ZEXT_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[ZEXT_V]], metadata [[ZEXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ZEXT_LOC]]
; CHECK: [[FTOUI_V:%[A-z0-9.]*]] = fptoui {{.*}} !dbg [[FTOUI_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[FTOUI_V]], metadata [[FTOUI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FTOUI_LOC]]
; CHECK: [[SELECT_V:%[A-z0-9.]*]] = select {{.*}} !dbg [[SELECT_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[SELECT_V]], metadata [[SELECT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT_LOC]]
; CHECK: [[LOAD_V:%[A-z0-9.]*]] = load {{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[LOAD_V]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: [[CALL_V:%[A-z0-9.]*]] = call {{.*}} !dbg [[CALL_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[CALL_V]], metadata [[CALL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL_LOC]]
; CHECK: [[INSRT_V:%[A-z0-9.]*]] = insertelement {{.*}} !dbg [[INSRT_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <2 x i8> [[INSRT_V]], metadata [[INSRT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[INSRT_LOC]]
; CHECK: [[EXTR_V:%[A-z0-9.]*]] = extractelement {{.*}} !dbg [[EXTR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[EXTR_V]], metadata [[EXTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR_LOC]]
; CHECK: [[ICMP_V:%[A-z0-9.]*]] = icmp {{.*}} !dbg [[ICMP_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i1 [[ICMP_V]], metadata [[ICMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMP_LOC]]
; CHECK: [[UITOF_V:%[A-z0-9.]*]] = uitofp {{.*}} !dbg [[UITOF_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float [[UITOF_V]], metadata [[UITOF_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UITOF_LOC]]
; CHECK: [[SEXT_V:%[A-z0-9.]*]] = sext {{.*}} !dbg [[SEXT_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{i(8|16)}} [[SEXT_V]], metadata [[SEXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SEXT_LOC]]

define void @test_promote(i8 %src1, i8 %src2, float %src3, i8* %dst) !dbg !6 {
entry:
  %0 = add i8 %src1, %src2, !dbg !33
  call void @llvm.dbg.value(metadata i8 %0, metadata !9, metadata !DIExpression()), !dbg !33
  %1 = sdiv i8 %src1, %0, !dbg !34
  call void @llvm.dbg.value(metadata i8 %1, metadata !11, metadata !DIExpression()), !dbg !34
  %2 = udiv i8 %src2, %1, !dbg !35
  call void @llvm.dbg.value(metadata i8 %2, metadata !12, metadata !DIExpression()), !dbg !35
  %3 = ashr i8 %2, 2, !dbg !36
  call void @llvm.dbg.value(metadata i8 %3, metadata !13, metadata !DIExpression()), !dbg !36
  %4 = lshr i8 %2, 3, !dbg !37
  call void @llvm.dbg.value(metadata i8 %4, metadata !14, metadata !DIExpression()), !dbg !37
  %5 = shl i8 %4, %3, !dbg !38
  call void @llvm.dbg.value(metadata i8 %5, metadata !15, metadata !DIExpression()), !dbg !38
  %6 = trunc i8 %5 to i1, !dbg !39
  call void @llvm.dbg.value(metadata i1 %6, metadata !16, metadata !DIExpression()), !dbg !39
  %7 = zext i1 %6 to i8, !dbg !40
  call void @llvm.dbg.value(metadata i8 %7, metadata !17, metadata !DIExpression()), !dbg !40
  %8 = fptoui float %src3 to i8, !dbg !41
  call void @llvm.dbg.value(metadata i8 %8, metadata !18, metadata !DIExpression()), !dbg !41
  %9 = select i1 %6, i8 %8, i8 %7, !dbg !42
  call void @llvm.dbg.value(metadata i8 %9, metadata !19, metadata !DIExpression()), !dbg !42
  %10 = load i8, i8* %dst, !dbg !43
  call void @llvm.dbg.value(metadata i8 %10, metadata !20, metadata !DIExpression()), !dbg !43
  %11 = call i8 @foo(i8 %10, i8 %9), !dbg !44
  call void @llvm.dbg.value(metadata i8 %11, metadata !21, metadata !DIExpression()), !dbg !44
  %12 = insertelement <2 x i8> <i8 2, i8 1>, i8 %11, i32 0, !dbg !45
  call void @llvm.dbg.value(metadata <2 x i8> %12, metadata !22, metadata !DIExpression()), !dbg !45
  %13 = insertelement <2 x i8> %12, i8 %8, i32 1, !dbg !46
  call void @llvm.dbg.value(metadata <2 x i8> %13, metadata !24, metadata !DIExpression()), !dbg !46
  %14 = extractelement <2 x i8> %13, i32 1, !dbg !47
  call void @llvm.dbg.value(metadata i8 %14, metadata !25, metadata !DIExpression()), !dbg !47
  %15 = extractelement <2 x i8> %13, i32 0, !dbg !48
  call void @llvm.dbg.value(metadata i8 %15, metadata !26, metadata !DIExpression()), !dbg !48
  %16 = icmp ugt i8 %14, %15, !dbg !49
  call void @llvm.dbg.value(metadata i1 %16, metadata !27, metadata !DIExpression()), !dbg !49
  %17 = uitofp i8 %15 to float, !dbg !50
  call void @llvm.dbg.value(metadata float %17, metadata !28, metadata !DIExpression()), !dbg !50
  %18 = alloca float, align 4, !dbg !51
  call void @llvm.dbg.value(metadata float* %18, metadata !30, metadata !DIExpression()), !dbg !51
  store float %17, float* %18, !dbg !52
  %19 = sext i1 %16 to i8, !dbg !53
  call void @llvm.dbg.value(metadata i8 %19, metadata !32, metadata !DIExpression()), !dbg !53
  store i8 %19, i8* %dst, !dbg !54
  ret void, !dbg !55
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SDIV_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[SDIV_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[UDIV_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[UDIV_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ASHR_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[ASHR_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LSHR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LSHR_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SHL_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[SHL_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[TRUNC_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[TRUNC_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ZEXT_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[ZEXT_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FTOUI_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[FTOUI_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[SELECT_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[INSRT_MD]] = !DILocalVariable(name: "13", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[INSRT_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR_MD]] = !DILocalVariable(name: "15", scope: [[SCOPE]], file: [[FILE]], line: 15
; CHECK-DAG: [[EXTR_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ICMP_MD]] = !DILocalVariable(name: "17", scope: [[SCOPE]], file: [[FILE]], line: 17
; CHECK-DAG: [[ICMP_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[UITOF_MD]] = !DILocalVariable(name: "18", scope: [[SCOPE]], file: [[FILE]], line: 18
; CHECK-DAG: [[UITOF_LOC]] = !DILocation(line: 18, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SEXT_MD]] = !DILocalVariable(name: "20", scope: [[SCOPE]], file: [[FILE]], line: 21
; CHECK-DAG: [[SEXT_LOC]] = !DILocation(line: 21, column: 1, scope: [[SCOPE]])

declare i8 @foo(i8, i8)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "basic.ll", directory: "/")
!2 = !{}
!3 = !{i32 23}
!4 = !{i32 20}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !24, !25, !26, !27, !28, !30, !32}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!17 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!18 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !10)
!19 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !10)
!20 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !10)
!21 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !10)
!22 = !DILocalVariable(name: "13", scope: !6, file: !1, line: 13, type: !23)
!23 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "14", scope: !6, file: !1, line: 14, type: !23)
!25 = !DILocalVariable(name: "15", scope: !6, file: !1, line: 15, type: !10)
!26 = !DILocalVariable(name: "16", scope: !6, file: !1, line: 16, type: !10)
!27 = !DILocalVariable(name: "17", scope: !6, file: !1, line: 17, type: !10)
!28 = !DILocalVariable(name: "18", scope: !6, file: !1, line: 18, type: !29)
!29 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!30 = !DILocalVariable(name: "19", scope: !6, file: !1, line: 19, type: !31)
!31 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!32 = !DILocalVariable(name: "20", scope: !6, file: !1, line: 21, type: !10)
!33 = !DILocation(line: 1, column: 1, scope: !6)
!34 = !DILocation(line: 2, column: 1, scope: !6)
!35 = !DILocation(line: 3, column: 1, scope: !6)
!36 = !DILocation(line: 4, column: 1, scope: !6)
!37 = !DILocation(line: 5, column: 1, scope: !6)
!38 = !DILocation(line: 6, column: 1, scope: !6)
!39 = !DILocation(line: 7, column: 1, scope: !6)
!40 = !DILocation(line: 8, column: 1, scope: !6)
!41 = !DILocation(line: 9, column: 1, scope: !6)
!42 = !DILocation(line: 10, column: 1, scope: !6)
!43 = !DILocation(line: 11, column: 1, scope: !6)
!44 = !DILocation(line: 12, column: 1, scope: !6)
!45 = !DILocation(line: 13, column: 1, scope: !6)
!46 = !DILocation(line: 14, column: 1, scope: !6)
!47 = !DILocation(line: 15, column: 1, scope: !6)
!48 = !DILocation(line: 16, column: 1, scope: !6)
!49 = !DILocation(line: 17, column: 1, scope: !6)
!50 = !DILocation(line: 18, column: 1, scope: !6)
!51 = !DILocation(line: 19, column: 1, scope: !6)
!52 = !DILocation(line: 20, column: 1, scope: !6)
!53 = !DILocation(line: 21, column: 1, scope: !6)
!54 = !DILocation(line: 22, column: 1, scope: !6)
!55 = !DILocation(line: 23, column: 1, scope: !6)
