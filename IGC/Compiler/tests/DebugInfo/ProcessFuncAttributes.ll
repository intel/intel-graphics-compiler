;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-process-func-attributes -S < %s | FileCheck %s
; ------------------------------------------------
; ProcessFuncAttributes
; ------------------------------------------------
; This test checks that ProcessFuncAttributes pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; This pass updates fuction attributes, check that nothing debug related is modified.

; CHECK: @test_prfuncattr{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: call void @foo{{.*}} !dbg [[CALLF_LOC:![0-9]*]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = load i1{{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i1 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: lbl:
; CHECK: store i32{{.*}} !dbg [[STORE1_LOC:![0-9]*]]

; CHECK: @foo{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: [[CALLA_V:%[A-z0-9]*]] = call i8*{{.*}} !dbg [[CALLA_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i8* [[CALLA_V]], metadata [[CALLA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALLA_LOC]]
; CHECK: [[TRUNC_V:%[A-z0-9]*]] = trunc i32{{.*}} !dbg [[TRUNC_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i8 [[TRUNC_V]], metadata [[TRUNC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TRUNC_LOC]]
; CHECK: store i8{{.*}} !dbg [[STORE2_LOC:![0-9]*]]

@__FastRelaxedMath = constant i1 true

define void @test_prfuncattr(i32* %src, i32* %dst) !dbg !6 {
entry:
  %0 = load i32, i32* %src, !dbg !13
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !13
  call void @foo(i32 %0), !dbg !14
  %1 = load i1, i1* @__FastRelaxedMath, !dbg !15
  call void @llvm.dbg.value(metadata i1 %1, metadata !11, metadata !DIExpression()), !dbg !15
  br i1 %1, label %lbl, label %end, !dbg !16

lbl:                                              ; preds = %entry
  store i32 %0, i32* %dst, !dbg !17
  br label %end, !dbg !18

end:                                              ; preds = %lbl, %entry
  ret void, !dbg !19
}

define void @foo(i32 %src1) !dbg !20 {
  %1 = call i8* @__builtin_IB_AllocLocalMemPool(i1 true, i32 16, i32 20), !dbg !25
  call void @llvm.dbg.value(metadata i8* %1, metadata !22, metadata !DIExpression()), !dbg !25
  %2 = trunc i32 %src1 to i8, !dbg !26
  call void @llvm.dbg.value(metadata i8 %2, metadata !24, metadata !DIExpression()), !dbg !26
  store i8 %2, i8* %1, !dbg !27
  ret void, !dbg !28
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ProcessFuncAttributes.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_prfuncattr", linkageName: "test_prfuncattr", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALLF_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: [[FILE]], line: 8
; CHECK-DAG: [[CALLA_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE2]], file: [[FILE]], line: 8
; CHECK-DAG: [[CALLA_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[TRUNC_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE2]], file: [[FILE]], line: 9
; CHECK-DAG: [[TRUNC_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE2]])


declare i8* @__builtin_IB_AllocLocalMemPool(i1, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ProcessFuncAttributes.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_prfuncattr", linkageName: "test_prfuncattr", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !12)
!12 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = !DILocation(line: 5, column: 1, scope: !6)
!18 = !DILocation(line: 6, column: 1, scope: !6)
!19 = !DILocation(line: 7, column: 1, scope: !6)
!20 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 8, type: !7, scopeLine: 8, unit: !0, retainedNodes: !21)
!21 = !{!22, !24}
!22 = !DILocalVariable(name: "3", scope: !20, file: !1, line: 8, type: !23)
!23 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "4", scope: !20, file: !1, line: 9, type: !12)
!25 = !DILocation(line: 8, column: 1, scope: !20)
!26 = !DILocation(line: 9, column: 1, scope: !20)
!27 = !DILocation(line: 10, column: 1, scope: !20)
!28 = !DILocation(line: 11, column: 1, scope: !20)
