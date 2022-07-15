;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-dp-to-fp-load-store -S < %s | FileCheck %s
; ------------------------------------------------
; HandleLoadStoreInstructions
; ------------------------------------------------
; This test checks that HandleLoadStoreInstructions pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
; TODO: check types

; CHECK: define spir_kernel void @test_load
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ACAST_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} addrspace(8585216)* [[ACAST_V]]
; CHECK-SAME: metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata double [[LF_V:%[A-z0-9.]*]], metadata [[LF_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LF_LOC:![0-9]*]]
; CHECK-DAG: [[LF_V]] = {{.*}} !dbg [[LF_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata <3 x double> [[LVF_V:%[A-z0-9.]*]], metadata [[LVF_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LVF_LOC:![0-9]*]]
; CHECK-DAG: [[LVF_V]] = {{.*}} !dbg [[LVF_LOC]]
;
; CHECK: [[IPTR_V:%[0-9A-z.]*]] = inttoptr
; CHECK-SAME: !dbg [[IPTR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} [[IPTR_V]], metadata [[IPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IPTR_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i1 [[LB_V:%[A-z0-9.]*]], metadata [[LB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LB_LOC:![0-9]*]]
; CHECK-DAG: [[LB_V]] = {{.*}} !dbg [[LB_LOC]]
;


define spir_kernel void @test_load(double* %src, <3 x double> addrspace(65536)* %srcv, i32 %srci) !dbg !6 {
  %addr = addrspacecast double* %src to double addrspace(8585216)*, !dbg !20
  call void @llvm.dbg.value(metadata double addrspace(8585216)* %addr, metadata !9, metadata !DIExpression()), !dbg !20
  %1 = load double, double addrspace(8585216)* %addr, !dbg !21
  call void @llvm.dbg.value(metadata double %1, metadata !11, metadata !DIExpression()), !dbg !21
  %2 = load <3 x double>, <3 x double> addrspace(65536)* %srcv, !dbg !22
  call void @llvm.dbg.value(metadata <3 x double> %2, metadata !12, metadata !DIExpression()), !dbg !22
  %3 = inttoptr i32 %srci to i1 addrspace(8585216)*, !dbg !23
  call void @llvm.dbg.value(metadata i1 addrspace(8585216)* %3, metadata !14, metadata !DIExpression()), !dbg !23
  %4 = load i1, i1 addrspace(8585216)* %3, !dbg !24
  call void @llvm.dbg.value(metadata i1 %4, metadata !15, metadata !DIExpression()), !dbg !24
  %5 = extractelement <3 x double> %2, i32 1, !dbg !25
  call void @llvm.dbg.value(metadata double %5, metadata !17, metadata !DIExpression()), !dbg !25
  %6 = select i1 %4, double %1, double %5, !dbg !26
  call void @llvm.dbg.value(metadata double %6, metadata !18, metadata !DIExpression()), !dbg !26
  %7 = fadd double %6, %1, !dbg !27
  call void @llvm.dbg.value(metadata double %7, metadata !19, metadata !DIExpression()), !dbg !27
  ret void, !dbg !28
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "load.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_load", linkageName: "test_load", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LF_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[LF_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LVF_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LVF_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IPTR_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[IPTR_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LB_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LB_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "load.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 8}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_load", linkageName: "test_load", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15, !17, !18, !19}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !16)
!16 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!20 = !DILocation(line: 1, column: 1, scope: !6)
!21 = !DILocation(line: 2, column: 1, scope: !6)
!22 = !DILocation(line: 3, column: 1, scope: !6)
!23 = !DILocation(line: 4, column: 1, scope: !6)
!24 = !DILocation(line: 5, column: 1, scope: !6)
!25 = !DILocation(line: 6, column: 1, scope: !6)
!26 = !DILocation(line: 7, column: 1, scope: !6)
!27 = !DILocation(line: 8, column: 1, scope: !6)
!28 = !DILocation(line: 9, column: 1, scope: !6)
