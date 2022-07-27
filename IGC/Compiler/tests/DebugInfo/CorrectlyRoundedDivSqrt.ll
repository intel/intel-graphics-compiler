;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-correctly-rounded-div-sqrt -S < %s | FileCheck %s --check-prefixes=%SPV_CHECK_PREFIX%,CHECK

; ------------------------------------------------
; CorrectlyRoundedDivSqrt
; ------------------------------------------------
; This test checks that CorrectlyRoundedDivSqrt pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'CorrectlyRoundedDivSqrt.ll'
source_filename = "CorrectlyRoundedDivSqrt.ll"

define spir_kernel void @test_fdiv(float %src1, float %src2) !dbg !9 {
; Testcase 1
; Check that scalar fdiv is substituted by call
;
; CHECK: [[FDIV_V:%[0-9]*]] = call float @__builtin_spirv_divide_cr_f32_f32({{.*}} !dbg [[FDIV_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] float [[FDIV_V]],  metadata [[FDIV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FDIV_LOC]]
  %1 = fdiv float %src1, %src2, !dbg !20
  call void @llvm.dbg.value(metadata float %1, metadata !12, metadata !DIExpression()), !dbg !20
  %2 = insertelement <2 x float> <float 3.000000e+00, float 2.000000e+00>, float %src1, i32 0, !dbg !21
  call void @llvm.dbg.value(metadata <2 x float> %2, metadata !14, metadata !DIExpression()), !dbg !21
  %3 = insertelement <2 x float> <float 3.000000e+00, float 2.000000e+00>, float %src2, i32 2, !dbg !22
  call void @llvm.dbg.value(metadata <2 x float> %3, metadata !16, metadata !DIExpression()), !dbg !22
; Testcase 2
; Check that vector fdiv is substituted by call and insertelement value is correct
;
; CHECK: [[FDIV_VEC_V:%[0-9]*]] = call float @__builtin_spirv_divide_cr_f32_f32({{.*}} !dbg [[FDIV_VEC_LOC:![0-9]*]]
; CHECK-NEXT: [[INSELM0_V:%[0-9]*]] = insertelement <2 x float> undef, float [[FDIV_VEC_V]], i64 0, !dbg [[FDIV_VEC_LOC]]
; CHECK: [[FDIV_VEC_V:%[0-9]*]] = call float @__builtin_spirv_divide_cr_f32_f32({{.*}} !dbg [[FDIV_VEC_LOC]]
; CHECK-NEXT: [[INSELM_V:%[0-9]*]] = insertelement <2 x float> [[INSELM0_V]], float [[FDIV_VEC_V]], i64 1, !dbg [[FDIV_VEC_LOC]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <2 x float> [[INSELM_V]],  metadata [[FDIV_VEC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FDIV_VEC_LOC]]
  %4 = fdiv <2 x float> %2, %3, !dbg !23
  call void @llvm.dbg.value(metadata <2 x float> %4, metadata !17, metadata !DIExpression()), !dbg !23
; Testcase 3
; Check that sqrt calls are renamed
;
; CHECK-LEGACY: call float @__builtin_spirv_OpenCL_sqrt_cr_f32({{.*}} !dbg [[FSQRT0_LOC:![0-9]*]]
; CHECK-KHR: call float @_Z19__spirv_ocl_sqrt_cr_f32({{.*}} !dbg [[FSQRT0_LOC:![0-9]*]]
; CHECK: call float @_Z7sqrt_cr_f32({{.*}} !dbg [[FSQRT1_LOC:![0-9]*]]
  %5 = call float @__builtin_spirv_OpenCL_sqrt_f32(float %src1), !dbg !24
  call void @llvm.dbg.value(metadata float %5, metadata !18, metadata !DIExpression()), !dbg !24
  %6 = call float @_Z16__spirv_ocl_sqrt_f32(float %src1), !dbg !24
  call void @llvm.dbg.value(metadata float %6, metadata !18, metadata !DIExpression()), !dbg !24
  %7 = call float @_Z4sqrt_f32(float %src2), !dbg !25
  call void @llvm.dbg.value(metadata float %7, metadata !19, metadata !DIExpression()), !dbg !25
  ret void, !dbg !26
}

; Testcase 1 MD:
; CHECK-DAG: [[FDIV_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[FDIV_MD]] = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)

; Testcase 2 MD:
; CHECK-DAG: [[FDIV_VEC_LOC]] = !DILocation(line: 4
; CHECK-DAG: [[FDIV_VEC_MD]] = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !15)

; Testcase 3 MD:
; CHECK-LEGACY-DAG: [[FSQRT0_LOC]] = !DILocation(line: 5
; CHECK-KHR-DAG: [[FSQRT0_LOC]] = !DILocation(line: 5
; CHECK-DAG: [[FSQRT1_LOC]] = !DILocation(line: 6

declare spir_func float @__builtin_spirv_OpenCL_sqrt_f32(float)
declare spir_func float @_Z16__spirv_ocl_sqrt_f32(float)
declare spir_func float @_Z4sqrt_f32(float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"CorrectlyRoundedDivSqrt", i1 true}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "CorrectlyRoundedDivSqrt.ll", directory: "/")
!5 = !{}
!6 = !{i32 7}
!7 = !{i32 6}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_fdiv", linkageName: "test_fdiv", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !16, !17, !18, !19}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !15)
!15 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !15)
!17 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !15)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !13)
!19 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 6, type: !13)
!20 = !DILocation(line: 1, column: 1, scope: !9)
!21 = !DILocation(line: 2, column: 1, scope: !9)
!22 = !DILocation(line: 3, column: 1, scope: !9)
!23 = !DILocation(line: 4, column: 1, scope: !9)
!24 = !DILocation(line: 5, column: 1, scope: !9)
!25 = !DILocation(line: 6, column: 1, scope: !9)
!26 = !DILocation(line: 7, column: 1, scope: !9)
