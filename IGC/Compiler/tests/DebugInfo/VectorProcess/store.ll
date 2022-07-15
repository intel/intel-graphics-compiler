;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-vectorprocess -S < %s | FileCheck %s
; ------------------------------------------------
; VectorProcess : store
; ------------------------------------------------
; This test checks that VectorProcess pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_vectorpro
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]

; CHECK-DAG: store {{.*}} [[S1_V:%[A-z0-9]*]],{{.*}} [[S1_PTR:%[A-z0-9]*]], align 2, !dbg [[S1_LOC:![0-9]*]]
; CHECK-DAG: [[S1_PTR]] = {{.*}} [[S1_LOC]]
; CHECK-DAG: [[S1_V]] = {{.*}} [[S1_LOC]]
; CHECK-DAG: store {{.*}} [[S2_V:%[A-z0-9]*]],{{.*}} [[S2_PTR:%[A-z0-9]*]], align 1, !dbg [[S2_LOC:![0-9]*]]
; CHECK-DAG: [[S2_PTR]] = {{.*}} [[S2_LOC]]
; CHECK-DAG: [[S2_V]] = {{.*}} [[S2_LOC]]
; CHECK-DAG: store {{.*}} {{.*}}i64 15{{.*}},{{.*}} [[S3_PTR:%[A-z0-9]*]], align 4, !dbg [[S3_LOC:![0-9]*]]
; CHECK-DAG: [[S3_PTR]] = {{.*}} [[S3_LOC]]
; CHECK-DAG: store {{.*}}<i8 1, i8 8>{{.*}},{{.*}} [[S4_PTR:%[A-z0-9]*]], align 1, !dbg [[S4_LOC:![0-9]*]]
; CHECK-DAG: [[S4_PTR]] = {{.*}} [[S4_LOC]]


define void @test_vectorpro(<2 x i16>* %src1, <2 x i8>* %src2, <3 x i32> addrspace(2)* %src3, i32 %src4) !dbg !6 {
  %1 = alloca <2 x i16>*, align 4, !dbg !17
  call void @llvm.dbg.value(metadata <2 x i16>** %1, metadata !9, metadata !DIExpression()), !dbg !17
  store <2 x i16>* %src1, <2 x i16>** %1, align 2, !dbg !18
  %2 = alloca <2 x i16*>, align 4, !dbg !19
  call void @llvm.dbg.value(metadata <2 x i16*>* %2, metadata !11, metadata !DIExpression()), !dbg !19
  %3 = inttoptr i32 %src4 to <2 x i16*> addrspace(1)*, !dbg !20
  call void @llvm.dbg.value(metadata <2 x i16*> addrspace(1)* %3, metadata !12, metadata !DIExpression()), !dbg !20
  %4 = load <2 x i16*>, <2 x i16*> addrspace(1)* %3, align 1, !dbg !21
  call void @llvm.dbg.value(metadata <2 x i16*> %4, metadata !13, metadata !DIExpression()), !dbg !21
  store <2 x i16*> %4, <2 x i16*>* %2, align 1, !dbg !22
  %5 = alloca i16 addrspace(1)*, align 4, !dbg !23
  call void @llvm.dbg.value(metadata i16 addrspace(1)** %5, metadata !15, metadata !DIExpression()), !dbg !23
  %6 = inttoptr i64 15 to i16 addrspace(1)*, !dbg !24
  call void @llvm.dbg.value(metadata i16 addrspace(1)* %6, metadata !16, metadata !DIExpression()), !dbg !24
  store i16 addrspace(1)* %6, i16 addrspace(1)** %5, align 4, !dbg !25
  store <2 x i8> <i8 1, i8 8>, <2 x i8>* %src2, align 1, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "store.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vectorpro", linkageName: "test_vectorpro", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[S1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[S2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[S3_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[S4_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "store.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_vectorpro", linkageName: "test_vectorpro", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !14)
!14 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 8, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
!27 = !DILocation(line: 11, column: 1, scope: !6)
