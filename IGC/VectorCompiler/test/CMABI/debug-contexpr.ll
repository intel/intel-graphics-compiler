;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; CMABI
; ------------------------------------------------
; This test checks that CMABI pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define {{.*}} @test_constexpr{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <2 x i32> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <2 x i32> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK: store {{.*}}, [[STORE1_LOC:![0-9]*]]

define <2 x i32> @test_constexpr(<2 x i32> addrspace(15)* %a, i16 %b) !dbg !8 {
entry:
  %0 = insertelement <2 x i32> <i32 4, i32 bitcast (<4 x i8> <i8 13, i8 13, i8 13, i8 13> to i32)>, i32 15, i32 0, !dbg !14
  call void @llvm.dbg.value(metadata <2 x i32> %0, metadata !11, metadata !DIExpression()), !dbg !14
  %1 = add <2 x i32> %0, <i32 bitcast (<2 x i16> <i16 5421, i16 13> to i32), i32 bitcast (<2 x i16> <i16 5421, i16 13> to i32)>, !dbg !15
  call void @llvm.dbg.value(metadata <2 x i32> %1, metadata !13, metadata !DIExpression()), !dbg !15
  store <2 x i32> %1, <2 x i32> addrspace(15)* %a, !dbg !16
  ret <2 x i32> %1, !dbg !17
}

; CHECK: define dllexport void @test_cmabi{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <2 x i32> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]

define dllexport void @test_cmabi(<2 x i32> addrspace(15)* %a1, i16 %b1) !dbg !18 {
  %1 = call <2 x i32> @test_constexpr(<2 x i32> addrspace(15)* %a1, i16 %b1), !dbg !21
  call void @llvm.dbg.value(metadata <2 x i32> %1, metadata !20, metadata !DIExpression()), !dbg !21
  store <2 x i32> %1, <2 x i32> addrspace(15)* %a1, !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "constexpr.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test_constexpr", linkageName: "test_constexpr", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE1]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test_cmabi", linkageName: "test_cmabi", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE2]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE2]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.debugify = !{!0, !1, !2, !3}
!llvm.module.flags = !{!4}
!llvm.dbg.cu = !{!5}

!0 = !{i32 9}
!1 = !{i32 5}
!2 = !{i32 7}
!3 = !{i32 3}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !6, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !7)
!6 = !DIFile(filename: "constexpr.ll", directory: "/")
!7 = !{}
!8 = distinct !DISubprogram(name: "test_constexpr", linkageName: "test_constexpr", scope: null, file: !6, line: 1, type: !9, scopeLine: 1, unit: !5, retainedNodes: !10)
!9 = !DISubroutineType(types: !7)
!10 = !{!11, !13}
!11 = !DILocalVariable(name: "1", scope: !8, file: !6, line: 1, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "2", scope: !8, file: !6, line: 2, type: !12)
!14 = !DILocation(line: 1, column: 1, scope: !8)
!15 = !DILocation(line: 2, column: 1, scope: !8)
!16 = !DILocation(line: 3, column: 1, scope: !8)
!17 = !DILocation(line: 4, column: 1, scope: !8)
!18 = distinct !DISubprogram(name: "test_cmabi", linkageName: "test_cmabi", scope: null, file: !6, line: 5, type: !9, scopeLine: 5, unit: !5, retainedNodes: !19)
!19 = !{!20}
!20 = !DILocalVariable(name: "3", scope: !18, file: !6, line: 5, type: !12)
!21 = !DILocation(line: 5, column: 1, scope: !18)
!22 = !DILocation(line: 6, column: 1, scope: !18)
!23 = !DILocation(line: 7, column: 1, scope: !18)
