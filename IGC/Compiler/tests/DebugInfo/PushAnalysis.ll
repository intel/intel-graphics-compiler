;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-push-analysis --inputds --igc-collect-domain-shader-properties -S < %s | FileCheck %s
; ------------------------------------------------
; PushAnalysis
; ------------------------------------------------
; This test checks that PushAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; CHECK: @test_pusha{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float [[DS_V:%[A-z0-9_]*]], metadata [[DS_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DS_LOC:![0-9]*]]
; CHECK: [[UI1_V:%[A-z0-9]*]] = fptoui float [[DS_V]]{{.*}} !dbg [[UI1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[UI1_V]], metadata [[UI1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UI1_LOC]]
; CHECK: @llvm.dbg.value(metadata float [[IN_V:%[A-z0-9_]*]], metadata [[IN_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IN_LOC:![0-9]*]]
; CHECK: [[UI2_V:%[A-z0-9]*]] = fptoui float [[IN_V]]{{.*}} !dbg [[UI2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[UI2_V]], metadata [[UI2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UI2_LOC]]
; this value is essentially dead and vectors are not salvagable yet
; CHECK: @llvm.dbg.value(metadata {{.*}}, metadata [[IVEC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IVEC_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float [[PI_V:%[A-z0-9_]*]], metadata [[PI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PI_LOC:![0-9]*]]
; CHECK: [[UI3_V:%[A-z0-9]*]] = fptoui float [[PI_V]]{{.*}} !dbg [[UI3_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[UI3_V]], metadata [[UI3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UI3_LOC]]
; CHECK: @llvm.dbg.value(metadata float [[RT_V:%[A-z0-9_]*]], metadata [[RT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[RT_LOC:![0-9]*]]
; CHECK: [[UI4_V:%[A-z0-9]*]] = fptoui float [[RT_V]]{{.*}} !dbg [[UI4_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[UI4_V]], metadata [[UI4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UI4_LOC]]
; dead
; CHECK: @llvm.dbg.value(metadata {{.*}}, metadata [[ADDR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADDR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[CB_V:%[A-z0-9_]*]], metadata [[CB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CB_LOC:![0-9]*]]
; CHECK: store i32 [[CB_V]]{{.*}} [[STORE_LOC:![0-9]*]]
;

define void @test_pusha(i32* %src1) !dbg !17 {
  %1 = call float @llvm.genx.GenISA.DCL.SystemValue.f32(i32 21), !dbg !34
  call void @llvm.dbg.value(metadata float %1, metadata !20, metadata !DIExpression()), !dbg !34
  %2 = fptoui float %1 to i32, !dbg !35
  call void @llvm.dbg.value(metadata i32 %2, metadata !22, metadata !DIExpression()), !dbg !35
  store i32 %2, i32* %src1, !dbg !36
  %3 = call float @llvm.genx.GenISA.DCL.inputVec.f32(i32 13, i32 8), !dbg !37
  call void @llvm.dbg.value(metadata float %3, metadata !23, metadata !DIExpression()), !dbg !37
  %4 = fptoui float %3 to i32, !dbg !38
  call void @llvm.dbg.value(metadata i32 %4, metadata !24, metadata !DIExpression()), !dbg !38
  store i32 %4, i32* %src1, !dbg !39
  %5 = call <4 x float> @llvm.genx.GenISA.DCL.ShaderInputVec.4f32(i32 14, i32 8), !dbg !40
  call void @llvm.dbg.value(metadata <4 x float> %5, metadata !25, metadata !DIExpression()), !dbg !40
  %6 = extractelement <4 x float> %5, i32 2, !dbg !41
  call void @llvm.dbg.value(metadata float %6, metadata !27, metadata !DIExpression()), !dbg !41
  %7 = fptoui float %6 to i32, !dbg !42
  call void @llvm.dbg.value(metadata i32 %7, metadata !28, metadata !DIExpression()), !dbg !42
  store i32 %7, i32* %src1, !dbg !43
  %8 = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 16), !dbg !44
  call void @llvm.dbg.value(metadata float %8, metadata !29, metadata !DIExpression()), !dbg !44
  %9 = fptoui float %8 to i32, !dbg !45
  call void @llvm.dbg.value(metadata i32 %9, metadata !30, metadata !DIExpression()), !dbg !45
  store i32 %9, i32* %src1, !dbg !46
  %10 = inttoptr i32 16 to i32 addrspace(65536)*, !dbg !47
  call void @llvm.dbg.value(metadata i32 addrspace(65536)* %10, metadata !31, metadata !DIExpression()), !dbg !47
  %11 = load i32, i32 addrspace(65536)* %10, !dbg !48
  call void @llvm.dbg.value(metadata i32 %11, metadata !33, metadata !DIExpression()), !dbg !48
  store i32 %11, i32* %src1, !dbg !49
  ret void, !dbg !50
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PushAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_pusha", linkageName: "test_pusha", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DS_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DS_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[UI1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[UI1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IN_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[IN_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[UI2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[UI2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IVEC_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[IVEC_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PI_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[PI_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[UI3_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[UI3_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[RT_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[RT_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[UI4_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[UI4_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADDR_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[ADDR_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CB_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 15
; CHECK-DAG: [[CB_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE]])

declare float @llvm.genx.GenISA.DCL.SystemValue.f32(i32)

declare float @llvm.genx.GenISA.DCL.inputVec.f32(i32, i32)

declare <4 x float> @llvm.genx.GenISA.DCL.ShaderInputVec.4f32(i32, i32)

declare float @llvm.genx.GenISA.RuntimeValue.f32(i32)

declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.p2555904.v4f32(<4 x float> addrspace(2555904)*, i32, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!llvm.dbg.cu = !{!11}
!llvm.debugify = !{!14, !15}
!llvm.module.flags = !{!16}

!0 = !{void (i32*)* @test_pusha, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5, !7}
!5 = !{!"compOpt", !6}
!6 = !{!"PushConstantsEnable", i1 true}
!7 = !{!"pushInfo", !8}
!8 = !{!"bindlessPushInfo", !9, !10}
!9 = !{!"bindlessPushInfoVec[0]", i32 0}
!10 = !{!"bindlessPushInfoVec[1]", i32 17}
!11 = distinct !DICompileUnit(language: DW_LANG_C, file: !12, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !13)
!12 = !DIFile(filename: "PushAnalysis.ll", directory: "/")
!13 = !{}
!14 = !{i32 17}
!15 = !{i32 11}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = distinct !DISubprogram(name: "test_pusha", linkageName: "test_pusha", scope: null, file: !12, line: 1, type: !18, scopeLine: 1, unit: !11, retainedNodes: !19)
!18 = !DISubroutineType(types: !13)
!19 = !{!20, !22, !23, !24, !25, !27, !28, !29, !30, !31, !33}
!20 = !DILocalVariable(name: "1", scope: !17, file: !12, line: 1, type: !21)
!21 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "2", scope: !17, file: !12, line: 2, type: !21)
!23 = !DILocalVariable(name: "3", scope: !17, file: !12, line: 4, type: !21)
!24 = !DILocalVariable(name: "4", scope: !17, file: !12, line: 5, type: !21)
!25 = !DILocalVariable(name: "5", scope: !17, file: !12, line: 7, type: !26)
!26 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!27 = !DILocalVariable(name: "6", scope: !17, file: !12, line: 8, type: !21)
!28 = !DILocalVariable(name: "7", scope: !17, file: !12, line: 9, type: !21)
!29 = !DILocalVariable(name: "8", scope: !17, file: !12, line: 11, type: !21)
!30 = !DILocalVariable(name: "9", scope: !17, file: !12, line: 12, type: !21)
!31 = !DILocalVariable(name: "10", scope: !17, file: !12, line: 14, type: !32)
!32 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!33 = !DILocalVariable(name: "11", scope: !17, file: !12, line: 15, type: !21)
!34 = !DILocation(line: 1, column: 1, scope: !17)
!35 = !DILocation(line: 2, column: 1, scope: !17)
!36 = !DILocation(line: 3, column: 1, scope: !17)
!37 = !DILocation(line: 4, column: 1, scope: !17)
!38 = !DILocation(line: 5, column: 1, scope: !17)
!39 = !DILocation(line: 6, column: 1, scope: !17)
!40 = !DILocation(line: 7, column: 1, scope: !17)
!41 = !DILocation(line: 8, column: 1, scope: !17)
!42 = !DILocation(line: 9, column: 1, scope: !17)
!43 = !DILocation(line: 10, column: 1, scope: !17)
!44 = !DILocation(line: 11, column: 1, scope: !17)
!45 = !DILocation(line: 12, column: 1, scope: !17)
!46 = !DILocation(line: 13, column: 1, scope: !17)
!47 = !DILocation(line: 14, column: 1, scope: !17)
!48 = !DILocation(line: 15, column: 1, scope: !17)
!49 = !DILocation(line: 16, column: 1, scope: !17)
!50 = !DILocation(line: 17, column: 1, scope: !17)
