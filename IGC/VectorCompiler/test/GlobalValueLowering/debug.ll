;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXGlobalValueLowering
; ------------------------------------------------
; This test checks that GenXGlobalValueLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: void @test_gvlower{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = load i32 addrspace(3)*{{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(3)* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = getelementptr {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(3)* [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = call i32 {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: store i32 {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = extractvalue {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata [8 x i32]* [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = getelementptr {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = load {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: store i32 {{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

%struct.st = type { i32, [8 x i32]* }

@a = internal addrspace(3) global i32 addrspace(3)* null, align 8
@b = common global [8 x i32] zeroinitializer, align 8

define dllexport spir_kernel void @test_gvlower(i32* %a) #2 !dbg !6 {
  %1 = load i32 addrspace(3)*, i32 addrspace(3)* addrspace(3)* @a, align 8, !dbg !17
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %1, metadata !9, metadata !DIExpression()), !dbg !17
  %2 = getelementptr inbounds i32, i32 addrspace(3)* %1, i64 0, !dbg !18
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %2, metadata !11, metadata !DIExpression()), !dbg !18
  %3 = call i32 bitcast (i32 (i8*)* @foo to i32 (i32*)*)(i32* %a), !dbg !19
  call void @llvm.dbg.value(metadata i32 %3, metadata !12, metadata !DIExpression()), !dbg !19
  store i32 %3, i32 addrspace(3)* %2, align 4, !dbg !20
  %4 = extractvalue %struct.st { i32 12, [8 x i32]* @b }, 1, !dbg !21
  call void @llvm.dbg.value(metadata [8 x i32]* %4, metadata !14, metadata !DIExpression()), !dbg !21
  %5 = getelementptr inbounds [8 x i32], [8 x i32]* %4, i32 0, i32 1, !dbg !22
  call void @llvm.dbg.value(metadata i32* %5, metadata !15, metadata !DIExpression()), !dbg !22
  %6 = load i32, i32* %5, !dbg !23
  call void @llvm.dbg.value(metadata i32 %6, metadata !16, metadata !DIExpression()), !dbg !23
  store i32 %6, i32* %a, !dbg !24
  ret void, !dbg !25
}

define i32 @foo(i8* %b) #1 !dbg !26 {
  %1 = ptrtoint i8* %b to i32, !dbg !29
  call void @llvm.dbg.value(metadata i32 %1, metadata !28, metadata !DIExpression()), !dbg !29
  ret i32 %1, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXGlobalValueLowering.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_gvlower", linkageName: "test_gvlower", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }
attributes #1 = { "CMStackCall" }
attributes #2 = { "CMGenxMain" }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}
!genx.kernels = !{!31}
!genx.kernel.internal = !{!34}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXGlobalValueLowering.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 7}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_gvlower", linkageName: "test_gvlower", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !13)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 10, type: !7, scopeLine: 10, unit: !0, retainedNodes: !27)
!27 = !{!28}
!28 = !DILocalVariable(name: "7", scope: !26, file: !1, line: 10, type: !13)
!29 = !DILocation(line: 10, column: 1, scope: !26)
!30 = !DILocation(line: 11, column: 1, scope: !26)
!31 = !{void (i32*)* @test_gvlower, !"test_gvlower", !32, i32 0, !32, !32, !33, i32 0}
!32 = !{}
!33 = !{!""}
!34 = !{void (i32*)* @test_gvlower, !32, !32, !32, !32}
