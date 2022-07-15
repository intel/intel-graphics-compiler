;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -GenOptLegalizer -S < %s | FileCheck %s
; ------------------------------------------------
; GenOptLegalizer
; ------------------------------------------------
; This test checks that GenOptLegalizer pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: store half [[VAL3_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata half [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: store half [[VAL7_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR2_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata half [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: store float [[VAL11_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR3_LOC:![0-9]*]]
; CHECK-DAG: store float [[VAL12_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR4_LOC:![0-9]*]]
; CHECK-DAG: store float [[VAL13_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR5_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL11_V]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC:![0-9]*]]
; CHECK-DAG: [[VAL11_V]] = {{.*}}, !dbg [[VAL11_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL12_V]], metadata [[VAL12_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL12_LOC:![0-9]*]]
; CHECK-DAG: [[VAL12_V]] = {{.*}}, !dbg [[VAL12_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL13_V]], metadata [[VAL13_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL13_LOC:![0-9]*]]
; CHECK-DAG: [[VAL13_V]] = {{.*}}, !dbg [[VAL13_LOC]]
; CHECK-DAG: store i8 [[VAL16_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR6_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 [[VAL16_V]], metadata [[VAL16_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL16_LOC:![0-9]*]]
; CHECK-DAG: [[VAL16_V]] = {{.*}}, !dbg [[VAL16_LOC]]
; CHECK-DAG: store i8 [[VAL19_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR7_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 [[VAL19_V]], metadata [[VAL19_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL19_LOC:![0-9]*]]
; CHECK-DAG: [[VAL19_V]] = {{.*}}, !dbg [[VAL19_LOC]]


define void @test(<3 x half> %src1, half* %dst1, <4 x float> %src2, float* %dst2, i8* %dst3) !dbg !6 {
  %1 = bitcast <3 x half> %src1 to i48, !dbg !34
  call void @llvm.dbg.value(metadata i48 %1, metadata !9, metadata !DIExpression()), !dbg !34
  %2 = trunc i48 %1 to i16, !dbg !35
  call void @llvm.dbg.value(metadata i16 %2, metadata !11, metadata !DIExpression()), !dbg !35
  %3 = bitcast i16 %2 to half, !dbg !36
  call void @llvm.dbg.value(metadata half %3, metadata !13, metadata !DIExpression()), !dbg !36
  store half %3, half* %dst1, !dbg !37
  %4 = bitcast <3 x half> %src1 to i48, !dbg !38
  call void @llvm.dbg.value(metadata i48 %4, metadata !14, metadata !DIExpression()), !dbg !38
  %5 = lshr i48 %4, 16, !dbg !39
  call void @llvm.dbg.value(metadata i48 %5, metadata !15, metadata !DIExpression()), !dbg !39
  %6 = trunc i48 %5 to i16, !dbg !40
  call void @llvm.dbg.value(metadata i16 %6, metadata !16, metadata !DIExpression()), !dbg !40
  %7 = bitcast i16 %6 to half, !dbg !41
  call void @llvm.dbg.value(metadata half %7, metadata !17, metadata !DIExpression()), !dbg !41
  store half %7, half* %dst1, !dbg !42
  %8 = bitcast <4 x float> %src2 to i128, !dbg !43
  call void @llvm.dbg.value(metadata i128 %8, metadata !18, metadata !DIExpression()), !dbg !43
  %9 = trunc i128 %8 to i96, !dbg !44
  call void @llvm.dbg.value(metadata i96 %9, metadata !20, metadata !DIExpression()), !dbg !44
  %10 = bitcast i96 %9 to <3 x float>, !dbg !45
  call void @llvm.dbg.value(metadata <3 x float> %10, metadata !22, metadata !DIExpression()), !dbg !45
  %11 = extractelement <3 x float> %10, i32 0, !dbg !46
  call void @llvm.dbg.value(metadata float %11, metadata !23, metadata !DIExpression()), !dbg !46
  %12 = extractelement <3 x float> %10, i32 1, !dbg !47
  call void @llvm.dbg.value(metadata float %12, metadata !25, metadata !DIExpression()), !dbg !47
  %13 = extractelement <3 x float> %10, i32 2, !dbg !48
  call void @llvm.dbg.value(metadata float %13, metadata !26, metadata !DIExpression()), !dbg !48
  store float %11, float* %dst2, !dbg !49
  store float %12, float* %dst2, !dbg !50
  store float %13, float* %dst2, !dbg !51
  %14 = bitcast <4 x float> %src2 to <4 x i32>, !dbg !52
  call void @llvm.dbg.value(metadata <4 x i32> %14, metadata !27, metadata !DIExpression()), !dbg !52
  %15 = bitcast <4 x i32> %14 to i128, !dbg !53
  call void @llvm.dbg.value(metadata i128 %15, metadata !28, metadata !DIExpression()), !dbg !53
  %16 = trunc i128 %15 to i8, !dbg !54
  call void @llvm.dbg.value(metadata i8 %16, metadata !29, metadata !DIExpression()), !dbg !54
  store i8 %16, i8* %dst3, !dbg !55
  %17 = bitcast <4 x i32> %14 to i128, !dbg !56
  call void @llvm.dbg.value(metadata i128 %17, metadata !31, metadata !DIExpression()), !dbg !56
  %18 = lshr i128 %17, 8, !dbg !57
  call void @llvm.dbg.value(metadata i128 %18, metadata !32, metadata !DIExpression()), !dbg !57
  %19 = trunc i128 %18 to i8, !dbg !58
  call void @llvm.dbg.value(metadata i8 %19, metadata !33, metadata !DIExpression()), !dbg !58
  store i8 %19, i8* %dst3, !dbg !59
  ret void, !dbg !60
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "bitcast.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL12_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL12_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL13_MD]] = !DILocalVariable(name: "13", scope: [[SCOPE]], file: [[FILE]], line: 15
; CHECK-DAG: [[VAL13_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR4_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR5_LOC]] = !DILocation(line: 18, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL16_MD]] = !DILocalVariable(name: "16", scope: [[SCOPE]], file: [[FILE]], line: 21
; CHECK-DAG: [[VAL16_LOC]] = !DILocation(line: 21, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR6_LOC]] = !DILocation(line: 22, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL19_MD]] = !DILocalVariable(name: "19", scope: [[SCOPE]], file: [[FILE]], line: 25
; CHECK-DAG: [[VAL19_LOC]] = !DILocation(line: 25, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR7_LOC]] = !DILocation(line: 26, column: 1, scope: [[SCOPE]])



; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "bitcast.ll", directory: "/")
!2 = !{}
!3 = !{i32 27}
!4 = !{i32 19}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16, !17, !18, !20, !22, !23, !25, !26, !27, !28, !29, !31, !32, !33}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !12)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !12)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 10, type: !19)
!19 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 11, type: !21)
!21 = !DIBasicType(name: "ty96", size: 96, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 12, type: !19)
!23 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 13, type: !24)
!24 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!25 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 14, type: !24)
!26 = !DILocalVariable(name: "13", scope: !6, file: !1, line: 15, type: !24)
!27 = !DILocalVariable(name: "14", scope: !6, file: !1, line: 19, type: !19)
!28 = !DILocalVariable(name: "15", scope: !6, file: !1, line: 20, type: !19)
!29 = !DILocalVariable(name: "16", scope: !6, file: !1, line: 21, type: !30)
!30 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!31 = !DILocalVariable(name: "17", scope: !6, file: !1, line: 23, type: !19)
!32 = !DILocalVariable(name: "18", scope: !6, file: !1, line: 24, type: !19)
!33 = !DILocalVariable(name: "19", scope: !6, file: !1, line: 25, type: !30)
!34 = !DILocation(line: 1, column: 1, scope: !6)
!35 = !DILocation(line: 2, column: 1, scope: !6)
!36 = !DILocation(line: 3, column: 1, scope: !6)
!37 = !DILocation(line: 4, column: 1, scope: !6)
!38 = !DILocation(line: 5, column: 1, scope: !6)
!39 = !DILocation(line: 6, column: 1, scope: !6)
!40 = !DILocation(line: 7, column: 1, scope: !6)
!41 = !DILocation(line: 8, column: 1, scope: !6)
!42 = !DILocation(line: 9, column: 1, scope: !6)
!43 = !DILocation(line: 10, column: 1, scope: !6)
!44 = !DILocation(line: 11, column: 1, scope: !6)
!45 = !DILocation(line: 12, column: 1, scope: !6)
!46 = !DILocation(line: 13, column: 1, scope: !6)
!47 = !DILocation(line: 14, column: 1, scope: !6)
!48 = !DILocation(line: 15, column: 1, scope: !6)
!49 = !DILocation(line: 16, column: 1, scope: !6)
!50 = !DILocation(line: 17, column: 1, scope: !6)
!51 = !DILocation(line: 18, column: 1, scope: !6)
!52 = !DILocation(line: 19, column: 1, scope: !6)
!53 = !DILocation(line: 20, column: 1, scope: !6)
!54 = !DILocation(line: 21, column: 1, scope: !6)
!55 = !DILocation(line: 22, column: 1, scope: !6)
!56 = !DILocation(line: 23, column: 1, scope: !6)
!57 = !DILocation(line: 24, column: 1, scope: !6)
!58 = !DILocation(line: 25, column: 1, scope: !6)
!59 = !DILocation(line: 26, column: 1, scope: !6)
!60 = !DILocation(line: 27, column: 1, scope: !6)
