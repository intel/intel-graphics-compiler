;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-promote-resources-to-direct-addrspace -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteResourceToDirectAS : load/store promotion part
; ------------------------------------------------
; This test checks that PromoteResourceToDirectAS pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_promote
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ACAST1_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} addrspace(1)* [[ACAST1_V]]
; CHECK-SAME: metadata [[ACAST1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST1_LOC]]
;
; CHECK: [[ACAST2_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} addrspace(2)* [[ACAST2_V]]
; CHECK-SAME: metadata [[ACAST2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST2_LOC]]
;
; CHECK-DAG: [[LOAD_V:%[A-z0-9]*]] = load {{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}} [[LOAD_V]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK-DAG: store i32 [[LOAD_V]], {{.*}} [[BCAST_V:%[A-z0-9]*]],{{.*}} !dbg [[STORE_LOC:![0-9]*]]
; CHECK-DAG: [[BCAST_V]] = {{.*}} [[GEP_V:%[A-z0-9]*]] {{.*}} !dbg [[BCAST_LOC:![0-9]*]]

; COM: This check fails - @llvm.dbg.value(metadata {{.*}} [[BCAST_V]], metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC]]
; CHECK-DAG: [[GEP_V]] = {{.*}} !dbg [[GEP_LOC:![0-9]*]]
; COM: This check fails - @llvm.dbg.value(metadata {{.*}} [[GEP_V]], metadata [[GEP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP_LOC]]

define spir_kernel void @test_promote(i32 %src) !dbg !6 {
  %1 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 0, i32 2), !dbg !18
  call void @llvm.dbg.value(metadata i32 addrspace(196608)* %1, metadata !9, metadata !DIExpression()), !dbg !18
  %2 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 1, i32 3), !dbg !19
  call void @llvm.dbg.value(metadata i32 addrspace(196608)* %2, metadata !11, metadata !DIExpression()), !dbg !19
  %3 = addrspacecast i32 addrspace(196608)* %1 to i32 addrspace(1)*, !dbg !20
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %3, metadata !12, metadata !DIExpression()), !dbg !20
  %4 = addrspacecast i32 addrspace(196608)* %2 to float addrspace(2)*, !dbg !21
  call void @llvm.dbg.value(metadata float addrspace(2)* %4, metadata !13, metadata !DIExpression()), !dbg !21
  %5 = getelementptr inbounds float, float addrspace(2)* %4, i64 2, !dbg !22
  call void @llvm.dbg.value(metadata float addrspace(2)* %5, metadata !14, metadata !DIExpression()), !dbg !22
  %6 = bitcast float addrspace(2)* %5 to i32 addrspace(2)*, !dbg !23
  call void @llvm.dbg.value(metadata i32 addrspace(2)* %6, metadata !15, metadata !DIExpression()), !dbg !23
  %7 = load i32, i32 addrspace(1)* %3, !dbg !24
  call void @llvm.dbg.value(metadata i32 %7, metadata !16, metadata !DIExpression()), !dbg !24
  store i32 %7, i32 addrspace(2)* %6, !dbg !25
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "loadstore.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[ACAST1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ACAST2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[ACAST2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; COM: This check fails - [[GEP_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[GEP_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; cHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

declare i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "loadstore.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 7}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 7, column: 1, scope: !6)
!25 = !DILocation(line: 8, column: 1, scope: !6)
!26 = !DILocation(line: 9, column: 1, scope: !6)
