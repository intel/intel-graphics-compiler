;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-fold-workgroup-sizes -S < %s | FileCheck %s
; ------------------------------------------------
; FoldKnownWorkGroupSizes
; ------------------------------------------------
; This test checks that FoldKnownWorkGroupSizes pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; Sanity checks (scope, allocas)
; CHECK: @test_foldws{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[AGO_V:%[A-z0-9]*]] = alloca i32{{.*}} !dbg [[AGO_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32* [[AGO_V]], metadata [[AGO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AGO_LOC]]
; CHECK: [[ALS_V:%[A-z0-9]*]] = alloca i32{{.*}} !dbg [[ALS_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32* [[ALS_V]], metadata [[ALS_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALS_LOC]]
; CHECK: @llvm.dbg.value(metadata i32 0, metadata [[GO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GO_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 13, metadata [[LS_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LS_LOC:![0-9]*]]
; CHECK: store i32 0, i32* [[AGO_V]], align 4, !dbg [[STOREGO_LOC:![0-9]*]]
; CHECK: store i32 13, i32* [[ALS_V]], align 4, !dbg [[STORELS_LOC:![0-9]*]]

define void @test_foldws() !dbg !13 {
  %1 = alloca i32, align 4, !dbg !22
  call void @llvm.dbg.value(metadata i32* %1, metadata !16, metadata !DIExpression()), !dbg !22
  %2 = alloca i32, align 4, !dbg !23
  call void @llvm.dbg.value(metadata i32* %2, metadata !18, metadata !DIExpression()), !dbg !23
  %3 = call spir_func i32 @__builtin_IB_get_global_offset(i32 12), !dbg !24
  call void @llvm.dbg.value(metadata i32 %3, metadata !19, metadata !DIExpression()), !dbg !24
  %4 = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 0), !dbg !25
  call void @llvm.dbg.value(metadata i32 %4, metadata !21, metadata !DIExpression()), !dbg !25
  store i32 %3, i32* %1, align 4, !dbg !26
  store i32 %4, i32* %2, align 4, !dbg !27
  ret void, !dbg !28
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "FoldKnownWorkGroupSizes.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_foldws", linkageName: "test_foldws", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[AGO_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[AGO_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALS_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[ALS_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GO_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[GO_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LS_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[LS_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STOREGO_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORELS_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

declare spir_func i32 @__builtin_IB_get_global_offset(i32)

declare spir_func i32 @__builtin_IB_get_enqueued_local_size(i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!igc.functions = !{!3}
!llvm.dbg.cu = !{!7}
!llvm.debugify = !{!10, !11}
!llvm.module.flags = !{!12}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"replaceGlobalOffsetsByZero", i1 true}
!3 = !{void ()* @test_foldws, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"thread_group_size", i32 13, i32 2, i32 3}
!7 = distinct !DICompileUnit(language: DW_LANG_C, file: !8, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !9)
!8 = !DIFile(filename: "FoldKnownWorkGroupSizes.ll", directory: "/")
!9 = !{}
!10 = !{i32 7}
!11 = !{i32 4}
!12 = !{i32 2, !"Debug Info Version", i32 3}
!13 = distinct !DISubprogram(name: "test_foldws", linkageName: "test_foldws", scope: null, file: !8, line: 1, type: !14, scopeLine: 1, unit: !7, retainedNodes: !15)
!14 = !DISubroutineType(types: !9)
!15 = !{!16, !18, !19, !21}
!16 = !DILocalVariable(name: "1", scope: !13, file: !8, line: 1, type: !17)
!17 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "2", scope: !13, file: !8, line: 2, type: !17)
!19 = !DILocalVariable(name: "3", scope: !13, file: !8, line: 3, type: !20)
!20 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "4", scope: !13, file: !8, line: 4, type: !20)
!22 = !DILocation(line: 1, column: 1, scope: !13)
!23 = !DILocation(line: 2, column: 1, scope: !13)
!24 = !DILocation(line: 3, column: 1, scope: !13)
!25 = !DILocation(line: 4, column: 1, scope: !13)
!26 = !DILocation(line: 5, column: 1, scope: !13)
!27 = !DILocation(line: 6, column: 1, scope: !13)
!28 = !DILocation(line: 7, column: 1, scope: !13)
