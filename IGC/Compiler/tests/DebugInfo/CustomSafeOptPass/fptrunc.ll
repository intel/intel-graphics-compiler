;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass
; ------------------------------------------------
; This test checks that CustomSafeOptPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_customsafe{{.*}} !dbg [[SCOPE:![0-9]*]]
; extended values in entry and bb1 blocks are lost, checking resulting phi and fpext
; CHECK: end:
; CHECK-DAG: void @llvm.dbg.value(metadata double [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]

define spir_kernel void @test_customsafe(float %a, double* %b, i1 %c, float* %d) !dbg !6 {
entry:
  %0 = fadd float %a, 1.000000e+00, !dbg !17
  call void @llvm.dbg.value(metadata float %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = fpext float %0 to double, !dbg !18
  call void @llvm.dbg.value(metadata double %1, metadata !11, metadata !DIExpression()), !dbg !18
  br i1 %c, label %end, label %bb1, !dbg !19

bb1:                                              ; preds = %entry
  %2 = fadd float %a, 2.000000e+00, !dbg !20
  call void @llvm.dbg.value(metadata float %2, metadata !13, metadata !DIExpression()), !dbg !20
  %3 = fpext float %2 to double, !dbg !21
  call void @llvm.dbg.value(metadata double %3, metadata !14, metadata !DIExpression()), !dbg !21
  br label %end, !dbg !22

end:                                              ; preds = %bb1, %entry
  %4 = phi double [ %1, %entry ], [ %3, %bb1 ], !dbg !23
  call void @llvm.dbg.value(metadata double %4, metadata !15, metadata !DIExpression()), !dbg !23
  %5 = fptrunc double %4 to float, !dbg !24
  call void @llvm.dbg.value(metadata float %5, metadata !16, metadata !DIExpression()), !dbg !24
  store float %5, float* %d, !dbg !25
  store double %4, double* %b, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fptrunc.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "fptrunc.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !12)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 8, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
!27 = !DILocation(line: 11, column: 1, scope: !6)
