;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-constant-coalescing -S < %s | FileCheck %s
; ------------------------------------------------
; ConstantCoalescing
; ------------------------------------------------
; This test checks that ConstantCoalescing pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Note: Vector value changes are not salvagable for now
;
; ------------------------------------------------


; CHECK: @test_constcoal{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: entry:
; CHECK-DAG: [[EXTR1_V:%[A-z0-9]*]] = extractelement {{.*}}[[LOAD1_V:%[A-z0-9]*]]{{.*}} !dbg [[EXTR1_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD1_V]] = call <2 x float> {{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR2_V:%[A-z0-9]*]] = extractelement {{.*}}[[LOAD2_V:%[A-z0-9]*]]{{.*}} !dbg [[EXTR2_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD2_V]] = call <2 x float> {{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR3_V:%[A-z0-9]*]] = extractelement {{.*}}[[LOAD2_V]]{{.*}} !dbg [[EXTR3_LOC:![0-9]*]]

define void @test_constcoal(i32 %a, float* %dst) !dbg !10 {
entry:
  %0 = inttoptr i32 16 to <2 x float> addrspace(2555904)*, !dbg !26
  call void @llvm.dbg.value(metadata <2 x float> addrspace(2555904)* %0, metadata !13, metadata !DIExpression()), !dbg !26
  %1 = call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2555904v2f32(<2 x float> addrspace(2555904)* %0, i32 4, i32 4, i1 false), !dbg !27
  call void @llvm.dbg.value(metadata <2 x float> %1, metadata !15, metadata !DIExpression()), !dbg !27
  br label %lbl1, !dbg !28

lbl1:                                             ; preds = %entry
  %2 = add i32 %a, 16, !dbg !29
  call void @llvm.dbg.value(metadata i32 %2, metadata !16, metadata !DIExpression()), !dbg !29
  %3 = call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2555904v2f32(<2 x float> addrspace(2555904)* %0, i32 %2, i32 4, i1 false), !dbg !30
  call void @llvm.dbg.value(metadata <2 x float> %3, metadata !18, metadata !DIExpression()), !dbg !30
  br label %lbl2, !dbg !31

lbl2:                                             ; preds = %lbl1
  %4 = or i32 %2, 24, !dbg !32
  call void @llvm.dbg.value(metadata i32 %4, metadata !19, metadata !DIExpression()), !dbg !32
  %5 = call <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2555904v2f32(<2 x float> addrspace(2555904)* %0, i32 %2, i32 4, i1 false), !dbg !33
  call void @llvm.dbg.value(metadata <2 x float> %5, metadata !20, metadata !DIExpression()), !dbg !33
  br label %end, !dbg !34

end:                                              ; preds = %lbl2
  %6 = extractelement <2 x float> %1, i32 1, !dbg !35
  call void @llvm.dbg.value(metadata float %6, metadata !21, metadata !DIExpression()), !dbg !35
  %7 = extractelement <2 x float> %3, i32 0, !dbg !36
  call void @llvm.dbg.value(metadata float %7, metadata !22, metadata !DIExpression()), !dbg !36
  %8 = extractelement <2 x float> %5, i32 1, !dbg !37
  call void @llvm.dbg.value(metadata float %8, metadata !23, metadata !DIExpression()), !dbg !37
  %9 = fadd float %6, %7, !dbg !38
  call void @llvm.dbg.value(metadata float %9, metadata !24, metadata !DIExpression()), !dbg !38
  %10 = fadd float %9, %8, !dbg !39
  call void @llvm.dbg.value(metadata float %10, metadata !25, metadata !DIExpression()), !dbg !39
  store float %10, float* %dst, !dbg !40
  ret void, !dbg !41
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "indirectcb_int.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_constcoal", linkageName: "test_constcoal", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR1_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR2_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR3_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

declare <2 x float> @llvm.genx.GenISA.ldrawvector.indexed.v2f32.p2555904v2f32(<2 x float> addrspace(2555904)*, i32, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32, float*)* @test_constcoal, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "indirectcb_int.ll", directory: "/")
!6 = !{}
!7 = !{i32 16}
!8 = !{i32 11}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_constcoal", linkageName: "test_constcoal", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !18, !19, !20, !21, !22, !23, !24, !25}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 4, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 5, type: !14)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 7, type: !17)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 8, type: !14)
!21 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 10, type: !17)
!22 = !DILocalVariable(name: "8", scope: !10, file: !5, line: 11, type: !17)
!23 = !DILocalVariable(name: "9", scope: !10, file: !5, line: 12, type: !17)
!24 = !DILocalVariable(name: "10", scope: !10, file: !5, line: 13, type: !17)
!25 = !DILocalVariable(name: "11", scope: !10, file: !5, line: 14, type: !17)
!26 = !DILocation(line: 1, column: 1, scope: !10)
!27 = !DILocation(line: 2, column: 1, scope: !10)
!28 = !DILocation(line: 3, column: 1, scope: !10)
!29 = !DILocation(line: 4, column: 1, scope: !10)
!30 = !DILocation(line: 5, column: 1, scope: !10)
!31 = !DILocation(line: 6, column: 1, scope: !10)
!32 = !DILocation(line: 7, column: 1, scope: !10)
!33 = !DILocation(line: 8, column: 1, scope: !10)
!34 = !DILocation(line: 9, column: 1, scope: !10)
!35 = !DILocation(line: 10, column: 1, scope: !10)
!36 = !DILocation(line: 11, column: 1, scope: !10)
!37 = !DILocation(line: 12, column: 1, scope: !10)
!38 = !DILocation(line: 13, column: 1, scope: !10)
!39 = !DILocation(line: 14, column: 1, scope: !10)
!40 = !DILocation(line: 15, column: 1, scope: !10)
!41 = !DILocation(line: 16, column: 1, scope: !10)
