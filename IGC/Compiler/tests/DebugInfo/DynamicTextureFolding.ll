;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-dynamic-texture-folding -S < %s | FileCheck %s
; ------------------------------------------------
; DynamicTextureFolding
; ------------------------------------------------
; This test checks that DynamicTextureFolding pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x float> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: void @llvm.dbg.value(metadata float {{.*}}, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float {{.*}}, metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STR2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float {{.*}}, metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STR3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float {{.*}}, metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STR4_LOC:![0-9]*]]


define void @test(float %src1, float %src2, float %src3, float* %dst) !dbg !11 {
  %1 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %src1, float %src2, float %src3, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0), !dbg !21
  call void @llvm.dbg.value(metadata <4 x float> %1, metadata !14, metadata !DIExpression()), !dbg !21
  %2 = extractelement <4 x float> %1, i32 0, !dbg !22
  call void @llvm.dbg.value(metadata float %2, metadata !16, metadata !DIExpression()), !dbg !22
  store float %2, float* %dst, !dbg !23
  %3 = extractelement <4 x float> %1, i32 1, !dbg !24
  call void @llvm.dbg.value(metadata float %3, metadata !18, metadata !DIExpression()), !dbg !24
  store float %3, float* %dst, !dbg !25
  %4 = extractelement <4 x float> %1, i32 2, !dbg !26
  call void @llvm.dbg.value(metadata float %4, metadata !19, metadata !DIExpression()), !dbg !26
  store float %4, float* %dst, !dbg !27
  %5 = extractelement <4 x float> %1, i32 3, !dbg !28
  call void @llvm.dbg.value(metadata float %5, metadata !20, metadata !DIExpression()), !dbg !28
  store float %5, float* %dst, !dbg !29
  ret void, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "DynamicTextureFolding.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR4_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])

declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float, float, float, float, float, float, i8 addrspace(196609)*, i8 addrspace(524293)*, i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!5}
!llvm.debugify = !{!8, !9}
!llvm.module.flags = !{!10}

!0 = !{!"ModuleMD", !1}
!1 = !{!"inlineDynTextures", !2, !3}
!2 = !{!"inlineDynTexturesMap[0]", i32 1}
!3 = !{!"inlineDynTexturesValue[0]", !4}
!4 = !{i32 0, i32 2, i32 1, i32 1}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !6, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !7)
!6 = !DIFile(filename: "DynamicTextureFolding.ll", directory: "/")
!7 = !{}
!8 = !{i32 10}
!9 = !{i32 5}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !6, line: 1, type: !12, scopeLine: 1, unit: !5, retainedNodes: !13)
!12 = !DISubroutineType(types: !7)
!13 = !{!14, !16, !18, !19, !20}
!14 = !DILocalVariable(name: "1", scope: !11, file: !6, line: 1, type: !15)
!15 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "2", scope: !11, file: !6, line: 2, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "3", scope: !11, file: !6, line: 4, type: !17)
!19 = !DILocalVariable(name: "4", scope: !11, file: !6, line: 6, type: !17)
!20 = !DILocalVariable(name: "5", scope: !11, file: !6, line: 8, type: !17)
!21 = !DILocation(line: 1, column: 1, scope: !11)
!22 = !DILocation(line: 2, column: 1, scope: !11)
!23 = !DILocation(line: 3, column: 1, scope: !11)
!24 = !DILocation(line: 4, column: 1, scope: !11)
!25 = !DILocation(line: 5, column: 1, scope: !11)
!26 = !DILocation(line: 6, column: 1, scope: !11)
!27 = !DILocation(line: 7, column: 1, scope: !11)
!28 = !DILocation(line: 8, column: 1, scope: !11)
!29 = !DILocation(line: 9, column: 1, scope: !11)
!30 = !DILocation(line: 10, column: 1, scope: !11)
