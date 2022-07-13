;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-legalize-function-signatures -S < %s | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------
; This test checks that LegalizeFunctionSignatures pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

;
; Testcase 1:
; Calls are substituted with calls to legalized functions
;
; CHECK-DAG: [[FOO_V:%[0-9]*]] = call i32 @foo({{.*}} [[FOO_ARG:%[0-9]*]]), !dbg [[FOO_LOC:![0-9]*]]
; CHECK-DAG: [[FOO_ARG]] = zext {{.*}}, !dbg [[FOO_LOC]]
; CHECK: @llvm.dbg.value(metadata i32 [[FOO_V]], metadata [[FOO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FOO_LOC]]
; CHECK: call void @bar{{.*}}, !dbg [[BAR_LOC:![0-9]*]]
;

define spir_kernel void @test_k(i32 %src) !dbg !6 {
  %1 = zext i32 %src to i48, !dbg !18
  call void @llvm.dbg.value(metadata i48 %1, metadata !9, metadata !DIExpression()), !dbg !18
  %2 = insertelement <3 x i48> <i48 0, i48 42, i48 13>, i48 %1, i32 0, !dbg !19
  call void @llvm.dbg.value(metadata <3 x i48> %2, metadata !11, metadata !DIExpression()), !dbg !19
  %3 = call i32 @foo(<3 x i48> %2), !dbg !20
  call void @llvm.dbg.value(metadata i32 %3, metadata !13, metadata !DIExpression()), !dbg !20
  %4 = alloca i32, !dbg !21
  call void @llvm.dbg.value(metadata i32* %4, metadata !15, metadata !DIExpression()), !dbg !21
  store i32 %3, i32* %4, !dbg !22
  call void @bar(<3 x i48> %2), !dbg !23
; FAILING
;  %5 = ptrtoint void (<3 x i48>) * @bar to i32
  ret void, !dbg !24
}

; Testcase 2:
;
; Substituted function has it's debug preserved.
; CHECK: define spir_func i32 @foo({{.*}}) !dbg [[FOO_SCOPE:![0-9]*]]
; CHECK-NEXT: [[ARG_V:%[0-9]*]] = trunc {{.*}}
; CHECK-NEXT: alloca {{.*}}, !dbg [[ALLOC_LOC:![0-9]*]]
; CHECK-NEXT: @llvm.dbg.declare(metadata <3 x i48>* {{.*}}, metadata [[ALLOC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC_LOC]]
; CHECK-NEXT: store <3 x i48> [[ARG_V]], {{.*}}, !dbg [[STORE_LOC:![0-9]*]]
define spir_func i32 @foo(<3 x i48> %src) !dbg !25 {
  %1 = alloca <3 x i48>, !dbg !33
  call void @llvm.dbg.declare(metadata <3 x i48>* %1, metadata !27, metadata !DIExpression()), !dbg !33
  store <3 x i48> %src, <3 x i48>* %1, !dbg !34
  %2 = load <3 x i48>, <3 x i48>* %1, !dbg !35
  call void @llvm.dbg.value(metadata <3 x i48> %2, metadata !28, metadata !DIExpression()), !dbg !35
  %3 = extractelement <3 x i48> %2, i32 0, !dbg !36
  call void @llvm.dbg.value(metadata i48 %3, metadata !29, metadata !DIExpression()), !dbg !36
  %4 = extractelement <3 x i48> %src, i32 1, !dbg !37
  call void @llvm.dbg.value(metadata i48 %4, metadata !30, metadata !DIExpression()), !dbg !37
  %5 = add i48 %3, %4, !dbg !38
  call void @llvm.dbg.value(metadata i48 %5, metadata !31, metadata !DIExpression()), !dbg !38
  %6 = trunc i48 %5 to i32, !dbg !39
  call void @llvm.dbg.value(metadata i32 %6, metadata !32, metadata !DIExpression()), !dbg !39
  ret i32 %6, !dbg !40
}

; MD checks:
;
; CHECK-DAG: [[KERNEL_SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_k", linkageName: "test_k", scope: null
; CHECK-DAG: [[FOO_LOC]] = !DILocation(line: 3, column: 1, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[FOO_MD]] = !DILocalVariable(name: "3", scope: [[KERNEL_SCOPE]], file: {{.*}}, line: 3
; CHECK-DAG: [[BAR_LOC]] = !DILocation(line: 6, column: 1, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[FOO_SCOPE]] =  distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: {{.*}}, line: 10
; CHECK-DAG: [[ALLOC_MD]] =  !DILocalVariable(name: "7", scope: [[FOO_SCOPE]], file: {{.*}}, line: 10
; CHECK-DAG: [[ALLOC_LOC]] = !DILocation(line: 10, column: 1, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 11, column: 1, scope: [[FOO_SCOPE]])

declare void @bar(<3 x i48>) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { "IndirectlyCalled" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "LegalizeFunctionSignatures/fixintv.ll", directory: "/")
!2 = !{}
!3 = !{i32 17}
!4 = !{i32 12}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_k", linkageName: "test_k", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15, !16, !17}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !10)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 8, type: !14)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 9, column: 1, scope: !6)
!25 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 10, type: !7, scopeLine: 10, unit: !0, retainedNodes: !26)
!26 = !{!27, !28, !29, !30, !31, !32}
!27 = !DILocalVariable(name: "7", scope: !25, file: !1, line: 10, type: !10)
!28 = !DILocalVariable(name: "8", scope: !25, file: !1, line: 12, type: !12)
!29 = !DILocalVariable(name: "9", scope: !25, file: !1, line: 13, type: !10)
!30 = !DILocalVariable(name: "10", scope: !25, file: !1, line: 14, type: !10)
!31 = !DILocalVariable(name: "11", scope: !25, file: !1, line: 15, type: !10)
!32 = !DILocalVariable(name: "12", scope: !25, file: !1, line: 16, type: !14)
!33 = !DILocation(line: 10, column: 1, scope: !25)
!34 = !DILocation(line: 11, column: 1, scope: !25)
!35 = !DILocation(line: 12, column: 1, scope: !25)
!36 = !DILocation(line: 13, column: 1, scope: !25)
!37 = !DILocation(line: 14, column: 1, scope: !25)
!38 = !DILocation(line: 15, column: 1, scope: !25)
!39 = !DILocation(line: 16, column: 1, scope: !25)
!40 = !DILocation(line: 17, column: 1, scope: !25)
