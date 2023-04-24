;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXGEPLowering
; ------------------------------------------------
; This test checks that GenXGEPLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Basic gep and ptrtoint transformations.


; CHECK: void @test_lowergep{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = addrspacecast{{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i8 addrspace(4)* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = ptrtoint{{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i8 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
;
; CHECK: geps:
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x float>* [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = load{{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x float> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9]*]] = inttoptr{{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32* [[VAL7_V:%[A-z0-9.]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x i32*> [[VAL9_V:%[A-z0-9.]*]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: [[VAL9_V]] = {{.*}}, !dbg [[VAL9_LOC]]

%struct._test = type { i32, <4 x float>, [2 x i64] }

define void @test_lowergep(i8 addrspace(1)* %a, %struct._test* byval(%struct._test) %src, i32 %iptr) !dbg !6 {
entry:
  %0 = addrspacecast i8 addrspace(1)* %a to i8 addrspace(4)*, !dbg !23
  call void @llvm.dbg.value(metadata i8 addrspace(4)* %0, metadata !9, metadata !DIExpression()), !dbg !23
  %1 = ptrtoint i8 addrspace(4)* %0 to i8, !dbg !24
  call void @llvm.dbg.value(metadata i8 %1, metadata !11, metadata !DIExpression()), !dbg !24
  store i8 %1, i8 addrspace(4)* %0, !dbg !25
  br label %geps, !dbg !26

geps:                                             ; preds = %entry
  %2 = alloca %struct._test, align 16, !dbg !27
  call void @llvm.dbg.value(metadata %struct._test* %2, metadata !13, metadata !DIExpression()), !dbg !27
  %3 = getelementptr inbounds %struct._test, %struct._test* %2, i32 1, i32 1, !dbg !28
  call void @llvm.dbg.value(metadata <4 x float>* %3, metadata !14, metadata !DIExpression()), !dbg !28
  %4 = load <4 x float>, <4 x float>* %3, align 16, !dbg !29
  call void @llvm.dbg.value(metadata <4 x float> %4, metadata !15, metadata !DIExpression()), !dbg !29
  %5 = inttoptr i32 %iptr to i32*, !dbg !30
  call void @llvm.dbg.value(metadata i32* %5, metadata !17, metadata !DIExpression()), !dbg !30
  %6 = getelementptr inbounds i32, i32* %5, i32 2, !dbg !31
  call void @llvm.dbg.value(metadata i32* %6, metadata !18, metadata !DIExpression()), !dbg !31
  %7 = load i32, i32* %6, align 16, !dbg !32
  call void @llvm.dbg.value(metadata i32 %7, metadata !19, metadata !DIExpression()), !dbg !32
  %8 = getelementptr inbounds i32, i32* %5, <4 x i32> <i32 1, i32 2, i32 3, i32 4>, !dbg !33
  call void @llvm.dbg.value(metadata <4 x i32*> %8, metadata !21, metadata !DIExpression()), !dbg !33
  ret void, !dbg !34
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_lowergep", linkageName: "test_lowergep", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "basic.ll", directory: "/")
!2 = !{}
!3 = !{i32 12}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_lowergep", linkageName: "test_lowergep", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !17, !18, !19, !21}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 5, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 6, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !16)
!16 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 8, type: !10)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 9, type: !10)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 10, type: !20)
!20 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 11, type: !22)
!22 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!23 = !DILocation(line: 1, column: 1, scope: !6)
!24 = !DILocation(line: 2, column: 1, scope: !6)
!25 = !DILocation(line: 3, column: 1, scope: !6)
!26 = !DILocation(line: 4, column: 1, scope: !6)
!27 = !DILocation(line: 5, column: 1, scope: !6)
!28 = !DILocation(line: 6, column: 1, scope: !6)
!29 = !DILocation(line: 7, column: 1, scope: !6)
!30 = !DILocation(line: 8, column: 1, scope: !6)
!31 = !DILocation(line: 9, column: 1, scope: !6)
!32 = !DILocation(line: 10, column: 1, scope: !6)
!33 = !DILocation(line: 11, column: 1, scope: !6)
!34 = !DILocation(line: 12, column: 1, scope: !6)
