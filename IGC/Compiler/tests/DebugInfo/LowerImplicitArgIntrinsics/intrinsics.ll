;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-lower-implicit-arg-intrinsic -S < %s | FileCheck %s
; ------------------------------------------------
; LowerImplicitArgIntrinsics
; ------------------------------------------------
; This test checks that LowerImplicitArgIntrinsics pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_func void @test{{.*}} !dbg [[SCOPE:![0-9]*]]

; CHECK-DAG: store <3 x i32> [[VAL1_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <3 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: store <3 x i32> [[VAL2_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR2_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <3 x i32> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: store <3 x i32> [[VAL3_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR3_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <3 x i32> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: store <3 x i32> [[VAL4_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR4_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <3 x i32> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 addrspace(1)* [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: store i32 [[VAL6_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR5_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @test(<3 x i32> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 !dbg !19 {
entry:
  %0 = call <3 x i32> @llvm.genx.GenISA.getLocalSize.v4i32(), !dbg !31
  call void @llvm.dbg.value(metadata <3 x i32> %0, metadata !22, metadata !DIExpression()), !dbg !31
  store <3 x i32> %0, <3 x i32> addrspace(1)* %dst, align 4, !dbg !32
  %1 = call <3 x i32> @llvm.genx.GenISA.getPayloadHeader.v4i32(), !dbg !33
  call void @llvm.dbg.value(metadata <3 x i32> %1, metadata !24, metadata !DIExpression()), !dbg !33
  store <3 x i32> %1, <3 x i32> addrspace(1)* %dst, align 4, !dbg !34
  %2 = call <3 x i32> @llvm.genx.GenISA.getGlobalSize.v4i32(), !dbg !35
  call void @llvm.dbg.value(metadata <3 x i32> %2, metadata !25, metadata !DIExpression()), !dbg !35
  store <3 x i32> %2, <3 x i32> addrspace(1)* %dst, align 4, !dbg !36
  %3 = call <3 x i32> @llvm.genx.GenISA.getNumWorkGroups.v4i32(), !dbg !37
  call void @llvm.dbg.value(metadata <3 x i32> %3, metadata !26, metadata !DIExpression()), !dbg !37
  store <3 x i32> %3, <3 x i32> addrspace(1)* %dst, align 4, !dbg !38
  %4 = call i32 addrspace(1)* @llvm.genx.GenISA.getPrintfBuffer.p1i32(), !dbg !39
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %4, metadata !27, metadata !DIExpression()), !dbg !39
  %5 = call i32 @llvm.genx.GenISA.getWorkDim.i32(), !dbg !40
  call void @llvm.dbg.value(metadata i32 %5, metadata !29, metadata !DIExpression()), !dbg !40
  store i32 %5, i32 addrspace(1)* %4, align 4, !dbg !41
  ret void, !dbg !42
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "intrinsics.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR4_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR5_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])

declare <3 x i32> @llvm.genx.GenISA.getLocalSize.v4i32()

declare <3 x i32> @llvm.genx.GenISA.getPayloadHeader.v4i32()

declare <3 x i32> @llvm.genx.GenISA.getGlobalSize.v4i32()

declare <3 x i32> @llvm.genx.GenISA.getNumWorkGroups.v4i32()

declare i32 @llvm.genx.GenISA.getWorkDim.i32()

declare i32 addrspace(1)* @llvm.genx.GenISA.getPrintfBuffer.p1i32()

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone "visaStackCall" }
attributes #1 = { nounwind readnone speculatable }

!llvm.module.flags = !{!0, !1, !2}
!igc.functions = !{!3}
!llvm.dbg.cu = !{!15}
!llvm.debugify = !{!12, !18}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{void (<3 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 2}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 7}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 12}
!13 = !{i32 14, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = distinct !DICompileUnit(language: DW_LANG_C, file: !16, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !17)
!16 = !DIFile(filename: "intrinsics.ll", directory: "/")
!17 = !{}
!18 = !{i32 6}
!19 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !16, line: 1, type: !20, scopeLine: 1, unit: !15, retainedNodes: !21)
!20 = !DISubroutineType(types: !17)
!21 = !{!22, !24, !25, !26, !27, !29}
!22 = !DILocalVariable(name: "1", scope: !19, file: !16, line: 1, type: !23)
!23 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "2", scope: !19, file: !16, line: 3, type: !23)
!25 = !DILocalVariable(name: "3", scope: !19, file: !16, line: 5, type: !23)
!26 = !DILocalVariable(name: "4", scope: !19, file: !16, line: 7, type: !23)
!27 = !DILocalVariable(name: "5", scope: !19, file: !16, line: 9, type: !28)
!28 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!29 = !DILocalVariable(name: "6", scope: !19, file: !16, line: 10, type: !30)
!30 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!31 = !DILocation(line: 1, column: 1, scope: !19)
!32 = !DILocation(line: 2, column: 1, scope: !19)
!33 = !DILocation(line: 3, column: 1, scope: !19)
!34 = !DILocation(line: 4, column: 1, scope: !19)
!35 = !DILocation(line: 5, column: 1, scope: !19)
!36 = !DILocation(line: 6, column: 1, scope: !19)
!37 = !DILocation(line: 7, column: 1, scope: !19)
!38 = !DILocation(line: 8, column: 1, scope: !19)
!39 = !DILocation(line: 9, column: 1, scope: !19)
!40 = !DILocation(line: 10, column: 1, scope: !19)
!41 = !DILocation(line: 11, column: 1, scope: !19)
!42 = !DILocation(line: 12, column: 1, scope: !19)
