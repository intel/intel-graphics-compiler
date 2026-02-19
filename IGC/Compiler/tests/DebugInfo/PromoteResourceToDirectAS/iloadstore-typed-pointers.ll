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
; PromoteResourceToDirectAS : load/store intrinsic promotion part
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
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load {{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} [[LOAD_V]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: store float [[LOAD_V]], {{.*}} !dbg [[STORE_LOC:![0-9]*]]

define spir_kernel void @test_promote(i32 %src) !dbg !6 {
  %1 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 0, i32 2), !dbg !16
  call void @llvm.dbg.value(metadata i32 addrspace(196608)* %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = call i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32 1, i32 3), !dbg !17
  call void @llvm.dbg.value(metadata i32 addrspace(196608)* %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = addrspacecast i32 addrspace(196608)* %1 to float addrspace(1)*, !dbg !18
  call void @llvm.dbg.value(metadata float addrspace(1)* %3, metadata !12, metadata !DIExpression()), !dbg !18
  %4 = addrspacecast i32 addrspace(196608)* %2 to float addrspace(2)*, !dbg !19
  call void @llvm.dbg.value(metadata float addrspace(2)* %4, metadata !13, metadata !DIExpression()), !dbg !19
  %5 = call float @llvm.genx.GenISA.ldraw_indexed.p1f32(float addrspace(1)* %3, i32 4, i32 4, i1 false), !dbg !20
  call void @llvm.dbg.value(metadata float %5, metadata !14, metadata !DIExpression()), !dbg !20
  call void @llvm.genx.GenISA.storeraw_indexed.p2f32(float addrspace(2)* %4, i32 4, float %5, i32 8, i1 false), !dbg !21
  ret void, !dbg !22
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "iloadstore.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[ACAST1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ACAST2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[ACAST2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

declare i32 addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608i32(i32, i32)

declare float @llvm.genx.GenISA.ldraw_indexed.p1f32(float addrspace(1)*, i32, i32, i1)

declare void @llvm.genx.GenISA.storeraw_indexed.p2f32(float addrspace(2)*, i32, float, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "iloadstore.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !15)
!15 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
