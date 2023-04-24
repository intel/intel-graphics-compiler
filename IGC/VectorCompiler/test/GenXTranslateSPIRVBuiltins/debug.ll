;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------
; This test checks that GenXTranslateSPIRVBuiltins pass follows
; 'How to Update Debug Info' llvm guideline.

; CHECK: spir_func <16 x double> @translate{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <16 x double> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: store i64{{.*}} !dbg [[STR1_LOC:![0-9]*]]

declare spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double>)

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

define spir_func <16 x double> @translate(<16 x double> %arg, i64* %dst) !dbg !6 {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2), !dbg !13
  call void @llvm.dbg.value(metadata i64 %1, metadata !9, metadata !DIExpression()), !dbg !13
  %2 = call spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double> %arg), !dbg !14
  call void @llvm.dbg.value(metadata <16 x double> %2, metadata !11, metadata !DIExpression()), !dbg !14
  store i64 %1, i64* %dst, !dbg !15
  ret <16 x double> %2, !dbg !16
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXTranslateSPIRVBuiltins.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "translate", linkageName: "translate", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXTranslateSPIRVBuiltins.ll", directory: "/")
!2 = !{}
!3 = !{i32 4}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "translate", linkageName: "translate", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty1024", size: 1024, encoding: DW_ATE_unsigned)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
