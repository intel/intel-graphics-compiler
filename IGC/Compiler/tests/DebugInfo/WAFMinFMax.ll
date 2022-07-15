;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-wa-fminmax -enable-fmax-fmin-plus-zero -retain-denormals -S < %s | FileCheck %s
; ------------------------------------------------
; WAFMinFMax
; ------------------------------------------------
; This test checks that WAFMinFMax pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_fmin{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata float [[MIN_V:%[0-9A-z]*]], metadata [[MIN_MD:![0-9]*]], metadata !DIExpression()), !dbg [[MIN_LOC:![0-9]*]]
; CHECK-DAG: [[MIN_V]] = {{.*}} !dbg [[MIN_LOC]]

define void @test_fmin(float %src1, float %src2) !dbg !6 {
  %1 = call float @llvm.minnum.f32(float %src1, float %src2), !dbg !11
  call void @llvm.dbg.value(metadata float %1, metadata !9, metadata !DIExpression()), !dbg !11
  ret void, !dbg !12
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "WAFMinFMax.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_fmin", linkageName: "test_fmin", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[MIN_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[MIN_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
;

; Function Attrs: nounwind readnone speculatable
declare float @llvm.minnum.f32(float, float) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "WAFMinFMax.ll", directory: "/")
!2 = !{}
!3 = !{i32 2}
!4 = !{i32 1}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_fmin", linkageName: "test_fmin", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
