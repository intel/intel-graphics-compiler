;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
; RUN: igc_opt --igc-gep-lowering -S < %s | FileCheck %s
; ------------------------------------------------
; GEPLowering
; ------------------------------------------------
; This test checks that GEPLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK:  spir_kernel void @test_gep1
; CHECK-SAME: !dbg [[SCOPE1:![0-9]*]]

; CHECK-DAG: void @llvm.dbg.declare(metadata i64* [[GEP1_V:%[A-z0-9]*]], metadata [[GEP1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP1_LOC:![0-9]*]]
; CHECK-DAG: [[GEP1_V]] = {{.*}}, !dbg [[GEP1_LOC]]
; CHECK: load i64{{.*}}, !dbg [[LOAD1_LOC:![0-9]*]]

define spir_kernel void @test_gep1(i32 %iptr) !dbg !11 {
  %1 = sext i32 %iptr to i64, !dbg !20
  call void @llvm.dbg.value(metadata i64 %1, metadata !14, metadata !DIExpression()), !dbg !20
  %2 = add i64 %1, 13, !dbg !21
  call void @llvm.dbg.value(metadata i64 %2, metadata !16, metadata !DIExpression()), !dbg !21
  %ptr = inttoptr i32 %iptr to i64*, !dbg !22
  call void @llvm.dbg.value(metadata i64* %ptr, metadata !17, metadata !DIExpression()), !dbg !22
  %gep = getelementptr inbounds i64, i64* %ptr, i64 %2, !dbg !23
  call void @llvm.dbg.declare(metadata i64* %gep, metadata !18, metadata !DIExpression()), !dbg !23
  %res = load i64, i64* %gep, !dbg !24
  call void @llvm.dbg.value(metadata i64 %res, metadata !19, metadata !DIExpression()), !dbg !24
  store i64 %1, i64* %gep, !dbg !25
  ret void, !dbg !26
}

; CHECK:  spir_kernel void @test_gep2
; CHECK-SAME: !dbg [[SCOPE2:![0-9]*]]

; CHECK-DAG: void @llvm.dbg.declare(metadata i8 addrspace(2)* [[GEP2_V:%[A-z0-9]*]], metadata [[GEP2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP2_LOC:![0-9]*]]
; CHECK-DAG: [[GEP2_V]] = {{.*}}, !dbg [[GEP2_LOC]]
; CHECK: load i8{{.*}}, !dbg [[LOAD2_LOC:![0-9]*]]

define spir_kernel void @test_gep2(i32 %iptr) !dbg !27 {
  %1 = add i32 %iptr, 42, !dbg !36
  call void @llvm.dbg.value(metadata i32 %1, metadata !29, metadata !DIExpression()), !dbg !36
  %2 = sext i32 %1 to i64, !dbg !37
  call void @llvm.dbg.value(metadata i64 %2, metadata !31, metadata !DIExpression()), !dbg !37
  %ptr = inttoptr i32 %iptr to i8 addrspace(2)*, !dbg !38
  call void @llvm.dbg.value(metadata i8 addrspace(2)* %ptr, metadata !32, metadata !DIExpression()), !dbg !38
  %gep = getelementptr inbounds i8, i8 addrspace(2)* %ptr, i64 %2, !dbg !39
  call void @llvm.dbg.declare(metadata i8 addrspace(2)* %gep, metadata !33, metadata !DIExpression()), !dbg !39
  %res = load i8, i8 addrspace(2)* %gep, !dbg !40
  call void @llvm.dbg.value(metadata i8 %res, metadata !34, metadata !DIExpression()), !dbg !40
  ret void, !dbg !41
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "reduceptr.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test_gep1", linkageName: "test_gep1", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test_gep2", linkageName: "test_gep2", scope: null, file: [[FILE]], line: 8
; CHECK-DAG: [[GEP1_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE1]], file: [[FILE]], line: 4
; CHECK-DAG: [[GEP1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[GEP2_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE2]], file: [[FILE]], line: 11
; CHECK-DAG: [[GEP2_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE2]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0, !4}
!llvm.dbg.cu = !{!5}
!llvm.debugify = !{!8, !9}
!llvm.module.flags = !{!10}

!0 = !{void (i32)* @test_gep1, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{void (i32)* @test_gep2, !1}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !6, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !7)
!6 = !DIFile(filename: "reduceptr.ll", directory: "/")
!7 = !{}
!8 = !{i32 13}
!9 = !{i32 10}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = distinct !DISubprogram(name: "test_gep1", linkageName: "test_gep1", scope: null, file: !6, line: 1, type: !12, scopeLine: 1, unit: !5, retainedNodes: !13)
!12 = !DISubroutineType(types: !7)
!13 = !{!14, !16, !17, !18, !19}
!14 = !DILocalVariable(name: "1", scope: !11, file: !6, line: 1, type: !15)
!15 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "2", scope: !11, file: !6, line: 2, type: !15)
!17 = !DILocalVariable(name: "3", scope: !11, file: !6, line: 3, type: !15)
!18 = !DILocalVariable(name: "4", scope: !11, file: !6, line: 4, type: !15)
!19 = !DILocalVariable(name: "5", scope: !11, file: !6, line: 5, type: !15)
!20 = !DILocation(line: 1, column: 1, scope: !11)
!21 = !DILocation(line: 2, column: 1, scope: !11)
!22 = !DILocation(line: 3, column: 1, scope: !11)
!23 = !DILocation(line: 4, column: 1, scope: !11)
!24 = !DILocation(line: 5, column: 1, scope: !11)
!25 = !DILocation(line: 6, column: 1, scope: !11)
!26 = !DILocation(line: 7, column: 1, scope: !11)
!27 = distinct !DISubprogram(name: "test_gep2", linkageName: "test_gep2", scope: null, file: !6, line: 8, type: !12, scopeLine: 8, unit: !5, retainedNodes: !28)
!28 = !{!29, !31, !32, !33, !34}
!29 = !DILocalVariable(name: "6", scope: !27, file: !6, line: 8, type: !30)
!30 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!31 = !DILocalVariable(name: "7", scope: !27, file: !6, line: 9, type: !15)
!32 = !DILocalVariable(name: "8", scope: !27, file: !6, line: 10, type: !15)
!33 = !DILocalVariable(name: "9", scope: !27, file: !6, line: 11, type: !15)
!34 = !DILocalVariable(name: "10", scope: !27, file: !6, line: 12, type: !35)
!35 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!36 = !DILocation(line: 8, column: 1, scope: !27)
!37 = !DILocation(line: 9, column: 1, scope: !27)
!38 = !DILocation(line: 10, column: 1, scope: !27)
!39 = !DILocation(line: 11, column: 1, scope: !27)
!40 = !DILocation(line: 12, column: 1, scope: !27)
!41 = !DILocation(line: 13, column: 1, scope: !27)
