;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-const-prop -S < %s | FileCheck %s
; ------------------------------------------------
; IGCConstProp
; ------------------------------------------------
; This test checks that IGCConstProp pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_slmconst{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; First value is dead
; CHECK: call void @llvm.dbg.value
;
; CHECK: void @llvm.dbg.value(metadata i1 false, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: store i32 [[VAL3_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

; Another dead value
; CHECK: call void @llvm.dbg.value
; CHECK: void @llvm.dbg.value(metadata i32 55, metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: store i32 55{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

; Another dead value
; CHECK: call void @llvm.dbg.value
; CHECK: void @llvm.dbg.value(metadata i32 42, metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: store i32 42{{.*}}, !dbg [[STORE3_LOC:![0-9]*]]

; Another dead value
; CHECK: call void @llvm.dbg.value
; CHECK: void @llvm.dbg.value(metadata i32 0, metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: store i32 0{{.*}}, !dbg [[STORE4_LOC:![0-9]*]]

define spir_kernel void @test_slmconst(i32 %a, i32* %b, i1 %c) !dbg !20 {
  %1 = extractelement <4 x i32> <i32 12, i32 13, i32 13, i32 42>, i32 %a, !dbg !36
  call void @llvm.dbg.value(metadata i32 %1, metadata !23, metadata !DIExpression()), !dbg !36
  %2 = icmp eq i32 %1, 0, !dbg !37
  call void @llvm.dbg.value(metadata i1 %2, metadata !25, metadata !DIExpression()), !dbg !37
  %3 = select i1 %2, i32 0, i32 %a, !dbg !38
  call void @llvm.dbg.value(metadata i32 %3, metadata !27, metadata !DIExpression()), !dbg !38
  store i32 %3, i32* %b, !dbg !39
  %4 = insertelement <4 x i32> <i32 12, i32 13, i32 13, i32 42>, i32 55, i32 3, !dbg !40
  call void @llvm.dbg.value(metadata <4 x i32> %4, metadata !28, metadata !DIExpression()), !dbg !40
  %5 = extractelement <4 x i32> %4, i32 3, !dbg !41
  call void @llvm.dbg.value(metadata i32 %5, metadata !30, metadata !DIExpression()), !dbg !41
  store i32 %5, i32* %b, !dbg !42
  %6 = select i1 %c, <4 x i32> <i32 12, i32 13, i32 13, i32 42>, <4 x i32> <i32 21, i32 31, i32 32, i32 42>, !dbg !43
  call void @llvm.dbg.value(metadata <4 x i32> %6, metadata !31, metadata !DIExpression()), !dbg !43
  %7 = extractelement <4 x i32> %6, i32 3, !dbg !44
  call void @llvm.dbg.value(metadata i32 %7, metadata !32, metadata !DIExpression()), !dbg !44
  store i32 %7, i32* %b, !dbg !45
  %8 = inttoptr i32 42 to i32 addrspace(65549)*, !dbg !46
  call void @llvm.dbg.value(metadata i32 addrspace(65549)* %8, metadata !33, metadata !DIExpression()), !dbg !46
  %9 = load i32, i32 addrspace(65549)* %8, !dbg !47
  call void @llvm.dbg.value(metadata i32 %9, metadata !35, metadata !DIExpression()), !dbg !47
  store i32 %9, i32* %b, !dbg !48
  ret void, !dbg !49
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_slmconst", linkageName: "test_slmconst", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE4_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!14}
!llvm.debugify = !{!17, !18}
!llvm.module.flags = !{!19}

!0 = !{!"ModuleMD", !1, !11}
!1 = !{!"immConstant", !2}
!2 = !{!"data", !3, !4, !5, !6, !7, !8, !9, !10}
!3 = !{!"dataVec[0]", i8 1}
!4 = !{!"dataVec[1]", i8 2}
!5 = !{!"dataVec[2]", i8 3}
!6 = !{!"dataVec[3]", i8 4}
!7 = !{!"dataVec[4]", i8 5}
!8 = !{!"dataVec[5]", i8 6}
!9 = !{!"dataVec[6]", i8 7}
!10 = !{!"dataVec[7]", i8 8}
!11 = !{!"pushInfo", !12, !13}
!12 = !{!"inlineConstantBufferOffset", i32 -1}
!13 = !{!"inlineConstantBufferSlot", i32 13}
!14 = distinct !DICompileUnit(language: DW_LANG_C, file: !15, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !16)
!15 = !DIFile(filename: "basic.ll", directory: "/")
!16 = !{}
!17 = !{i32 14}
!18 = !{i32 9}
!19 = !{i32 2, !"Debug Info Version", i32 3}
!20 = distinct !DISubprogram(name: "test_slmconst", linkageName: "test_slmconst", scope: null, file: !15, line: 1, type: !21, scopeLine: 1, unit: !14, retainedNodes: !22)
!21 = !DISubroutineType(types: !16)
!22 = !{!23, !25, !27, !28, !30, !31, !32, !33, !35}
!23 = !DILocalVariable(name: "1", scope: !20, file: !15, line: 1, type: !24)
!24 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!25 = !DILocalVariable(name: "2", scope: !20, file: !15, line: 2, type: !26)
!26 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!27 = !DILocalVariable(name: "3", scope: !20, file: !15, line: 3, type: !24)
!28 = !DILocalVariable(name: "4", scope: !20, file: !15, line: 5, type: !29)
!29 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!30 = !DILocalVariable(name: "5", scope: !20, file: !15, line: 6, type: !24)
!31 = !DILocalVariable(name: "6", scope: !20, file: !15, line: 8, type: !29)
!32 = !DILocalVariable(name: "7", scope: !20, file: !15, line: 9, type: !24)
!33 = !DILocalVariable(name: "8", scope: !20, file: !15, line: 11, type: !34)
!34 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!35 = !DILocalVariable(name: "9", scope: !20, file: !15, line: 12, type: !24)
!36 = !DILocation(line: 1, column: 1, scope: !20)
!37 = !DILocation(line: 2, column: 1, scope: !20)
!38 = !DILocation(line: 3, column: 1, scope: !20)
!39 = !DILocation(line: 4, column: 1, scope: !20)
!40 = !DILocation(line: 5, column: 1, scope: !20)
!41 = !DILocation(line: 6, column: 1, scope: !20)
!42 = !DILocation(line: 7, column: 1, scope: !20)
!43 = !DILocation(line: 8, column: 1, scope: !20)
!44 = !DILocation(line: 9, column: 1, scope: !20)
!45 = !DILocation(line: 10, column: 1, scope: !20)
!46 = !DILocation(line: 11, column: 1, scope: !20)
!47 = !DILocation(line: 12, column: 1, scope: !20)
!48 = !DILocation(line: 13, column: 1, scope: !20)
!49 = !DILocation(line: 14, column: 1, scope: !20)
