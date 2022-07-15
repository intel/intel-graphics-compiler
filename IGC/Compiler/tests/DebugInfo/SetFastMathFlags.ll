;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-set-fast-math-flags -S < %s | FileCheck %s
; ------------------------------------------------
; SetFastMathFlags
; ------------------------------------------------
; This test checks that SetFastMathFlags pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_fast
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK: [[FADD_V:%[0-9]*]] = fadd
; CHECK-SAME: !dbg [[FADD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[FADD_V]]
; CHECK-SAME: metadata [[FADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FADD_LOC]]
;
; CHECK: [[FSUB_V:%[0-9]*]] = fsub
; CHECK-SAME: !dbg [[FSUB_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[FSUB_V]]
; CHECK-SAME: metadata [[FSUB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FSUB_LOC]]
;
; CHECK: [[SQRT_V:%[0-9]*]] = call
; CHECK-SAME: !dbg [[SQRT_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float [[SQRT_V]]
; CHECK-SAME: metadata [[SQRT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SQRT_LOC]]

define void @test_fast(float %src1, float %src2) !dbg !9 {
  %1 = fadd float %src1, %src2, !dbg !16
  call void @llvm.dbg.value(metadata float %1, metadata !12, metadata !DIExpression()), !dbg !16
  %2 = fsub float %src1, %src2, !dbg !17
  call void @llvm.dbg.value(metadata float %2, metadata !14, metadata !DIExpression()), !dbg !17
  %3 = call float @llvm.sqrt.f32(float %src1), !dbg !18
  call void @llvm.dbg.value(metadata float %3, metadata !15, metadata !DIExpression()), !dbg !18
  ret void, !dbg !19
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "SetFastMathFlags.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_fast", linkageName: "test_fast", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[FADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[FADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FSUB_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[FSUB_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SQRT_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[SQRT_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare float @llvm.sqrt.f32(float) #0

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
!4 = !DIFile(filename: "SetFastMathFlags.ll", directory: "/")
!5 = !{}
!6 = !{i32 4}
!7 = !{i32 3}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_fast", linkageName: "test_fast", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocation(line: 1, column: 1, scope: !9)
!17 = !DILocation(line: 2, column: 1, scope: !9)
!18 = !DILocation(line: 3, column: 1, scope: !9)
!19 = !DILocation(line: 4, column: 1, scope: !9)
