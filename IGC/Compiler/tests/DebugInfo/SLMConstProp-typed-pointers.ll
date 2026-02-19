;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-slmconstprop -S < %s | FileCheck %s
; ------------------------------------------------
; SLMConstProp
; ------------------------------------------------
; This test checks that SLMConstProp pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_slmconst{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float addrspace(3)* [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: store float 4.0{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: store float 4.0{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: store float 4.0{{.*}}, !dbg [[STORE3_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg [[CALL_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float 4.0{{.*}}, metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float 4.0{{.*}}, metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]

define spir_kernel void @test_slmconst(float %a, float* %b) !dbg !9 {
entry:
  %0 = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17), !dbg !24
  call void @llvm.dbg.value(metadata i32 %0, metadata !12, metadata !DIExpression()), !dbg !24
  %aa = mul i32 %0, 4, !dbg !25
  call void @llvm.dbg.value(metadata i32 %aa, metadata !14, metadata !DIExpression()), !dbg !25
  %1 = inttoptr i32 %aa to float addrspace(3)*, !dbg !26
  call void @llvm.dbg.value(metadata float addrspace(3)* %1, metadata !15, metadata !DIExpression()), !dbg !26
  store float 4.000000e+00, float addrspace(3)* %1, !dbg !27
  %2 = add i32 %aa, 4, !dbg !28
  call void @llvm.dbg.value(metadata i32 %2, metadata !17, metadata !DIExpression()), !dbg !28
  %3 = inttoptr i32 %2 to float addrspace(3)*, !dbg !29
  call void @llvm.dbg.value(metadata float addrspace(3)* %3, metadata !18, metadata !DIExpression()), !dbg !29
  store float 4.000000e+00, float addrspace(3)* %3, !dbg !30
  %4 = add i32 %aa, 8, !dbg !31
  call void @llvm.dbg.value(metadata i32 %4, metadata !19, metadata !DIExpression()), !dbg !31
  %5 = inttoptr i32 %4 to float addrspace(3)*, !dbg !32
  call void @llvm.dbg.value(metadata float addrspace(3)* %5, metadata !20, metadata !DIExpression()), !dbg !32
  store float 4.000000e+00, float addrspace(3)* %5, !dbg !33
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !34
  %load1 = load float, float addrspace(3)* %1, !dbg !35
  call void @llvm.dbg.value(metadata float %load1, metadata !21, metadata !DIExpression()), !dbg !35
  %load2 = load float, float addrspace(3)* %3, !dbg !36
  call void @llvm.dbg.value(metadata float %load2, metadata !22, metadata !DIExpression()), !dbg !36
  %add1 = fadd float %load1, %load2, !dbg !37
  call void @llvm.dbg.value(metadata float %add1, metadata !23, metadata !DIExpression()), !dbg !37
  store float %add1, float* %b, !dbg !38
  ret void, !dbg !39
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "SLMConstProp.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_slmconst", linkageName: "test_slmconst", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])

declare void @llvm.genx.GenISA.threadgroupbarrier()

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (float, float*)* @test_slmconst, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "SLMConstProp.ll", directory: "/")
!5 = !{}
!6 = !{i32 16}
!7 = !{i32 10}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_slmconst", linkageName: "test_slmconst", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !17, !18, !19, !20, !21, !22, !23}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 5, type: !13)
!18 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 6, type: !16)
!19 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 8, type: !13)
!20 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 9, type: !16)
!21 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 12, type: !13)
!22 = !DILocalVariable(name: "9", scope: !9, file: !4, line: 13, type: !13)
!23 = !DILocalVariable(name: "10", scope: !9, file: !4, line: 14, type: !13)
!24 = !DILocation(line: 1, column: 1, scope: !9)
!25 = !DILocation(line: 2, column: 1, scope: !9)
!26 = !DILocation(line: 3, column: 1, scope: !9)
!27 = !DILocation(line: 4, column: 1, scope: !9)
!28 = !DILocation(line: 5, column: 1, scope: !9)
!29 = !DILocation(line: 6, column: 1, scope: !9)
!30 = !DILocation(line: 7, column: 1, scope: !9)
!31 = !DILocation(line: 8, column: 1, scope: !9)
!32 = !DILocation(line: 9, column: 1, scope: !9)
!33 = !DILocation(line: 10, column: 1, scope: !9)
!34 = !DILocation(line: 11, column: 1, scope: !9)
!35 = !DILocation(line: 12, column: 1, scope: !9)
!36 = !DILocation(line: 13, column: 1, scope: !9)
!37 = !DILocation(line: 14, column: 1, scope: !9)
!38 = !DILocation(line: 15, column: 1, scope: !9)
!39 = !DILocation(line: 16, column: 1, scope: !9)
