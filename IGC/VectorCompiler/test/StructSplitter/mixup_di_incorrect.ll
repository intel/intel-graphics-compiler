;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test shows wrong behavior of dbg support when types in structure are in mess.

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%v8_varying_B = type { i32, %v8_uniform_A, float }
%v8_uniform_A = type { i32, float, float, i32 }
; CHECK-DAG: %[[B_I:[^ ]+]] = type { i32, %[[A_I:[^ ]+]] }
; CHECK-DAG: %[[A_I]] = type { i32, i32 }
; CHECK-DAG: %[[B_F:[^ ]+]] = type { %[[A_F:[^ ]+]], float }
; CHECK-DAG: %[[A_F]] = type { float, float }

define void @mixup_test___() !dbg !3 {
  ; CHECK-DAG:  %[[MX_I:[^ ]+]] = alloca %[[B_I]], align 8
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata %[[B_I]]* %[[MX_I]], metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 32))
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata %[[B_I]]* %[[MX_I]], metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 32, 64))
  ; should be:
  ;  dbg.declare(..., fragment(0, 32))
  ;  dbg.declare(..., fragment(32, 32))
  ;  dbg.declare(..., fragment(128, 32))
  ; or:
  ;  dbg.declare(..., fragment(0, 64))
  ;  dbg.declare(..., fragment(128, 32))
  ; CHECK-DAG:  %[[MX_F:[^ ]+]] = alloca %[[B_F]], align 8
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata %[[B_F]]* %[[MX_F]], metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 64, 64))
  ; CHECK-DAG:  call void @llvm.dbg.declare(metadata %[[B_F]]* %[[MX_F]], metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 160, 32))
  ; Two declares above are correct.

  %mx = alloca %v8_varying_B, align 8, !dbg !22
  call void @llvm.dbg.declare(metadata %v8_varying_B* %mx, metadata !9, metadata !DIExpression()), !dbg !22

  %mx1 = getelementptr %v8_varying_B, %v8_varying_B* %mx, i32 0, i32 1
  %mx12 = getelementptr %v8_uniform_A, %v8_uniform_A* %mx1, i32 0, i32 1
  store float 5.000000e+00, float* %mx12, align 4
  ret void
}

declare void @llvm.dbg.declare(metadata, metadata, metadata) #3

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!23}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !1, producer: "ispc", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "mixup_di.ispc", directory: "/")
!2 = !{}
!3 = distinct !DISubprogram(name: "mixup_test", linkageName: "mixup_test___", scope: !4, file: !1, line: 15, type: !5, scopeLine: 15, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !7)
!4 = !DINamespace(name: "ispc", scope: !1, exportSymbols: true)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!7 = !{!9}
!8 = !DIBasicType(name: "int32", size: 32, encoding: DW_ATE_signed)
!9 = !DILocalVariable(name: "mx", scope: !3, file: !1, line: 15, type: !10)
!10 = !DICompositeType(tag: DW_TAG_structure_type, name: "varying struct B", scope: !3, file: !1, line: 8, size: 192, align: 32, elements: !11)
!11 = !{!12, !13, !21}
!12 = !DIDerivedType(tag: DW_TAG_member, name: "i", scope: !10, file: !1, line: 9, baseType: !8, size: 32, align: 32)
!13 = !DIDerivedType(tag: DW_TAG_member, name: "a", scope: !10, file: !1, line: 10, baseType: !14, size: 128, align: 32, offset: 32)
!14 = !DICompositeType(tag: DW_TAG_structure_type, name: "uniform struct A", scope: !3, file: !1, line: 1, size: 128, align: 32, elements: !15)
!15 = !{!16, !17, !19, !20}
!16 = !DIDerivedType(tag: DW_TAG_member, name: "a", scope: !14, file: !1, line: 2, baseType: !8, size: 32, align: 32)
!17 = !DIDerivedType(tag: DW_TAG_member, name: "b", scope: !14, file: !1, line: 3, baseType: !18, size: 32, align: 32, offset: 32)
!18 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!19 = !DIDerivedType(tag: DW_TAG_member, name: "c", scope: !14, file: !1, line: 4, baseType: !18, size: 32, align: 32, offset: 64)
!20 = !DIDerivedType(tag: DW_TAG_member, name: "d", scope: !14, file: !1, line: 5, baseType: !8, size: 32, align: 32, offset: 96)
!21 = !DIDerivedType(tag: DW_TAG_member, name: "f", scope: !14, file: !1, line: 11, baseType: !18, size: 32, align: 32, offset: 160)
!22 = !DILocation(line: 15, column: 14, scope: !3)
!23 = !{i32 2, !"Debug Info Version", i32 3}
