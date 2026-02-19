;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --EstimateFunctionSize -S < %s | FileCheck %s
; ------------------------------------------------
; EstimateFunctionSize
; ------------------------------------------------
; This test checks that EstimateFunctionSize pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_estimate{{.*}}!dbg [[SCOPEK:![0-9]*]]
; CHECK: entry:
; CHECK: [[ALLOCA_V:%[0-9]*]] = {{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32* [[ALLOCA_V]]
; CHECK-SAME: metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: [[CALLF_V:%[0-9]*]] = {{.*}} !dbg [[CALLF_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[CALLF_V]]
; CHECK-SAME: metadata [[CALLF_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALLF_LOC]]
; CHECK: end:
; CHECK: [[CALLB_V:%[0-9]*]] = {{.*}} !dbg [[CALLB_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[CALLB_V]]
; CHECK-SAME: metadata [[CALLB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALLB_LOC]]
; CHECK: [[ADD_V:%[0-9]*]] = {{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[ADD_V]]
; CHECK-SAME: metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]

define spir_kernel void @test_estimate(i32* %s) !dbg !6 {
entry:
  %0 = alloca i32, align 4, !dbg !15
  call void @llvm.dbg.value(metadata i32* %0, metadata !9, metadata !DIExpression()), !dbg !15
  %1 = call spir_func i32 @foo(i32* %s), !dbg !16
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !16
  br label %end, !dbg !17

end:                                              ; preds = %entry
  %2 = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId(), !dbg !18
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !18
  %3 = add i32 %2, %1, !dbg !19
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !19
  store i32 %3, i32* %0, !dbg !20
  ret void, !dbg !21
}

; CHECK: define spir_func i32 @foo{{.*}}!dbg [[SCOPEF:![0-9]*]]
; CHECK: entry:
; CHECK: [[LOAD_V:%[0-9]*]] = {{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[LOAD_V]]
; CHECK-SAME: metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: [[PTRTOI_V:%[0-9]*]] = {{.*}} !dbg [[PTRTOI_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[PTRTOI_V]]
; CHECK-SAME: metadata [[PTRTOI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTRTOI_LOC]]
; CHECK: [[CMP_V:%[0-9]*]] = {{.*}} !dbg [[CMP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i1 [[CMP_V]]
; CHECK-SAME: metadata [[CMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CMP_LOC]]
; CHECK: continue:
; CHECK: [[ITOPTR_V:%[0-9]*]] = {{.*}} !dbg [[ITOPTR_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32* [[ITOPTR_V]]
; CHECK-SAME: metadata [[ITOPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ITOPTR_LOC]]
; CHECK: [[CALLF2_V:%[0-9]*]] = {{.*}} !dbg [[CALLF2_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value(metadata i32 [[CALLF2_V]]
; CHECK-SAME: metadata [[CALLF2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALLF2_LOC]]

define spir_func i32 @foo(i32* %a) !dbg !22 {
entry:
  %0 = load i32, i32* %a, !dbg !31
  call void @llvm.dbg.value(metadata i32 %0, metadata !24, metadata !DIExpression()), !dbg !31
  %1 = ptrtoint i32* %a to i32, !dbg !32
  call void @llvm.dbg.value(metadata i32 %1, metadata !25, metadata !DIExpression()), !dbg !32
  %2 = icmp slt i32 %0, %1, !dbg !33
  call void @llvm.dbg.value(metadata i1 %2, metadata !26, metadata !DIExpression()), !dbg !33
  br i1 %2, label %end, label %continue, !dbg !34

continue:                                         ; preds = %entry
  %3 = inttoptr i32 %0 to i32*, !dbg !35
  call void @llvm.dbg.value(metadata i32* %3, metadata !28, metadata !DIExpression()), !dbg !35
  %4 = call spir_func i32 @foo(i32* %3), !dbg !36
  call void @llvm.dbg.value(metadata i32 %4, metadata !29, metadata !DIExpression()), !dbg !36
  br label %end, !dbg !37

end:                                              ; preds = %continue, %entry
  %5 = phi i32 [ %1, %entry ], [ %4, %continue ], !dbg !38
  call void @llvm.dbg.value(metadata i32 %5, metadata !30, metadata !DIExpression()), !dbg !38
  ret i32 %5, !dbg !39
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "recursion.ll", directory: "/")
; CHECK-DAG: [[SCOPEK]] = distinct !DISubprogram(name: "test_estimate", linkageName: "test_estimate", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "1", scope: [[SCOPEK]], file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPEK]])
; CHECK-DAG: [[CALLF_MD]] = !DILocalVariable(name: "2", scope: [[SCOPEK]], file: [[FILE]], line: 2
; CHECK-DAG: [[CALLF_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPEK]])
; CHECK-DAG: [[CALLB_MD]] = !DILocalVariable(name: "3", scope: [[SCOPEK]], file: [[FILE]], line: 4
; CHECK-DAG: [[CALLB_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPEK]])
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "4", scope: [[SCOPEK]], file: [[FILE]], line: 5
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPEK]])
;
; CHECK-DAG: [[SCOPEF]] = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: [[FILE]], line: 8
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "5", scope: [[SCOPEF]], file: [[FILE]], line: 8
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPEF]])
; CHECK-DAG: [[PTRTOI_MD]] = !DILocalVariable(name: "6", scope: [[SCOPEF]], file: [[FILE]], line: 9
; CHECK-DAG: [[PTRTOI_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPEF]])
; CHECK-DAG: [[CMP_MD]] = !DILocalVariable(name: "7", scope: [[SCOPEF]], file: [[FILE]], line: 10
; CHECK-DAG: [[CMP_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPEF]])
; CHECK-DAG: [[ITOPTR_MD]] = !DILocalVariable(name: "8", scope: [[SCOPEF]], file: [[FILE]], line: 12
; CHECK-DAG: [[ITOPTR_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPEF]])
; CHECK-DAG: [[CALLF2_MD]] = !DILocalVariable(name: "9", scope: [[SCOPEF]], file: [[FILE]], line: 13
; CHECK-DAG: [[CALLF2_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPEF]])

declare spir_func i32 @__builtin_spirv_BuiltInSubgroupId()

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "recursion.ll", directory: "/")
!2 = !{}
!3 = !{i32 16}
!4 = !{i32 10}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_estimate", linkageName: "test_estimate", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !12)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
!20 = !DILocation(line: 6, column: 1, scope: !6)
!21 = !DILocation(line: 7, column: 1, scope: !6)
!22 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 8, type: !7, scopeLine: 8, unit: !0, retainedNodes: !23)
!23 = !{!24, !25, !26, !28, !29, !30}
!24 = !DILocalVariable(name: "5", scope: !22, file: !1, line: 8, type: !12)
!25 = !DILocalVariable(name: "6", scope: !22, file: !1, line: 9, type: !12)
!26 = !DILocalVariable(name: "7", scope: !22, file: !1, line: 10, type: !27)
!27 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!28 = !DILocalVariable(name: "8", scope: !22, file: !1, line: 12, type: !10)
!29 = !DILocalVariable(name: "9", scope: !22, file: !1, line: 13, type: !12)
!30 = !DILocalVariable(name: "10", scope: !22, file: !1, line: 15, type: !12)
!31 = !DILocation(line: 8, column: 1, scope: !22)
!32 = !DILocation(line: 9, column: 1, scope: !22)
!33 = !DILocation(line: 10, column: 1, scope: !22)
!34 = !DILocation(line: 11, column: 1, scope: !22)
!35 = !DILocation(line: 12, column: 1, scope: !22)
!36 = !DILocation(line: 13, column: 1, scope: !22)
!37 = !DILocation(line: 14, column: 1, scope: !22)
!38 = !DILocation(line: 15, column: 1, scope: !22)
!39 = !DILocation(line: 16, column: 1, scope: !22)
