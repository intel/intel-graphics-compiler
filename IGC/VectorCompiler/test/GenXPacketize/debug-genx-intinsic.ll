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

define void @test_packetize(i32 %a, float %b) !dbg !6 {
  %1 = insertelement <8 x i32> zeroinitializer, i32 %a, i32 1, !dbg !12
  call void @llvm.dbg.value(metadata <8 x i32> %1, metadata !9, metadata !DIExpression()), !dbg !12
  %2 = insertelement <8 x float> zeroinitializer, float %b, i32 2, !dbg !13
  call void @llvm.dbg.value(metadata <8 x float> %2, metadata !11, metadata !DIExpression()), !dbg !13
  call void @test_vectorize(<8 x i32> %1, <8 x float> %2), !dbg !14
  ret void, !dbg !15
}

; Modified part:
; Values are not salavageble
; Note: pointers prbly could be salvaged though, for now check lineinfo

; CHECK: define internal void @test_vectorize{{.*}} !dbg [[SCOPE:![0-9]*]]
; extr is opt out
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; extr is opt out
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: [[VAL6_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: [[VAL7_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: [[VAL8_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: [[VAL9_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: [[VAL10_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK: call {{.*}}scatter{{.*}}, !dbg [[SCATTER1_LOC:![0-9]*]]
; CHECK: [[VAL11_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL11_LOC:![0-9]*]]

define internal void @test_vectorize(<8 x i32> %a, <8 x float> %b) #0 !dbg !16 {
entry:
  %0 = extractelement <8 x i32> %a, i32 1, !dbg !32
  call void @llvm.dbg.value(metadata i32 %0, metadata !18, metadata !DIExpression()), !dbg !32
  %1 = bitcast i32 %0 to <1 x i32>, !dbg !33
  call void @llvm.dbg.value(metadata <1 x i32> %1, metadata !20, metadata !DIExpression()), !dbg !33
  %2 = extractelement <8 x float> %b, i32 2, !dbg !34
  call void @llvm.dbg.value(metadata float %2, metadata !21, metadata !DIExpression()), !dbg !34
  %3 = bitcast float %2 to <1 x float>, !dbg !35
  call void @llvm.dbg.value(metadata <1 x float> %3, metadata !22, metadata !DIExpression()), !dbg !35
  %4 = call <1 x float> @llvm.genx.dword.atomic.fadd.v1f32.v1i1.v1i32(<1 x i1> zeroinitializer, i32 0, <1 x i32> zeroinitializer, <1 x float> %3, <1 x float> %3), !dbg !36
  call void @llvm.dbg.value(metadata <1 x float> %4, metadata !23, metadata !DIExpression()), !dbg !36
  %5 = call <1 x i32> @llvm.genx.dword.atomic.inc.v1i32.v1i1.v1i32(<1 x i1> zeroinitializer, i32 0, <1 x i32> %1, <1 x i32> %1), !dbg !37
  call void @llvm.dbg.value(metadata <1 x i32> %5, metadata !24, metadata !DIExpression()), !dbg !37
  %6 = call <1 x float> @llvm.genx.dword.atomic.fcmpwr.v1f32.v1i1.v1i32(<1 x i1> zeroinitializer, i32 0, <1 x i32> zeroinitializer, <1 x float> %3, <1 x float> %3, <1 x float> %3), !dbg !38
  call void @llvm.dbg.value(metadata <1 x float> %6, metadata !25, metadata !DIExpression()), !dbg !38
  %7 = call <1 x i32> @llvm.genx.dword.atomic.cmpxchg.v1i32.v1i1.v1i32(<1 x i1> zeroinitializer, i32 0, <1 x i32> %1, <1 x i32> %1, <1 x i32> %1, <1 x i32> %1), !dbg !39
  call void @llvm.dbg.value(metadata <1 x i32> %7, metadata !26, metadata !DIExpression()), !dbg !39
  %8 = call i32 @llvm.genx.bfn.i32(i32 %0, i32 %0, i32 %0, i8 13), !dbg !40
  call void @llvm.dbg.value(metadata i32 %8, metadata !27, metadata !DIExpression()), !dbg !40
  %9 = inttoptr i32 %0 to float*, !dbg !41
  call void @llvm.dbg.value(metadata float* %9, metadata !28, metadata !DIExpression()), !dbg !41
  %10 = bitcast float* %9 to <1 x float*>, !dbg !42
  call void @llvm.dbg.value(metadata <1 x float*> %10, metadata !30, metadata !DIExpression()), !dbg !42
  call void @llvm.genx.svm.scatter.v1i1.v1p0f32.v1f32(<1 x i1> zeroinitializer, i32 4, <1 x float*> %10, <1 x float> %3), !dbg !43
  %11 = call <1 x float> @llvm.genx.svm.gather.v1f32.v1i1.v1p0f32(<1 x i1> zeroinitializer, i32 4, <1 x float*> %10, <1 x float> %3), !dbg !44
  call void @llvm.dbg.value(metadata <1 x float> %11, metadata !31, metadata !DIExpression()), !dbg !44
  ret void, !dbg !45
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPacketize.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SCATTER1_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])

declare <1 x float> @llvm.genx.dword.atomic.fadd.v1f32.v1i1.v1i32(<1 x i1>, i32, <1 x i32>, <1 x float>, <1 x float>)

declare <1 x float> @llvm.genx.dword.atomic.fcmpwr.v1f32.v1i1.v1i32(<1 x i1>, i32, <1 x i32>, <1 x float>, <1 x float>, <1 x float>)

declare <1 x i32> @llvm.genx.dword.atomic.inc.v1i32.v1i1.v1i32(<1 x i1>, i32, <1 x i32>, <1 x i32>)

declare <1 x i32> @llvm.genx.dword.atomic.cmpxchg.v1i32.v1i1.v1i32(<1 x i1>, i32, <1 x i32>, <1 x i32>, <1 x i32>, <1 x i32>)

declare i32 @llvm.genx.bfn.i32(i32, i32, i32, i8)

declare <1 x float> @llvm.genx.svm.gather.v1f32.v1i1.v1p0f32(<1 x i1>, i32, <1 x float*>, <1 x float>)

declare void @llvm.genx.svm.scatter.v1i1.v1p0f32.v1f32(<1 x i1>, i32, <1 x float*>, <1 x float>)

declare float @llvm.genx.sqrt.f32(float)

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
!3 = !{i32 18}
!4 = !{i32 14}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocation(line: 1, column: 1, scope: !6)
!13 = !DILocation(line: 2, column: 1, scope: !6)
!14 = !DILocation(line: 3, column: 1, scope: !6)
!15 = !DILocation(line: 4, column: 1, scope: !6)
!16 = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: !1, line: 5, type: !7, scopeLine: 5, unit: !0, retainedNodes: !17)
!17 = !{!18, !20, !21, !22, !23, !24, !25, !26, !27, !28, !30, !31}
!18 = !DILocalVariable(name: "3", scope: !16, file: !1, line: 5, type: !19)
!19 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "4", scope: !16, file: !1, line: 6, type: !19)
!21 = !DILocalVariable(name: "5", scope: !16, file: !1, line: 7, type: !19)
!22 = !DILocalVariable(name: "6", scope: !16, file: !1, line: 8, type: !19)
!23 = !DILocalVariable(name: "7", scope: !16, file: !1, line: 9, type: !19)
!24 = !DILocalVariable(name: "8", scope: !16, file: !1, line: 10, type: !19)
!25 = !DILocalVariable(name: "9", scope: !16, file: !1, line: 11, type: !19)
!26 = !DILocalVariable(name: "10", scope: !16, file: !1, line: 12, type: !19)
!27 = !DILocalVariable(name: "11", scope: !16, file: !1, line: 13, type: !19)
!28 = !DILocalVariable(name: "12", scope: !16, file: !1, line: 14, type: !29)
!29 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!30 = !DILocalVariable(name: "13", scope: !16, file: !1, line: 15, type: !29)
!31 = !DILocalVariable(name: "14", scope: !16, file: !1, line: 17, type: !19)
!32 = !DILocation(line: 5, column: 1, scope: !16)
!33 = !DILocation(line: 6, column: 1, scope: !16)
!34 = !DILocation(line: 7, column: 1, scope: !16)
!35 = !DILocation(line: 8, column: 1, scope: !16)
!36 = !DILocation(line: 9, column: 1, scope: !16)
!37 = !DILocation(line: 10, column: 1, scope: !16)
!38 = !DILocation(line: 11, column: 1, scope: !16)
!39 = !DILocation(line: 12, column: 1, scope: !16)
!40 = !DILocation(line: 13, column: 1, scope: !16)
!41 = !DILocation(line: 14, column: 1, scope: !16)
!42 = !DILocation(line: 15, column: 1, scope: !16)
!43 = !DILocation(line: 16, column: 1, scope: !16)
!44 = !DILocation(line: 17, column: 1, scope: !16)
!45 = !DILocation(line: 18, column: 1, scope: !16)
