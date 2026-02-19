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
; CHECK-DAG: [[ACAST_V:%[0-9]*]] = {{.*}} !dbg [[ACAST_LOC:![0-9]*]]
; CHECK-DAG: call void @llvm.dbg.value(metadata float{{.*}} [[ACAST_V]]
; CHECK-SAME: metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC]]
;
; CHECK: store float {{.*}} !dbg [[STORE_LOC:![0-9]*]]
;
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load float, {{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata float [[LOAD_V]]
; CHECK-SAME: metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
;
; CHECK: [[BCAST_V:%[A-z0-9]*]] = bitcast {{.*}} to <2 x i16> {{.*}}, !dbg [[BCAST_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata <2 x i16> {{.*}} [[BCAST_V]]
; CHECK-SAME: metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC]]
;
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load <2 x i16>, {{.*}} !dbg [[LOADV_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata <2 x i16> [[LOAD_V]]
; CHECK-SAME: metadata [[LOADV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOADV_LOC]]
;
; CHECK: [[IPTR_V:%[A-z0-9]*]] = ptrtoint {{.*}}, !dbg [[IPTR_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[IPTR_V]]
; CHECK-SAME: metadata [[IPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IPTR_LOC]]
;
; CHECK: [[EXTR_V:%[A-z0-9]*]] = extractelement {{.*}}, !dbg [[EXTR_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i16 [[EXTR_V]]
; CHECK-SAME: metadata [[EXTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR_LOC]]
;
; CHECK: [[GEP_V:%[A-z0-9]*]] = getelementptr {{.*}}, !dbg [[GEP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata float {{.*}} [[GEP_V]]
; CHECK-SAME: metadata [[GEP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP_LOC]]

define spir_kernel void @test_kernel(i32 addrspace(1)* %src) !dbg !10 {
  %1 = addrspacecast i32 addrspace(1)* %src to float addrspace(4)*, !dbg !23
  call void @llvm.dbg.value(metadata float addrspace(4)* %1, metadata !13, metadata !DIExpression()), !dbg !23
  store float 1.000000e+00, float addrspace(4)* %1, !dbg !24
  %2 = load float, float addrspace(4)* %1, !dbg !25
  call void @llvm.dbg.value(metadata float %2, metadata !15, metadata !DIExpression()), !dbg !25
  %3 = bitcast float addrspace(4)* %1 to <2 x i16> addrspace(4)*, !dbg !26
  call void @llvm.dbg.value(metadata <2 x i16> addrspace(4)* %3, metadata !17, metadata !DIExpression()), !dbg !26
  %4 = load <2 x i16>, <2 x i16> addrspace(4)* %3, !dbg !27
  call void @llvm.dbg.value(metadata <2 x i16> %4, metadata !18, metadata !DIExpression()), !dbg !27
  %5 = ptrtoint float addrspace(4)* %1 to i32, !dbg !28
  call void @llvm.dbg.value(metadata i32 %5, metadata !19, metadata !DIExpression()), !dbg !28
  %6 = extractelement <2 x i16> %4, i32 0, !dbg !29
  call void @llvm.dbg.value(metadata i16 %6, metadata !20, metadata !DIExpression()), !dbg !29
  %7 = getelementptr inbounds float, float addrspace(4)* %1, i16 %6, !dbg !30
  call void @llvm.dbg.value(metadata float addrspace(4)* %7, metadata !22, metadata !DIExpression()), !dbg !30
  ret void, !dbg !31
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !5, line: 1
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOADV_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LOADV_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IPTR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[IPTR_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[EXTR_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GEP_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[GEP_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32 addrspace(1)*)* @test_kernel, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "basic.ll", directory: "/")
!6 = !{}
!7 = !{i32 9}
!8 = !{i32 7}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17, !18, !19, !20, !22}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 3, type: !16)
!16 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 4, type: !14)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 5, type: !16)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 6, type: !16)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 7, type: !21)
!21 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 8, type: !14)
!23 = !DILocation(line: 1, column: 1, scope: !10)
!24 = !DILocation(line: 2, column: 1, scope: !10)
!25 = !DILocation(line: 3, column: 1, scope: !10)
!26 = !DILocation(line: 4, column: 1, scope: !10)
!27 = !DILocation(line: 5, column: 1, scope: !10)
!28 = !DILocation(line: 6, column: 1, scope: !10)
!29 = !DILocation(line: 7, column: 1, scope: !10)
!30 = !DILocation(line: 8, column: 1, scope: !10)
!31 = !DILocation(line: 9, column: 1, scope: !10)
