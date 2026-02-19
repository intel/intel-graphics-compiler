;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-extension-funcs-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; ExtensionFuncsAnalysis
; ------------------------------------------------
; This test checks that ExtensionFuncsAnalysis  pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; This is analysis pass, check that debug is unmodified
; ------------------------------------------------


; CHECK: @test_extfunca{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[CALL1_V:%[A-z0-9]*]] = call spir_func i32{{.*}} !dbg [[CALL1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[CALL1_V]], metadata [[CALL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL1_LOC]]
; CHECK: [[CALL2_V:%[A-z0-9]*]] = call spir_func i32{{.*}} !dbg [[CALL2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[CALL2_V]], metadata [[CALL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL2_LOC]]
; CHECK: [[CALL3_V:%[A-z0-9]*]] = call spir_func i32{{.*}} !dbg [[CALL3_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[CALL3_V]], metadata [[CALL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL3_LOC]]
; CHECK: [[CALL4_V:%[A-z0-9]*]] = call spir_func i32{{.*}} !dbg [[CALL4_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[CALL4_V]], metadata [[CALL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL4_LOC]]
; CHECK: [[ADD1_V:%[A-z0-9]*]] = add i32{{.*}} !dbg [[ADD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD1_V]], metadata [[ADD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD1_LOC]]
; CHECK: [[ADD2_V:%[A-z0-9]*]] = add i32{{.*}} !dbg [[ADD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD2_V]], metadata [[ADD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD2_LOC]]
; CHECK: [[ADD3_V:%[A-z0-9]*]] = add i32{{.*}} !dbg [[ADD3_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD3_V]], metadata [[ADD3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD3_LOC]]
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[CALL5_V:%[A-z0-9]*]] = call spir_func %struct.mce_payload_t*{{.*}} !dbg [[CALL5_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata %struct.mce_payload_t* [[CALL5_V]], metadata [[CALL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL5_LOC]]
; CHECK: [[CALL6_V:%[A-z0-9]*]] = call spir_func <4 x i32>{{.*}} !dbg [[CALL6_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <4 x i32> [[CALL6_V]], metadata [[CALL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL6_LOC]]

%struct.mce_payload_t = type opaque

define spir_kernel void @test_extfunca(<4 x i32> %a, i32* %b) !dbg !10 {
  %1 = call spir_func i32 @__builtin_IB_vme_mb_block_type(), !dbg !25
  call void @llvm.dbg.value(metadata i32 %1, metadata !13, metadata !DIExpression()), !dbg !25
  %2 = call spir_func i32 @__builtin_IB_vme_subpixel_mode(), !dbg !26
  call void @llvm.dbg.value(metadata i32 %2, metadata !15, metadata !DIExpression()), !dbg !26
  %3 = call spir_func i32 @__builtin_IB_vme_sad_adjust_mode(), !dbg !27
  call void @llvm.dbg.value(metadata i32 %3, metadata !16, metadata !DIExpression()), !dbg !27
  %4 = call spir_func i32 @__builtin_IB_vme_search_path_type(), !dbg !28
  call void @llvm.dbg.value(metadata i32 %4, metadata !17, metadata !DIExpression()), !dbg !28
  %5 = add i32 %1, %2, !dbg !29
  call void @llvm.dbg.value(metadata i32 %5, metadata !18, metadata !DIExpression()), !dbg !29
  %6 = add i32 %2, %3, !dbg !30
  call void @llvm.dbg.value(metadata i32 %6, metadata !19, metadata !DIExpression()), !dbg !30
  %7 = add i32 %5, %6, !dbg !31
  call void @llvm.dbg.value(metadata i32 %7, metadata !20, metadata !DIExpression()), !dbg !31
  store i32 %7, i32* %b, align 4, !dbg !32
  %8 = call spir_func %struct.mce_payload_t* @__builtin_IB_vme_helper_get_as_avc_mce_payload_t(<4 x i32> %a), !dbg !33
  call void @llvm.dbg.value(metadata %struct.mce_payload_t* %8, metadata !21, metadata !DIExpression()), !dbg !33
  %9 = call spir_func <4 x i32> @__builtin_IB_vme_helper_get_handle_avc_mce_payload_t(%struct.mce_payload_t* %8), !dbg !34
  call void @llvm.dbg.value(metadata <4 x i32> %9, metadata !23, metadata !DIExpression()), !dbg !34
  ret void, !dbg !35
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ExtensionFuncsAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_extfunca", linkageName: "test_extfunca", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[CALL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CALL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[CALL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD1_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[ADD1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD2_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[ADD2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD3_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[ADD3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL5_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[CALL5_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL6_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[CALL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

declare spir_func i32 @__builtin_IB_vme_mb_block_type()

declare spir_func i32 @__builtin_IB_vme_subpixel_mode()

declare spir_func i32 @__builtin_IB_vme_sad_adjust_mode()

declare spir_func i32 @__builtin_IB_vme_search_path_type()

declare spir_func <4 x i32> @__builtin_IB_vme_helper_get_handle_avc_mce_payload_t(%struct.mce_payload_t*)

declare spir_func %struct.mce_payload_t* @__builtin_IB_vme_helper_get_as_avc_mce_payload_t(<4 x i32>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (<4 x i32>, i32*)* @test_extfunca, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "ExtensionFuncsAnalysis.ll", directory: "/")
!6 = !{}
!7 = !{i32 11}
!8 = !{i32 9}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_extfunca", linkageName: "test_extfunca", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !17, !18, !19, !20, !21, !23}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !14)
!17 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !14)
!18 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 5, type: !14)
!19 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 6, type: !14)
!20 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 7, type: !14)
!21 = !DILocalVariable(name: "8", scope: !10, file: !5, line: 9, type: !22)
!22 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!23 = !DILocalVariable(name: "9", scope: !10, file: !5, line: 10, type: !24)
!24 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!25 = !DILocation(line: 1, column: 1, scope: !10)
!26 = !DILocation(line: 2, column: 1, scope: !10)
!27 = !DILocation(line: 3, column: 1, scope: !10)
!28 = !DILocation(line: 4, column: 1, scope: !10)
!29 = !DILocation(line: 5, column: 1, scope: !10)
!30 = !DILocation(line: 6, column: 1, scope: !10)
!31 = !DILocation(line: 7, column: 1, scope: !10)
!32 = !DILocation(line: 8, column: 1, scope: !10)
!33 = !DILocation(line: 9, column: 1, scope: !10)
!34 = !DILocation(line: 10, column: 1, scope: !10)
!35 = !DILocation(line: 11, column: 1, scope: !10)
