;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-extension-arg-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; ExtensionArgAnalysis
; ------------------------------------------------
; This test checks that ExtensionArgAnalysis  pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_extarga{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[MBLOCK_V:%[A-z0-9]*]] = call spir_func i8*{{.*}} !dbg [[MBLOCK_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i8* [[MBLOCK_V]], metadata [[MBLOCK_MD:![0-9]*]], metadata !DIExpression()), !dbg [[MBLOCK_LOC]]
; CHECK: [[FBRN_V:%[A-z0-9]*]] = call spir_func <4 x i32>{{.*}} !dbg [[FBRN_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <4 x i32> [[FBRN_V]], metadata [[FBRN_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FBRN_LOC]]
; CHECK: call spir_func void{{.*}} !dbg [[SENDI_LOC:![0-9]*]]
; CHECK: call spir_func void{{.*}} !dbg [[SENDS_LOC:![0-9]*]]
; CHECK: [[ALLOCA_V:%[A-z0-9]*]] = alloca <4 x i32>{{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <4 x i32>* [[ALLOCA_V]], metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: store {{.*}} !dbg [[STORE2_LOC:![0-9]*]]

define spir_kernel void @test_extarga(i32 %a, <2 x i32> %b, <4 x i32> %c) !dbg !10 {
  %1 = call spir_func i8* @__builtin_IB_media_block_read_uchar(i32 %a, <2 x i32> %b, i32 0, i32 1), !dbg !18
  call void @llvm.dbg.value(metadata i8* %1, metadata !13, metadata !DIExpression()), !dbg !18
  %2 = call spir_func <4 x i32> @__builtin_IB_vme_send_fbr_new(<4 x i32> %c, i64 42, i64 43, i64 44, i64 45), !dbg !19
  call void @llvm.dbg.value(metadata <4 x i32> %2, metadata !15, metadata !DIExpression()), !dbg !19
  call spir_func void @__builtin_IB_vme_send_ime(i32 1, i32 2, i32 3, i64 45, i64 46, i32 %a, i32 12, i32 3), !dbg !20
  call spir_func void @__builtin_IB_vme_send_sic(i32 %a, i32 31, i32 33, i64 44, i64 55, i64 66), !dbg !21
  %3 = alloca <4 x i32>, align 4, !dbg !22
  call void @llvm.dbg.value(metadata <4 x i32>* %3, metadata !17, metadata !DIExpression()), !dbg !22
  store i8 13, i8* %1, !dbg !23
  store <4 x i32> %2, <4 x i32>* %3, !dbg !24
  ret void, !dbg !25
}
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ExtensionArgAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_extarga", linkageName: "test_extarga", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[MBLOCK_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[MBLOCK_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FBRN_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[FBRN_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SENDI_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SENDS_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

declare spir_func i8* @__builtin_IB_media_block_read_uchar(i32, <2 x i32>, i32, i32)

declare spir_func <4 x i32> @__builtin_IB_vme_send_fbr_new(<4 x i32>, i64, i64, i64, i64)

declare spir_func void @__builtin_IB_vme_send_ime(i32, i32, i32, i64, i64, i32, i32, i32)

declare spir_func void @__builtin_IB_vme_send_sic(i32, i32, i32, i64, i64, i64)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32, <2 x i32>, <4 x i32>)* @test_extarga, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "ExtensionArgAnalysis.ll", directory: "/")
!6 = !{}
!7 = !{i32 8}
!8 = !{i32 3}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_extarga", linkageName: "test_extarga", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !16)
!16 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 5, type: !14)
!18 = !DILocation(line: 1, column: 1, scope: !10)
!19 = !DILocation(line: 2, column: 1, scope: !10)
!20 = !DILocation(line: 3, column: 1, scope: !10)
!21 = !DILocation(line: 4, column: 1, scope: !10)
!22 = !DILocation(line: 5, column: 1, scope: !10)
!23 = !DILocation(line: 6, column: 1, scope: !10)
!24 = !DILocation(line: 7, column: 1, scope: !10)
!25 = !DILocation(line: 8, column: 1, scope: !10)
