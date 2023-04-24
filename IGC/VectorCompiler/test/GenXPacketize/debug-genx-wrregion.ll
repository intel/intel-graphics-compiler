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

define void @test_packetize() !dbg !6 {
  call void @test_vectorize(), !dbg !8
  ret void, !dbg !9
}

; Modified part:
; CHECK: define internal void @test_vectorize{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <1 x i32> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]

declare void @test_use(<1 x i32>)

define internal void @test_vectorize() #0 !dbg !10 {
entry:
  %0 = call i32 @llvm.genx.lane.id(), !dbg !16
  call void @llvm.dbg.value(metadata i32 %0, metadata !12, metadata !DIExpression()), !dbg !16
  %1 = bitcast i32 %0 to <1 x i32>, !dbg !17
  call void @llvm.dbg.value(metadata <1 x i32> %1, metadata !14, metadata !DIExpression()), !dbg !17
  %2 = call <1 x i32> @llvm.genx.wrregioni.v1i32.v1i32.v1i32.i16(<1 x i32> %1, <1 x i32> %1, i32 1, i32 1, i32 0, i16 0, i32 0, i1 false), !dbg !18
  call void @llvm.dbg.value(metadata <1 x i32> %2, metadata !15, metadata !DIExpression()), !dbg !18
  call void @llvm.genx.scatter.scaled.v1i32.v1i32.v1i1(<1 x i1> zeroinitializer, i32 0, i16 0, i32 0, i32 0, <1 x i32> <i32 16>, <1 x i32> %2)
  ret void, !dbg !19
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPacketize.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

declare i32 @llvm.genx.lane.id()

declare <1 x i32> @llvm.genx.rdregioni.1vi32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)

declare <1 x i32> @llvm.genx.wrregioni.v1i32.v1i32.v1i32.i16(<1 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)

declare void @llvm.genx.scatter.scaled.v1i32.v1i32.v1i1(<1 x i1>, i32, i16, i32, i32, <1 x i32>, <1 x i32>)

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
!3 = !{i32 6}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !2)
!7 = !DISubroutineType(types: !2)
!8 = !DILocation(line: 1, column: 1, scope: !6)
!9 = !DILocation(line: 2, column: 1, scope: !6)
!10 = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: !1, line: 3, type: !7, scopeLine: 3, unit: !0, retainedNodes: !11)
!11 = !{!12, !14, !15}
!12 = !DILocalVariable(name: "1", scope: !10, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !10, file: !1, line: 4, type: !13)
!15 = !DILocalVariable(name: "3", scope: !10, file: !1, line: 5, type: !13)
!16 = !DILocation(line: 3, column: 1, scope: !10)
!17 = !DILocation(line: 4, column: 1, scope: !10)
!18 = !DILocation(line: 5, column: 1, scope: !10)
!19 = !DILocation(line: 6, column: 1, scope: !10)
