;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXImportOCLBiF -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXImportOCLBiF -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
;
; RUN: %opt_new_pm_typed -passes=GenXImportOCLBiF -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_new_pm_opaque -passes=GenXImportOCLBiF -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
;
; ------------------------------------------------
; GenXImportOCLBiF
; ------------------------------------------------
; This test checks that GenXImportOCLBiF pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; Add bifs to script/assets

; CHECK: define dllexport void @test_kernel{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata double [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata double [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata double [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata double [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata double [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK: call spir_func void {{.*}}, !dbg [[CALL_LOC:![0-9]*]]

define dllexport void @test_kernel(double* %a) !dbg !6 {
  %1 = load double, double* %a, !dbg !15
  call void @llvm.dbg.value(metadata double %1, metadata !9, metadata !DIExpression()), !dbg !15
  %2 = call spir_func double @_Z3expd(double %1), !dbg !16
  call void @llvm.dbg.value(metadata double %2, metadata !11, metadata !DIExpression()), !dbg !16
  store double %2, double* %a, !dbg !17
  %3 = call spir_func double @_Z3fmaddd(double %1, double %1, double %1), !dbg !18
  call void @llvm.dbg.value(metadata double %3, metadata !12, metadata !DIExpression()), !dbg !18
  %4 = call double @__builtin_IB_ieee_sqrt(double %3), !dbg !19
  call void @llvm.dbg.value(metadata double %4, metadata !13, metadata !DIExpression()), !dbg !19
  %5 = call double @__builtin_IB_fma_rtz_f64(double %4, double %4, double %4), !dbg !20
  call void @llvm.dbg.value(metadata double %5, metadata !14, metadata !DIExpression()), !dbg !20
  call spir_func void @_Z18intel_source_valueh(i8 3), !dbg !21
  ret void, !dbg !22
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXImportOCLBiF.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

declare spir_func double @_Z3expd(double)

declare spir_func double @_Z3fmaddd(double, double, double)

declare double @__builtin_IB_ieee_sqrt(double)

declare double @__builtin_IB_fma_rtz_f64(double, double, double)

declare spir_func void @_Z18intel_source_valueh(i8)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXImportOCLBiF.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
!20 = !DILocation(line: 6, column: 1, scope: !6)
!21 = !DILocation(line: 7, column: 1, scope: !6)
!22 = !DILocation(line: 8, column: 1, scope: !6)
