;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%st0 = type { <8 x i32>, <8 x float> }
%st1 = type { <8 x i32>, %st0 }
; CHECK: %[[S1_I:[^ ]+]] = type { <8 x i32>, <8 x i32> }

define void @test_split(<8 x float> %a, <8 x i32> %b) !dbg !6 {

  ; CHECK-DAG:  %[[I_A:[^ ]+]] = alloca <8 x i32>, align 16
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata <8 x i32>* %[[I_A]], metadata ![[MET_ST0:[^ ]+]], metadata !DIExpression(DW_OP_LLVM_fragment, 0, 256))
  ; CHECK-DAG:  %[[F_A:[^ ]+]] = alloca <8 x float>, align 16
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata <8 x float>* %[[F_A]], metadata ![[MET_ST0]], metadata !DIExpression(DW_OP_LLVM_fragment, 256, 256))

  %1 = alloca %st0, align 16, !dbg !27
  call void @llvm.dbg.declare(metadata %st0* %1, metadata !9, metadata !DIExpression()), !dbg !27

  ; CHECK-DAG:  %[[I1_A:[^ ]+]] = alloca  %[[S1_I]], align 16
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata %[[S1_I]]* %[[I1_A]], metadata ![[MET_ST1:[^ ]+]], metadata !DIExpression(DW_OP_LLVM_fragment, 0, 256))
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata %[[S1_I]]* %[[I1_A]], metadata ![[MET_ST1]], metadata !DIExpression(DW_OP_LLVM_fragment, 256, 256))
  ; CHECK-DAG:  %[[F2_A:[^ ]+]] = alloca <8 x float>, align 16
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata <8 x float>* %[[F2_A]], metadata ![[MET_ST1]], metadata !DIExpression(DW_OP_LLVM_fragment, 512, 256))

  %2 = alloca %st1, align 16, !dbg !28
  call void @llvm.dbg.declare(metadata %st1* %2, metadata !21, metadata !DIExpression()), !dbg !28
  %3 = getelementptr %st0, %st0* %1, i32 0, i32 1, !dbg !29
  %4 = getelementptr %st1, %st1* %2, i32 0, i32 1, i32 0, !dbg !30
  store <8 x float> %a, <8 x float>* %3, !dbg !31
  store <8 x i32> %b, <8 x i32>* %4, !dbg !32
  ret void, !dbg !33
}

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "split_alloc.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_split", linkageName: "test_split", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !21}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
; CHECK:  ![[MET_ST0]] = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: ![[TYPE_ST0:[^ ]+]])
!10 = !DIDerivedType(tag: DW_TAG_typedef, name: "St0", file: !1, line: 14, baseType: !11)
!11 = !DICompositeType(tag: DW_TAG_structure_type, file: !1, line: 15, size: 512, align: 32, elements: !12)
!12 = !{!13, !18}
!13 = !DIDerivedType(tag: DW_TAG_member, name: "St0a", scope: !11, file: !1, line: 16, baseType: !14, size: 256, align: 32)
!14 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 256, align: 32, flags: DIFlagVector, elements: !16)
!15 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!16 = !{!17}
!17 = !DISubrange(count: 8)
!18 = !DIDerivedType(tag: DW_TAG_member, name: "St0b", scope: !11, file: !1, line: 17, baseType: !19, size: 256, align: 32, offset: 256)
!19 = !DICompositeType(tag: DW_TAG_array_type, baseType: !20, size: 256, align: 32, flags: DIFlagVector, elements: !16)
!20 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!21 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !22)
; CHECK:  ![[MET_ST1]] = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: ![[TYPE_ST1:[^ ]+]])
!22 = !DIDerivedType(tag: DW_TAG_typedef, name: "St1", file: !1, line: 18, baseType: !23)
!23 = !DICompositeType(tag: DW_TAG_structure_type, file: !1, line: 19, size: 768, align: 32, elements: !24)
!24 = !{!25, !26}
!25 = !DIDerivedType(tag: DW_TAG_member, name: "St1a", scope: !23, file: !1, line: 20, baseType: !14, size: 256, align: 32)
!26 = !DIDerivedType(tag: DW_TAG_member, name: "St1b", scope: !23, file: !1, line: 21, baseType: !10, size: 512, offset: 256)
!27 = !DILocation(line: 1, column: 1, scope: !6)
!28 = !DILocation(line: 2, column: 1, scope: !6)
!29 = !DILocation(line: 3, column: 1, scope: !6)
!30 = !DILocation(line: 4, column: 1, scope: !6)
!31 = !DILocation(line: 5, column: 1, scope: !6)
!32 = !DILocation(line: 6, column: 1, scope: !6)
!33 = !DILocation(line: 7, column: 1, scope: !6)
