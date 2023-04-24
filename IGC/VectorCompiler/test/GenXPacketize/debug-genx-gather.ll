;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPacketize
; ------------------------------------------------
; This test checks that GenXPacketize pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Entry part:

define void @test_packetize(i64 %a, float %b) !dbg !6 {
  %1 = insertelement <8 x i64> zeroinitializer, i64 %a, i64 1, !dbg !13
  call void @llvm.dbg.value(metadata <8 x i64> %1, metadata !9, metadata !DIExpression()), !dbg !13
  %2 = insertelement <8 x float> zeroinitializer, float %b, i64 2, !dbg !14
  call void @llvm.dbg.value(metadata <8 x float> %2, metadata !11, metadata !DIExpression()), !dbg !14
  call void @test_vectorize(<8 x i64> %1, <8 x float> %2), !dbg !15
  ret void, !dbg !16
}

; Modified part:
; Values are not salavageble

; CHECK: define internal void @test_vectorize{{.*}} !dbg [[SCOPE:![0-9]*]]
; extr is opt out
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; extr is opt out
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; extr is opt out
; CHECK: [[VAL6_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: [[VAL7_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: [[VAL8_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: call {{.*}}scatter4{{.*}}, !dbg [[SCATTER1_LOC:![0-9]*]]
; CHECK: [[VAL9_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: call {{.*}}scatter{{.*}}, !dbg [[SCATTER2_LOC:![0-9]*]]

define internal void @test_vectorize(<8 x i64> %a, <8 x float> %b) #0 !dbg !17 {
entry:
  %0 = extractelement <8 x i64> %a, i64 1, !dbg !30
  call void @llvm.dbg.value(metadata i64 %0, metadata !19, metadata !DIExpression()), !dbg !30
  %1 = bitcast i64 %0 to <1 x i64>, !dbg !31
  call void @llvm.dbg.value(metadata <1 x i64> %1, metadata !21, metadata !DIExpression()), !dbg !31
  %2 = extractelement <8 x float> %b, i64 2, !dbg !32
  call void @llvm.dbg.value(metadata float %2, metadata !22, metadata !DIExpression()), !dbg !32
  %3 = bitcast float %2 to <1 x float>, !dbg !33
  call void @llvm.dbg.value(metadata <1 x float> %3, metadata !24, metadata !DIExpression()), !dbg !33
  %4 = inttoptr i64 %0 to float*, !dbg !34
  call void @llvm.dbg.value(metadata float* %4, metadata !25, metadata !DIExpression()), !dbg !34
  %5 = bitcast float* %4 to <1 x float*>, !dbg !35
  call void @llvm.dbg.value(metadata <1 x float*> %5, metadata !26, metadata !DIExpression()), !dbg !35
  %6 = call <1 x float> @llvm.genx.svm.gather4.scaled.v1f32.v1i64.v1i1(<1 x i1> zeroinitializer, i32 4, i16 0, i64 0, <1 x i64> %1, <1 x float> %3), !dbg !36
  call void @llvm.dbg.value(metadata <1 x float> %6, metadata !27, metadata !DIExpression()), !dbg !36
  %7 = call <1 x float> @llvm.genx.gather4.typed.v1f32.v1i32.v1i1(i32 1, <1 x i1> zeroinitializer, i32 4, <1 x i32> <i32 13>, <1 x i32> <i32 14>, <1 x i32> <i32 15>, <1 x float> %3), !dbg !37
  call void @llvm.dbg.value(metadata <1 x float> %7, metadata !28, metadata !DIExpression()), !dbg !37
  call void @llvm.genx.scatter4.typed.v1f32.v1i32.v1i1(i32 1, <1 x i1> zeroinitializer, i32 4, <1 x i32> <i32 13>, <1 x i32> <i32 14>, <1 x i32> <i32 15>, <1 x float> %3), !dbg !38
  %8 = call <1 x float> @llvm.genx.gather.scaled.v1f32.v1i32.v1i1(<1 x i1> zeroinitializer, i32 0, i16 0, i32 0, i32 0, <1 x i32> <i32 16>, <1 x float> %3), !dbg !39
  call void @llvm.dbg.value(metadata <1 x float> %8, metadata !29, metadata !DIExpression()), !dbg !39
  call void @llvm.genx.scatter.scaled.v1f32.v1i32.v1i1(<1 x i1> zeroinitializer, i32 0, i16 0, i32 0, i32 0, <1 x i32> <i32 16>, <1 x float> %3), !dbg !40
  ret void, !dbg !41
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPacketize.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SCATTER1_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SCATTER2_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])

declare <1 x float> @llvm.genx.svm.gather4.scaled.v1f32.v1i64.v1i1(<1 x i1>, i32, i16, i64, <1 x i64>, <1 x float>)

declare <1 x float> @llvm.genx.gather4.typed.v1f32.v1i32.v1i1(i32, <1 x i1>, i32, <1 x i32>, <1 x i32>, <1 x i32>, <1 x float>)

declare void @llvm.genx.scatter4.typed.v1f32.v1i32.v1i1(i32, <1 x i1>, i32, <1 x i32>, <1 x i32>, <1 x i32>, <1 x float>)

declare <1 x float> @llvm.genx.gather.scaled.v1f32.v1i32.v1i1(<1 x i1>, i32, i16, i32, i32, <1 x i32>, <1 x float>)

declare void @llvm.genx.scatter.scaled.v1f32.v1i32.v1i1(<1 x i1>, i32, i16, i32, i32, <1 x i32>, <1 x float>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxSIMT"="8" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXPacketize.ll", directory: "/")
!2 = !{}
!3 = !{i32 16}
!4 = !{i32 11}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: !1, line: 5, type: !7, scopeLine: 5, unit: !0, retainedNodes: !18)
!18 = !{!19, !21, !22, !24, !25, !26, !27, !28, !29}
!19 = !DILocalVariable(name: "3", scope: !17, file: !1, line: 5, type: !20)
!20 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "4", scope: !17, file: !1, line: 6, type: !20)
!22 = !DILocalVariable(name: "5", scope: !17, file: !1, line: 7, type: !23)
!23 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "6", scope: !17, file: !1, line: 8, type: !23)
!25 = !DILocalVariable(name: "7", scope: !17, file: !1, line: 9, type: !20)
!26 = !DILocalVariable(name: "8", scope: !17, file: !1, line: 10, type: !20)
!27 = !DILocalVariable(name: "9", scope: !17, file: !1, line: 11, type: !23)
!28 = !DILocalVariable(name: "10", scope: !17, file: !1, line: 12, type: !23)
!29 = !DILocalVariable(name: "11", scope: !17, file: !1, line: 14, type: !23)
!30 = !DILocation(line: 5, column: 1, scope: !17)
!31 = !DILocation(line: 6, column: 1, scope: !17)
!32 = !DILocation(line: 7, column: 1, scope: !17)
!33 = !DILocation(line: 8, column: 1, scope: !17)
!34 = !DILocation(line: 9, column: 1, scope: !17)
!35 = !DILocation(line: 10, column: 1, scope: !17)
!36 = !DILocation(line: 11, column: 1, scope: !17)
!37 = !DILocation(line: 12, column: 1, scope: !17)
!38 = !DILocation(line: 13, column: 1, scope: !17)
!39 = !DILocation(line: 14, column: 1, scope: !17)
!40 = !DILocation(line: 15, column: 1, scope: !17)
!41 = !DILocation(line: 16, column: 1, scope: !17)
