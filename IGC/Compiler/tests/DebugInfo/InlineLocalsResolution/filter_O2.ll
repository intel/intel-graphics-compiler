;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-resolve-inline-locals -S < %s | FileCheck %s
; ------------------------------------------------
; InlineLocalsResolution
; ------------------------------------------------
; This test checks that InlineLocalsResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------



; CHECK: void @test_inline{{.*}}!dbg [[SCOPE:![0-9]*]]
; Value debug info made from global variable dbg
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(3)* undef, metadata [[BCASTA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FIRST_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(3)* undef, metadata [[BCASTB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCASTB_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(3)* undef, metadata [[GEPB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEPB_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.declare(metadata i32 addrspace(3)* addrspace(3)* @c, metadata [[C_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FIRST_LOC]]
; CHECK: [[BCASTC_V:%[A-z0-9]*]] = {{.*}}, !dbg [[BCASTC_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(3)* [[BCASTC_V]], metadata [[BCASTC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCASTC_LOC]]
; CHECK: [[PTOI_V:%[A-z0-9]*]] = {{.*}}, !dbg [[PTOI_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[PTOI_V]], metadata [[PTOI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTOI_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]
; CHECK: ret{{.*}}, !dbg [[RET_LOC:![0-9]*]]

@a = internal addrspace(3) global i32 addrspace(3)* null, align 8, !dbg !0
@b = internal addrspace(3) global i32 addrspace(3)* null, align 8, !dbg !6
@c = internal addrspace(3) global i32 addrspace(3)* null, align 8, !dbg !10

define spir_kernel void @test_inline(i32* %dst) !dbg !21 {
  %1 = bitcast i32 addrspace(3)* addrspace(3)* @a to i32 addrspace(3)*, !dbg !31
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %1, metadata !24, metadata !DIExpression()), !dbg !31
  store i32 13, i32 addrspace(3)* %1, !dbg !32
  %2 = bitcast i32 addrspace(3)* addrspace(3)* @b to i32 addrspace(3)*, !dbg !33
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %2, metadata !26, metadata !DIExpression()), !dbg !33
  %3 = getelementptr inbounds i32, i32 addrspace(3)* %2, i64 0, !dbg !34
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %3, metadata !27, metadata !DIExpression()), !dbg !34
  %4 = bitcast i32 addrspace(3)* addrspace(3)* @c to i32 addrspace(3)*, !dbg !35
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %4, metadata !28, metadata !DIExpression()), !dbg !35
  %5 = ptrtoint i32 addrspace(3)* %4 to i32, !dbg !36
  call void @llvm.dbg.value(metadata i32 %5, metadata !29, metadata !DIExpression()), !dbg !36
  store i32 %5, i32* %dst, !dbg !37
  ret void, !dbg !38
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "filter_O2.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_inline", linkageName: "test_inline", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[C_MD]] = !DILocalVariable(name: "c", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[FIRST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCASTA_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[BCASTB_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[BCASTB_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GEPB_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[GEPB_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCASTC_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[BCASTC_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PTOI_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[PTOI_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[RET_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!12}
!igc.functions = !{!15}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!18, !19}
!llvm.module.flags = !{!20}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "a", scope: !2, file: !3, line: 1, type: !8, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "filter_O2.ll", directory: "/")
!4 = !{}
!5 = !{!0, !6, !10}
!6 = !DIGlobalVariableExpression(var: !7, expr: !DIExpression())
!7 = distinct !DIGlobalVariable(name: "b", scope: !2, file: !3, line: 2, type: !8, isLocal: true, isDefinition: true)
!8 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !DIGlobalVariableExpression(var: !11, expr: !DIExpression())
!11 = distinct !DIGlobalVariable(name: "c", scope: !2, file: !3, line: 3, type: !8, isLocal: true, isDefinition: true)
!12 = !{!"ModuleMD", !13}
!13 = !{!"compOpt", !14}
!14 = !{!"OptDisable", i1 false}
!15 = !{void (i32*)* @test_inline, !16}
!16 = !{!17}
!17 = !{!"function_type", i32 0}
!18 = !{i32 8}
!19 = !{i32 5}
!20 = !{i32 2, !"Debug Info Version", i32 3}
!21 = distinct !DISubprogram(name: "test_inline", linkageName: "test_inline", scope: null, file: !3, line: 1, type: !22, scopeLine: 1, unit: !2, retainedNodes: !23)
!22 = !DISubroutineType(types: !4)
!23 = !{!24, !26, !27, !28, !29}
!24 = !DILocalVariable(name: "1", scope: !21, file: !3, line: 1, type: !25)
!25 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!26 = !DILocalVariable(name: "2", scope: !21, file: !3, line: 3, type: !25)
!27 = !DILocalVariable(name: "3", scope: !21, file: !3, line: 4, type: !25)
!28 = !DILocalVariable(name: "4", scope: !21, file: !3, line: 5, type: !25)
!29 = !DILocalVariable(name: "5", scope: !21, file: !3, line: 6, type: !30)
!30 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!31 = !DILocation(line: 1, column: 1, scope: !21)
!32 = !DILocation(line: 2, column: 1, scope: !21)
!33 = !DILocation(line: 3, column: 1, scope: !21)
!34 = !DILocation(line: 4, column: 1, scope: !21)
!35 = !DILocation(line: 5, column: 1, scope: !21)
!36 = !DILocation(line: 6, column: 1, scope: !21)
!37 = !DILocation(line: 7, column: 1, scope: !21)
!38 = !DILocation(line: 8, column: 1, scope: !21)
