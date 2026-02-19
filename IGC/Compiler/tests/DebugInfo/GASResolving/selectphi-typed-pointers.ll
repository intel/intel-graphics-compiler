;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-gas-resolve -S < %s | FileCheck %s
; ------------------------------------------------
; GASResolve
; ------------------------------------------------
; This test checks that GASResolve pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_kernel
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value(metadata i32{{.*}} [[SRC_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SRC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value(metadata i32{{.*}} [[DST_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: [[CMP_V:%[A-z0-9]*]] = icmp {{.*}} !dbg [[CMP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i1 [[CMP_V]]
; CHECK-SAME: metadata [[CMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CMP_LOC]]
;
; CHECK: [[SELECT_V:%[A-z0-9]*]] = select {{.*}} !dbg [[SELECT_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32{{.*}} [[SELECT_V]]
; CHECK-SAME: metadata [[SELECT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT_LOC]]
;
; CHECK: lbl:
; CHECK: [[PHI_V:%[A-z0-9]*]] = phi {{.*}} !dbg [[PHI_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32{{.*}} [[PHI_V]]
; CHECK-SAME: metadata [[PHI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI_LOC]]

define spir_kernel void @test_kernel(i32 addrspace(1)* %src, i32 addrspace(1)* %dst) !dbg !10 {
entry:
  %0 = addrspacecast i32 addrspace(1)* %src to i32 addrspace(4)*, !dbg !21
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %0, metadata !13, metadata !DIExpression()), !dbg !21
  %1 = addrspacecast i32 addrspace(1)* %dst to i32 addrspace(4)*, !dbg !22
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %1, metadata !15, metadata !DIExpression()), !dbg !22
  %2 = icmp ult i32 addrspace(4)* %0, %1, !dbg !23
  call void @llvm.dbg.value(metadata i1 %2, metadata !16, metadata !DIExpression()), !dbg !23
  %3 = select i1 %2, i32 addrspace(4)* %0, i32 addrspace(4)* %1, !dbg !24
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %3, metadata !18, metadata !DIExpression()), !dbg !24
  br label %lbl, !dbg !25

lbl:                                              ; preds = %lbl, %entry
  %4 = phi i32 addrspace(4)* [ %0, %entry ], [ %1, %lbl ], !dbg !26
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %4, metadata !19, metadata !DIExpression()), !dbg !26
  %5 = icmp eq i32 addrspace(4)* %0, %4, !dbg !27
  call void @llvm.dbg.value(metadata i1 %5, metadata !20, metadata !DIExpression()), !dbg !27
  br i1 %5, label %lbl, label %end, !dbg !28

end:                                              ; preds = %lbl
  ret void, !dbg !29
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "selectphi.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CMP_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CMP_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[SELECT_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[PHI_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @test_kernel, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "selectphi.ll", directory: "/")
!6 = !{}
!7 = !{i32 9}
!8 = !{i32 6}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !18, !19, !20}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !17)
!17 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !14)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 6, type: !14)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 7, type: !17)
!21 = !DILocation(line: 1, column: 1, scope: !10)
!22 = !DILocation(line: 2, column: 1, scope: !10)
!23 = !DILocation(line: 3, column: 1, scope: !10)
!24 = !DILocation(line: 4, column: 1, scope: !10)
!25 = !DILocation(line: 5, column: 1, scope: !10)
!26 = !DILocation(line: 6, column: 1, scope: !10)
!27 = !DILocation(line: 7, column: 1, scope: !10)
!28 = !DILocation(line: 8, column: 1, scope: !10)
!29 = !DILocation(line: 9, column: 1, scope: !10)
