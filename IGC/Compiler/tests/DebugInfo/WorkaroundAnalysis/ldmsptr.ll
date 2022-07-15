;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-workaround -S < %s | FileCheck %s
; ------------------------------------------------
; WorkaroundAnalysis : Ldmsptr wa
; ------------------------------------------------
; This test checks that WorkaroundAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK:  spir_kernel void @test_wa
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ST_V:%[A-z0-9]*]] = alloca
; CHECK: @llvm.dbg.declare(metadata {{.*}} [[ST_V]], metadata [[ST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ST_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}} [[CALL_V:%[A-z0-9]*]], metadata [[CALL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL_LOC:![0-9]*]]
; CHECK-DAG: [[CALL_V]] = {{.*}}, !dbg [[CALL_LOC]]

define spir_kernel void @test_wa(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g, i8* %s) !dbg !6 {
  %st = alloca <4 x float>, align 4, !dbg !13
  call void @llvm.dbg.declare(metadata <4 x float>* %st, metadata !9, metadata !DIExpression()), !dbg !13
  %1 = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.f32(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g, i8* %s, i32 1, i32 2, i32 3), !dbg !14
  call void @llvm.dbg.value(metadata <4 x float> %1, metadata !11, metadata !DIExpression()), !dbg !14
  store <4 x float> %1, <4 x float>* %st, !dbg !15
  ret void, !dbg !16
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ldmsptr.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_wa", linkageName: "test_wa", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])

declare <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.f32(i32, i32, i32, i32, i32, i32, i32, i8*, i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ldmsptr.ll", directory: "/")
!2 = !{}
!3 = !{i32 4}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_wa", linkageName: "test_wa", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
