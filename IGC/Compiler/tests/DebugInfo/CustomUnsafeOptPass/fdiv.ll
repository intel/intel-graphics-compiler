;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
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
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; this value is unsalvageble
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK: void @llvm.dbg.value(metadata float [[VAL1_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: store float [[VAL1_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; this value is unsalvageble
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK: store float [[VAL5_V]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; this value is unsalvageble
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK: store float [[VAL7_V]]{{.*}}, !dbg [[STORE3_LOC:![0-9]*]]
; CHECK-DAG: store float [[VAL9_V:%[0-9]*]]{{.*}}, !dbg [[STORE4_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: [[VAL9_V]] = fadd float [[TEMP_VAL:%[A-z0-9]*]], {{.*}}, !dbg [[VAL9_LOC]]
; It would be more correct to have VAL9_LOC here:
; CHECK-DAG: [[TEMP_VAL]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]

define spir_kernel void @test_custom(float %a, float addrspace(65549)* %b) !dbg !9 {
entry:
  %0 = load float, float addrspace(65549)* %b, !dbg !22
  call void @llvm.dbg.value(metadata float %0, metadata !12, metadata !DIExpression()), !dbg !22
  %1 = fdiv float 1.000000e+00, %0, !dbg !23
  call void @llvm.dbg.value(metadata float %1, metadata !14, metadata !DIExpression()), !dbg !23
  %2 = fdiv float 1.000000e+00, %1, !dbg !24
  call void @llvm.dbg.value(metadata float %2, metadata !15, metadata !DIExpression()), !dbg !24
  store float %2, float addrspace(65549)* %b, !dbg !25
  %3 = fdiv float 1.000000e+00, %0, !dbg !26
  call void @llvm.dbg.value(metadata float %3, metadata !16, metadata !DIExpression()), !dbg !26
  %4 = fdiv float %a, %3, !dbg !27
  call void @llvm.dbg.value(metadata float %4, metadata !17, metadata !DIExpression()), !dbg !27
  store float %4, float addrspace(65549)* %b, !dbg !28
  %5 = call float @llvm.GenISA.rsq.f32(float %0), !dbg !29
  call void @llvm.dbg.value(metadata float %5, metadata !18, metadata !DIExpression()), !dbg !29
  %6 = fdiv float %a, %5, !dbg !30
  call void @llvm.dbg.value(metadata float %6, metadata !19, metadata !DIExpression()), !dbg !30
  store float %6, float addrspace(65549)* %b, !dbg !31
  %7 = fadd float %a, %0, !dbg !32
  call void @llvm.dbg.value(metadata float %7, metadata !20, metadata !DIExpression()), !dbg !32
  %8 = fdiv float %7, %0, !dbg !33
  call void @llvm.dbg.value(metadata float %8, metadata !21, metadata !DIExpression()), !dbg !33
  store float %8, float addrspace(65549)* %b, !dbg !34
  ret void, !dbg !35
}


; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fdiv.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

declare float @llvm.GenISA.rsq.f32(float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "fdiv.ll", directory: "/")
!5 = !{}
!6 = !{i32 14}
!7 = !{i32 9}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !16, !17, !18, !19, !20, !21}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 5, type: !13)
!17 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 6, type: !13)
!18 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 8, type: !13)
!19 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 9, type: !13)
!20 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 11, type: !13)
!21 = !DILocalVariable(name: "9", scope: !9, file: !4, line: 12, type: !13)
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
!32 = !DILocation(line: 11, column: 1, scope: !9)
!33 = !DILocation(line: 12, column: 1, scope: !9)
!34 = !DILocation(line: 13, column: 1, scope: !9)
!35 = !DILocation(line: 14, column: 1, scope: !9)
