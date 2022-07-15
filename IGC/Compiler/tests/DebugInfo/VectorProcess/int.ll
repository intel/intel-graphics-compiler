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
; VectorProcess : intrinsics
; ------------------------------------------------
; This test checks that VectorProcess pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_vectorpro
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK: [[LOAD_V:%[A-z0-9]*]] = call {{.*}}ldrawvector{{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; Note: vectors are not currently salvageble in vanila llvm
; CHECK: @llvm.dbg.value(metadata {{.*}}, metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: @llvm.dbg.value(metadata {{.*}} [[LOAD_V]], metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC:![0-9]*]]
; CHECK: call {{.*}}storerawvector{{.*}} !dbg [[CST_LOC:![0-9]*]]
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]
; CHECK: [[BCAST2_V:%[A-z0-9]*]] = bitcast {{.*}}[[LOAD_V]] to float, !dbg [[BCAST2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} [[BCAST2_V]], metadata [[BCAST2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST2_LOC]]
;


define void @test_vectorpro(<2 x i16> addrspace(1)* %src1) !dbg !6 {
  %1 = call <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.p1v2i16(<2 x i16> addrspace(1)* %src1, i32 4, i32 2, i1 true), !dbg !16
  call void @llvm.dbg.value(metadata <2 x i16> %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = bitcast <2 x i16> %1 to i32, !dbg !17
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !17
  call void @llvm.genx.GenISA.storerawvector.indexed.p1v2i16(<2 x i16> addrspace(1)* %src1, i32 0, <2 x i16> %1, i32 2, i1 true), !dbg !18
  %3 = bitcast <2 x i16> addrspace(1)* %src1 to i32 addrspace(1)*, !dbg !19
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %3, metadata !12, metadata !DIExpression()), !dbg !19
  store i32 %2, i32 addrspace(1)* %3, !dbg !20
  %4 = bitcast <2 x i16> %1 to float, !dbg !21
  call void @llvm.dbg.value(metadata float %4, metadata !14, metadata !DIExpression()), !dbg !21
  %5 = bitcast <2 x i16> addrspace(1)* %src1 to float addrspace(1)*, !dbg !22
  call void @llvm.dbg.value(metadata float addrspace(1)* %5, metadata !15, metadata !DIExpression()), !dbg !22
  store float %4, float addrspace(1)* %5, !dbg !23
  ret void, !dbg !24
}
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "int.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vectorpro", linkageName: "test_vectorpro", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CST_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[BCAST2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
;

declare <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.p1v2i16(<2 x i16> addrspace(1)*, i32, i32, i1)

declare void @llvm.genx.GenISA.storerawvector.indexed.p1v2i16(<2 x i16> addrspace(1)*, i32, <2 x i16>, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "int.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_vectorpro", linkageName: "test_vectorpro", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 6, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !13)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
!24 = !DILocation(line: 9, column: 1, scope: !6)
