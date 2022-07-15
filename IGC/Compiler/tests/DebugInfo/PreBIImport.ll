;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-Pre-BIImport-Analysis -S < %s | FileCheck %s
; ------------------------------------------------
; PreBIImport
; ------------------------------------------------
; This test checks that PreBIImport pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'PreBIImport.ll'
source_filename = "PreBIImport.ll"

define spir_kernel void @test_sincos(float %src1, float* %dst) !dbg !10 {
; Testcase 1:
; direct fmul usage
; CHECK: [[COS_V:%[0-9]*]] = call float @__builtin_spirv_OpenCL_cos{{.*}}, !dbg [[COS_LOC:![0-9]*]]
; CHECK-NEXT: dbg.value(metadata float [[COS_V]],  metadata [[COS_MD:![0-9]*]], metadata !DIExpression()), !dbg [[COS_LOC]]
  %1 = fmul float 0x400921FB60000000, %src1, !dbg !21
  call void @llvm.dbg.value(metadata float %1, metadata !13, metadata !DIExpression()), !dbg !21
  %2 = call float @__builtin_spirv_OpenCL_cos_f32(float %1), !dbg !22
  call void @llvm.dbg.value(metadata float %2, metadata !15, metadata !DIExpression()), !dbg !22
; Testcase 2:
; alloced fmul usage
; CHECK: [[SIN_V:%[0-9]*]] = call float @__builtin_spirv_OpenCL_sin{{.*}}, !dbg [[SIN_LOC:![0-9]*]]
; CHECK-NEXT: dbg.value(metadata float [[SIN_V]],  metadata [[SIN_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SIN_LOC]]
  %3 = fmul float 0x401921FB60000000, %src1, !dbg !23
  call void @llvm.dbg.value(metadata float %3, metadata !16, metadata !DIExpression()), !dbg !23
  %4 = alloca float, align 4, !dbg !24
  store float %3, float* %4, !dbg !25
  call void @llvm.dbg.declare(metadata float* %4, metadata !17, metadata !DIExpression()), !dbg !24
  %5 = load float, float* %4, !dbg !26
  call void @llvm.dbg.value(metadata float %5, metadata !19, metadata !DIExpression()), !dbg !26
  %6 = call float @__builtin_spirv_OpenCL_sin_f32(float %5), !dbg !27
  call void @llvm.dbg.value(metadata float %6, metadata !20, metadata !DIExpression()), !dbg !27
  ret void, !dbg !28
}

; Testcase 1 MD:
; CHECK-DAG: [[COS_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[COS_MD]] = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)

; Testcase 2 MD:
; CHECK-DAG: [[SIN_LOC]] = !DILocation(line: 7
; CHECK-DAG: [[SIN_MD]] = !DILocalVariable(name: "6", scope: !10, file: !5, line: 7, type: !14)

declare float @__builtin_spirv_OpenCL_cos_f32(float)

declare float @__builtin_spirv_OpenCL_sin_f32(float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3}
!2 = !{!"MatchSinCosPi", i1 true}
!3 = !{!"FastRelaxedMath", i1 false}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "PreBIImport.ll", directory: "/")
!6 = !{}
!7 = !{i32 8}
!8 = !{i32 6}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_sincos", linkageName: "test_sincos", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !17, !19, !20}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !14)
!17 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !18)
!18 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 6, type: !14)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 7, type: !14)
!21 = !DILocation(line: 1, column: 1, scope: !10)
!22 = !DILocation(line: 2, column: 1, scope: !10)
!23 = !DILocation(line: 3, column: 1, scope: !10)
!24 = !DILocation(line: 4, column: 1, scope: !10)
!25 = !DILocation(line: 5, column: 1, scope: !10)
!26 = !DILocation(line: 6, column: 1, scope: !10)
!27 = !DILocation(line: 7, column: 1, scope: !10)
!28 = !DILocation(line: 8, column: 1, scope: !10)
