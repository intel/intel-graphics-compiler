;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLowering
; ------------------------------------------------
; This test checks that GenXLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: void @test_lowering{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: phs:
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata %st.struct.1 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: end:
; CHECK-DAG: void @llvm.dbg.value(metadata %st.struct.1 [[VAL3_V:%[A-z0-9.]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> {{%[A-z0-9.]*}}, metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]


%st.struct.1 = type { i1, i32, <4 x i32> }

define void @test_lowering(%st.struct.1 %a, i32 %b, %st.struct.1 %c, <4 x i32>* %d) !dbg !6 {
entry:
  %0 = extractvalue %st.struct.1 %a, 0, !dbg !16
  call void @llvm.dbg.value(metadata i1 %0, metadata !9, metadata !DIExpression()), !dbg !16
  br i1 %0, label %end, label %phs, !dbg !17

phs:                                              ; preds = %entry
  %1 = insertvalue %st.struct.1 %c, i32 %b, 1, !dbg !18
  call void @llvm.dbg.value(metadata %st.struct.1 %1, metadata !11, metadata !DIExpression()), !dbg !18
  br label %end, !dbg !19

end:                                              ; preds = %phs, %entry
  %2 = phi %st.struct.1 [ %a, %entry ], [ %1, %phs ], !dbg !20
  call void @llvm.dbg.value(metadata %st.struct.1 %2, metadata !13, metadata !DIExpression()), !dbg !20
  %3 = extractvalue %st.struct.1 %2, 2, !dbg !21
  call void @llvm.dbg.value(metadata <4 x i32> %3, metadata !14, metadata !DIExpression()), !dbg !21
  store <4 x i32> %3, <4 x i32>* %d, !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "trunc-cast.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_lowering", linkageName: "test_lowering", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "trunc-cast.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_lowering", linkageName: "test_lowering", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !12)
!12 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 5, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 6, type: !15)
!15 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
