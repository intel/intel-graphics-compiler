;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-unsafe-opt-pass -S < %s | FileCheck %s
; ------------------------------------------------
; CustomUnsafeOptPass
; ------------------------------------------------
; This test checks that CustomUnsafeOptPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_custom{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: void @llvm.dbg.value(metadata i1 false, metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 true, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK: [[VAL6_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL11_V:%[A-z0-9]*]] = fcmp one {{.*}}, !dbg [[VAL11_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL11_V]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC]]
; CHECK: [[VAL14_V:%[A-z0-9]*]] = fcmp uge {{.*}}, !dbg [[VAL14_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL14_V]], metadata [[VAL14_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL14_LOC]]

define spir_kernel void @test_custom(float %a, float %b, float* %d) !dbg !9 {
entry:
  %0 = fcmp uno float %a, %b, !dbg !30
  call void @llvm.dbg.value(metadata i1 %0, metadata !12, metadata !DIExpression()), !dbg !30
  %1 = fcmp ord float %a, %b, !dbg !31
  call void @llvm.dbg.value(metadata i1 %1, metadata !14, metadata !DIExpression()), !dbg !31
  %2 = fcmp ole float %a, 0.000000e+00, !dbg !32
  call void @llvm.dbg.value(metadata i1 %2, metadata !15, metadata !DIExpression()), !dbg !32
  %3 = select i1 %2, float 0.000000e+00, float 1.000000e+00, !dbg !33
  call void @llvm.dbg.value(metadata float %3, metadata !16, metadata !DIExpression()), !dbg !33
  %4 = fsub float -0.000000e+00, %3, !dbg !34
  call void @llvm.dbg.value(metadata float %4, metadata !18, metadata !DIExpression()), !dbg !34
  %5 = fcmp ueq float %3, %4, !dbg !35
  call void @llvm.dbg.value(metadata i1 %5, metadata !19, metadata !DIExpression()), !dbg !35
  %6 = or i1 %0, %5, !dbg !36
  call void @llvm.dbg.value(metadata i1 %6, metadata !20, metadata !DIExpression()), !dbg !36
  %7 = xor i1 %6, %1, !dbg !37
  call void @llvm.dbg.value(metadata i1 %7, metadata !21, metadata !DIExpression()), !dbg !37
  %8 = fmul float %a, %a, !dbg !38
  call void @llvm.dbg.value(metadata float %8, metadata !22, metadata !DIExpression()), !dbg !38
  %9 = fsub float 0.000000e+00, %8, !dbg !39
  call void @llvm.dbg.value(metadata float %9, metadata !23, metadata !DIExpression()), !dbg !39
  %10 = fcmp ogt float %8, %9, !dbg !40
  call void @llvm.dbg.value(metadata i1 %10, metadata !24, metadata !DIExpression()), !dbg !40
  %11 = or i1 %10, %7, !dbg !41
  call void @llvm.dbg.value(metadata i1 %11, metadata !25, metadata !DIExpression()), !dbg !41
  %12 = fadd float %a, 1.545000e+02, !dbg !42
  call void @llvm.dbg.value(metadata float %12, metadata !26, metadata !DIExpression()), !dbg !42
  %13 = fcmp uge float %12, 0.000000e+00, !dbg !43
  call void @llvm.dbg.value(metadata i1 %13, metadata !27, metadata !DIExpression()), !dbg !43
  %14 = xor i1 %11, %13, !dbg !44
  call void @llvm.dbg.value(metadata i1 %14, metadata !28, metadata !DIExpression()), !dbg !44
  %15 = select i1 %14, float 1.300000e+01, float 1.500000e+01, !dbg !45
  call void @llvm.dbg.value(metadata float %15, metadata !29, metadata !DIExpression()), !dbg !45
  store float %15, float* %d, !dbg !46
  ret void, !dbg !47
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fcmp.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL14_MD]] = !DILocalVariable(name: "14", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL14_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare float @llvm.floor.f32(float) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"NoNaNs", i1 true}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "fcmp.ll", directory: "/")
!5 = !{}
!6 = !{i32 18}
!7 = !{i32 16}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !16, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !17)
!19 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 6, type: !13)
!20 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 7, type: !13)
!21 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 8, type: !13)
!22 = !DILocalVariable(name: "9", scope: !9, file: !4, line: 9, type: !17)
!23 = !DILocalVariable(name: "10", scope: !9, file: !4, line: 10, type: !17)
!24 = !DILocalVariable(name: "11", scope: !9, file: !4, line: 11, type: !13)
!25 = !DILocalVariable(name: "12", scope: !9, file: !4, line: 12, type: !13)
!26 = !DILocalVariable(name: "13", scope: !9, file: !4, line: 13, type: !17)
!27 = !DILocalVariable(name: "14", scope: !9, file: !4, line: 14, type: !13)
!28 = !DILocalVariable(name: "15", scope: !9, file: !4, line: 15, type: !13)
!29 = !DILocalVariable(name: "16", scope: !9, file: !4, line: 16, type: !17)
!30 = !DILocation(line: 1, column: 1, scope: !9)
!31 = !DILocation(line: 2, column: 1, scope: !9)
!32 = !DILocation(line: 3, column: 1, scope: !9)
!33 = !DILocation(line: 4, column: 1, scope: !9)
!34 = !DILocation(line: 5, column: 1, scope: !9)
!35 = !DILocation(line: 6, column: 1, scope: !9)
!36 = !DILocation(line: 7, column: 1, scope: !9)
!37 = !DILocation(line: 8, column: 1, scope: !9)
!38 = !DILocation(line: 9, column: 1, scope: !9)
!39 = !DILocation(line: 10, column: 1, scope: !9)
!40 = !DILocation(line: 11, column: 1, scope: !9)
!41 = !DILocation(line: 12, column: 1, scope: !9)
!42 = !DILocation(line: 13, column: 1, scope: !9)
!43 = !DILocation(line: 14, column: 1, scope: !9)
!44 = !DILocation(line: 15, column: 1, scope: !9)
!45 = !DILocation(line: 16, column: 1, scope: !9)
!46 = !DILocation(line: 17, column: 1, scope: !9)
!47 = !DILocation(line: 18, column: 1, scope: !9)
