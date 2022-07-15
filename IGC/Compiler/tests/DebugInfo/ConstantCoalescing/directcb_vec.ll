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
; CHECK-DAG: [[LOAD1_V]] = load {{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR2_V:%[A-z0-9]*]] = extractelement {{.*}}[[LOAD2_V:%[A-z0-9]*]]{{.*}} !dbg [[EXTR2_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD2_V]] = load {{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR3_V:%[A-z0-9]*]] = extractelement {{.*}}[[LOAD3_V:%[A-z0-9]*]]{{.*}} !dbg [[EXTR3_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD3_V]] = load {{.*}} !dbg [[LOAD3_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR4_V:%[A-z0-9]*]] = extractelement {{.*}}[[LOAD4_V:%[A-z0-9]*]]{{.*}} !dbg [[EXTR4_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD4_V]] = load {{.*}} !dbg [[LOAD4_LOC:![0-9]*]]

define void @test_constcoal(i32 %a, float* %dst) !dbg !10 {
entry:
  %0 = load <2 x float>, <2 x float> addrspace(65537)* null, align 4, !dbg !30
  call void @llvm.dbg.value(metadata <2 x float> %0, metadata !13, metadata !DIExpression()), !dbg !30
  %1 = load <2 x float>, <2 x float> addrspace(65536)* null, align 4, !dbg !31
  call void @llvm.dbg.value(metadata <2 x float> %1, metadata !15, metadata !DIExpression()), !dbg !31
  br label %lbl1, !dbg !32

lbl1:                                             ; preds = %entry
  %2 = add i32 15, 16, !dbg !33
  call void @llvm.dbg.value(metadata i32 %2, metadata !16, metadata !DIExpression()), !dbg !33
  %3 = inttoptr i32 %2 to <2 x float> addrspace(65536)*, !dbg !34
  call void @llvm.dbg.value(metadata <2 x float> addrspace(65536)* %3, metadata !18, metadata !DIExpression()), !dbg !34
  %4 = load <2 x float>, <2 x float> addrspace(65536)* %3, align 4, !dbg !35
  call void @llvm.dbg.value(metadata <2 x float> %4, metadata !19, metadata !DIExpression()), !dbg !35
  br label %lbl2, !dbg !36

lbl2:                                             ; preds = %lbl1
  %5 = or i32 %2, 24, !dbg !37
  call void @llvm.dbg.value(metadata i32 %5, metadata !20, metadata !DIExpression()), !dbg !37
  %6 = inttoptr i32 %5 to <2 x float> addrspace(65536)*, !dbg !38
  call void @llvm.dbg.value(metadata <2 x float> addrspace(65536)* %6, metadata !21, metadata !DIExpression()), !dbg !38
  %7 = load <2 x float>, <2 x float> addrspace(65536)* %6, align 4, !dbg !39
  call void @llvm.dbg.value(metadata <2 x float> %7, metadata !22, metadata !DIExpression()), !dbg !39
  br label %end, !dbg !40

end:                                              ; preds = %lbl2
  %8 = extractelement <2 x float> %0, i32 1, !dbg !41
  call void @llvm.dbg.value(metadata float %8, metadata !23, metadata !DIExpression()), !dbg !41
  %9 = extractelement <2 x float> %1, i32 0, !dbg !42
  call void @llvm.dbg.value(metadata float %9, metadata !24, metadata !DIExpression()), !dbg !42
  %10 = extractelement <2 x float> %4, i32 1, !dbg !43
  call void @llvm.dbg.value(metadata float %10, metadata !25, metadata !DIExpression()), !dbg !43
  %11 = fadd float %8, %9, !dbg !44
  call void @llvm.dbg.value(metadata float %11, metadata !26, metadata !DIExpression()), !dbg !44
  %12 = fadd float %11, %10, !dbg !45
  call void @llvm.dbg.value(metadata float %12, metadata !27, metadata !DIExpression()), !dbg !45
  %13 = extractelement <2 x float> %7, i32 0, !dbg !46
  call void @llvm.dbg.value(metadata float %13, metadata !28, metadata !DIExpression()), !dbg !46
  %14 = fadd float %13, %12, !dbg !47
  call void @llvm.dbg.value(metadata float %14, metadata !29, metadata !DIExpression()), !dbg !47
  store float %14, float* %dst, !dbg !48
  ret void, !dbg !49
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "directcb_vec.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_constcoal", linkageName: "test_constcoal", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD3_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD4_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR1_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR2_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR3_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR4_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])

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
!5 = !DIFile(filename: "directcb_vec.ll", directory: "/")
!6 = !{}
!7 = !{i32 20}
!8 = !{i32 15}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_constcoal", linkageName: "test_constcoal", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 4, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 5, type: !14)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 6, type: !14)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 8, type: !17)
!21 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 9, type: !14)
!22 = !DILocalVariable(name: "8", scope: !10, file: !5, line: 10, type: !14)
!23 = !DILocalVariable(name: "9", scope: !10, file: !5, line: 12, type: !17)
!24 = !DILocalVariable(name: "10", scope: !10, file: !5, line: 13, type: !17)
!25 = !DILocalVariable(name: "11", scope: !10, file: !5, line: 14, type: !17)
!26 = !DILocalVariable(name: "12", scope: !10, file: !5, line: 15, type: !17)
!27 = !DILocalVariable(name: "13", scope: !10, file: !5, line: 16, type: !17)
!28 = !DILocalVariable(name: "14", scope: !10, file: !5, line: 17, type: !17)
!29 = !DILocalVariable(name: "15", scope: !10, file: !5, line: 18, type: !17)
!30 = !DILocation(line: 1, column: 1, scope: !10)
!31 = !DILocation(line: 2, column: 1, scope: !10)
!32 = !DILocation(line: 3, column: 1, scope: !10)
!33 = !DILocation(line: 4, column: 1, scope: !10)
!34 = !DILocation(line: 5, column: 1, scope: !10)
!35 = !DILocation(line: 6, column: 1, scope: !10)
!36 = !DILocation(line: 7, column: 1, scope: !10)
!37 = !DILocation(line: 8, column: 1, scope: !10)
!38 = !DILocation(line: 9, column: 1, scope: !10)
!39 = !DILocation(line: 10, column: 1, scope: !10)
!40 = !DILocation(line: 11, column: 1, scope: !10)
!41 = !DILocation(line: 12, column: 1, scope: !10)
!42 = !DILocation(line: 13, column: 1, scope: !10)
!43 = !DILocation(line: 14, column: 1, scope: !10)
!44 = !DILocation(line: 15, column: 1, scope: !10)
!45 = !DILocation(line: 16, column: 1, scope: !10)
!46 = !DILocation(line: 17, column: 1, scope: !10)
!47 = !DILocation(line: 18, column: 1, scope: !10)
!48 = !DILocation(line: 19, column: 1, scope: !10)
!49 = !DILocation(line: 20, column: 1, scope: !10)
