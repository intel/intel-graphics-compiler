;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-low-precision-opt -S < %s | FileCheck %s
; ------------------------------------------------
; LowPrecisionOpt
; ------------------------------------------------
; This test checks that LowPrecisionOpt pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = './LowPrecisionOpt/fpext.ll'
source_filename = "./LowPrecisionOpt/fpext.ll"

define void @test_low(float %src) !dbg !9 {
entry:
  %0 = fadd float %src, 2.000000e+00, !dbg !22
  call void @llvm.dbg.value(metadata float %0, metadata !12, metadata !DIExpression()), !dbg !22
  %1 = fadd float %src, %0, !dbg !23
  call void @llvm.dbg.value(metadata float %1, metadata !14, metadata !DIExpression()), !dbg !23
;
; Testcase 1:
; fpext and fptrunc
;
; CHECK: [[TRUNC_V:%[0-9]*]] = fptrunc float [[ADD_V:%[0-9]*]] to half, !dbg [[TRUNC_LOC:![0-9]*]]
; CHECK-NEXT: dbg.value(metadata half [[TRUNC_V]], metadata [[TRUNC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TRUNC_LOC]]
;
; Check that fpext is removed and its value is RAUW
;
; CHECK-NEXT: dbg.value(metadata float [[ADD_V]], metadata [[EXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXT_LOC:![0-9]*]]
; CHECK: store float [[ADD_V]], float* [[STORE_A:%[0-9]*]],{{.*}} !dbg [[STORE_LOC:![0-9]*]]
; CHECK-NEXT: dbg.declare(metadata float* [[STORE_A]], metadata [[STORE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[STORE_LOC]]
  %2 = fptrunc float %1 to half, !dbg !24
  call void @llvm.dbg.value(metadata half %2, metadata !15, metadata !DIExpression()), !dbg !24
  %3 = fpext half %2 to float, !dbg !25
  call void @llvm.dbg.value(metadata float %3, metadata !17, metadata !DIExpression()), !dbg !25
  %4 = alloca float, align 4, !dbg !26
  store float %3, float* %4, align 4, !dbg !27
  call void @llvm.dbg.declare(metadata float* %4, metadata !18, metadata !DIExpression()), !dbg !27
;
; Testcase 2:
; fpext and half intrinsic
;
; Check that fpext is removed and its value is RAUW
;
; CHECK: [[CALL_V:%[0-9]*]] = call float @llvm.genx{{.*}} !dbg [[CALL_LOC:![0-9]*]]
; CHECK: dbg.value(metadata float [[CALL_V]], metadata [[EXT2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXT2_LOC:![0-9]*]]
; CHECK: store float [[CALL_V]], float* [[STORE_A]], align 4, !dbg [[STORE2_LOC:![0-9]*]]
  %5 = call half @llvm.genx.GenISA.DCL.inputVec.f16(i32 1, i32 2), !dbg !28
  call void @llvm.dbg.value(metadata half %5, metadata !20, metadata !DIExpression()), !dbg !28
  %6 = fpext half %5 to float, !dbg !29
  call void @llvm.dbg.value(metadata float %6, metadata !21, metadata !DIExpression()), !dbg !29
  store float %6, float* %4, align 4, !dbg !30
  ret void, !dbg !31
}

; Testcase 1 MD:
; CHECK-DAG: [[TRUNC_LOC]] = !DILocation(line: 3, column: 1, scope: !9)
; CHECK-DAG: [[TRUNC_MD]] = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !16)
;
; CHECK-DAG: [[EXT_LOC]] = !DILocation(line: 4, column: 1, scope: !9)
; CHECK-DAG: [[EXT_MD]] = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !13)
;
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 6, column: 1, scope: !9)
; CHECK-DAG: [[STORE_MD]] = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !19)

; Testcase 2 MD:
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 7, column: 1, scope: !9)
;
; CHECK-DAG: [[EXT2_LOC]] = !DILocation(line: 8, column: 1, scope: !9)
; CHECK-DAG: [[EXT2_MD]] = !DILocalVariable(name: "7", scope: !9, file: !4, line: 8, type: !13)
;
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 9, column: 1, scope: !9)

declare half @llvm.genx.GenISA.DCL.inputVec.f16(i32, i32)

declare float @llvm.genx.GenISA.RuntimeValue.f32(i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (float)* @test_low, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "./LowPrecisionOpt/fpext.ll", directory: "/")
!5 = !{}
!6 = !{i32 10}
!7 = !{i32 7}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_low", linkageName: "test_low", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !17, !18, !20, !21}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !16)
!16 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !13)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !19)
!19 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 7, type: !16)
!21 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 8, type: !13)
!22 = !DILocation(line: 1, column: 1, scope: !9)
!23 = !DILocation(line: 2, column: 1, scope: !9)
!24 = !DILocation(line: 3, column: 1, scope: !9)
!25 = !DILocation(line: 4, column: 1, scope: !9)
!26 = !DILocation(line: 5, column: 1, scope: !9)
!27 = !DILocation(line: 6, column: 1, scope: !9)
!28 = !DILocation(line: 7, column: 1, scope: !9)
!29 = !DILocation(line: 8, column: 1, scope: !9)
!30 = !DILocation(line: 9, column: 1, scope: !9)
!31 = !DILocation(line: 10, column: 1, scope: !9)
