;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -loop-gating -S < %s | FileCheck %s
; ------------------------------------------------
; GatingSimilarSamples
; ------------------------------------------------
; This test checks that GatingSimilarSamples pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x float> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x float> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: [[VAL8_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]

; CHECK-DAG: void @llvm.dbg.value(metadata <4 x float> [[VAL9_V:%[A-z0-9]*]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: [[VAL9_V]] = {{.*}}, !dbg [[VAL9_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL10_V:%[A-z0-9]*]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC:![0-9]*]]
; CHECK-DAG: [[VAL10_V]] = {{.*}}, !dbg [[VAL10_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL11_V:%[A-z0-9]*]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC:![0-9]*]]
; CHECK-DAG: [[VAL11_V]] = {{.*}}, !dbg [[VAL11_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL12_V:%[A-z0-9]*]], metadata [[VAL12_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL12_LOC:![0-9]*]]
; CHECK-DAG: [[VAL12_V]] = {{.*}}, !dbg [[VAL12_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL13_V:%[A-z0-9]*]], metadata [[VAL13_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL13_LOC:![0-9]*]]
; CHECK-DAG: [[VAL13_V]] = {{.*}}, !dbg [[VAL13_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL14_V:%[A-z0-9]*]], metadata [[VAL14_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL14_LOC:![0-9]*]]
; CHECK-DAG: [[VAL14_V]] = {{.*}}, !dbg [[VAL14_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL15_V:%[A-z0-9]*]], metadata [[VAL15_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL15_LOC:![0-9]*]]
; CHECK-DAG: [[VAL15_V]] = {{.*}}, !dbg [[VAL15_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL16_V:%[A-z0-9]*]], metadata [[VAL16_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL16_LOC:![0-9]*]]
; CHECK-DAG: [[VAL16_V]] = {{.*}}, !dbg [[VAL16_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL17_V:%[A-z0-9]*]], metadata [[VAL17_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL17_LOC:![0-9]*]]
; CHECK-DAG: [[VAL17_V]] = {{.*}}, !dbg [[VAL17_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL18_V:%[A-z0-9]*]], metadata [[VAL18_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL18_LOC:![0-9]*]]
; CHECK-DAG: [[VAL18_V]] = {{.*}}, !dbg [[VAL18_LOC]]

define void @test(float %src1, float %src2, float %src3, float* %dst) !dbg !6 {
  %mul.1 = fmul float %src1, %src2, !dbg !29
  call void @llvm.dbg.value(metadata float %mul.1, metadata !9, metadata !DIExpression()), !dbg !29
  %a = fsub float %src1, %mul.1, !dbg !30
  call void @llvm.dbg.value(metadata float %a, metadata !11, metadata !DIExpression()), !dbg !30
  %b = fadd float %src1, %mul.1, !dbg !31
  call void @llvm.dbg.value(metadata float %b, metadata !12, metadata !DIExpression()), !dbg !31
  %1 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %src1, float %src2, float 0.000000e+00, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0), !dbg !32
  call void @llvm.dbg.value(metadata <4 x float> %1, metadata !13, metadata !DIExpression()), !dbg !32
  %2 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %b, float %a, float %src3, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0), !dbg !33
  call void @llvm.dbg.value(metadata <4 x float> %2, metadata !15, metadata !DIExpression()), !dbg !33
  %3 = extractelement <4 x float> %2, i32 0, !dbg !34
  call void @llvm.dbg.value(metadata float %3, metadata !16, metadata !DIExpression()), !dbg !34
  %4 = extractelement <4 x float> %2, i32 1, !dbg !35
  call void @llvm.dbg.value(metadata float %4, metadata !17, metadata !DIExpression()), !dbg !35
  %5 = extractelement <4 x float> %2, i32 2, !dbg !36
  call void @llvm.dbg.value(metadata float %5, metadata !18, metadata !DIExpression()), !dbg !36
  %6 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float %a, float %b, float %src3, float 1.000000e+00, float %src2, float %src3, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0), !dbg !37
  call void @llvm.dbg.value(metadata <4 x float> %6, metadata !19, metadata !DIExpression()), !dbg !37
  %7 = extractelement <4 x float> %6, i32 0, !dbg !38
  call void @llvm.dbg.value(metadata float %7, metadata !20, metadata !DIExpression()), !dbg !38
  %8 = extractelement <4 x float> %6, i32 1, !dbg !39
  call void @llvm.dbg.value(metadata float %8, metadata !21, metadata !DIExpression()), !dbg !39
  %9 = extractelement <4 x float> %6, i32 2, !dbg !40
  call void @llvm.dbg.value(metadata float %9, metadata !22, metadata !DIExpression()), !dbg !40
  %10 = fadd float %3, %7, !dbg !41
  call void @llvm.dbg.value(metadata float %10, metadata !23, metadata !DIExpression()), !dbg !41
  %11 = fmul float %10, 5.000000e-01, !dbg !42
  call void @llvm.dbg.value(metadata float %11, metadata !24, metadata !DIExpression()), !dbg !42
  %12 = fadd float %4, %8, !dbg !43
  call void @llvm.dbg.value(metadata float %12, metadata !25, metadata !DIExpression()), !dbg !43
  %13 = fmul float %12, 5.000000e-01, !dbg !44
  call void @llvm.dbg.value(metadata float %13, metadata !26, metadata !DIExpression()), !dbg !44
  %14 = fadd float %5, %9, !dbg !45
  call void @llvm.dbg.value(metadata float %14, metadata !27, metadata !DIExpression()), !dbg !45
  %15 = fmul float %14, 5.000000e-01, !dbg !46
  call void @llvm.dbg.value(metadata float %15, metadata !28, metadata !DIExpression()), !dbg !46
  call void @llvm.genx.GenISA.OUTPUT.f32(float %11, float %13, float %15, float 0.000000e+00, i32 1, i32 1), !dbg !47
  ret void, !dbg !48
}


; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GatingSimilarSamples.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL12_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL12_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL13_MD]] = !DILocalVariable(name: "13", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL13_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL14_MD]] = !DILocalVariable(name: "14", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL14_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL15_MD]] = !DILocalVariable(name: "15", scope: [[SCOPE]], file: [[FILE]], line: 15
; CHECK-DAG: [[VAL15_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL16_MD]] = !DILocalVariable(name: "16", scope: [[SCOPE]], file: [[FILE]], line: 16
; CHECK-DAG: [[VAL16_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL17_MD]] = !DILocalVariable(name: "17", scope: [[SCOPE]], file: [[FILE]], line: 17
; CHECK-DAG: [[VAL17_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL18_MD]] = !DILocalVariable(name: "18", scope: [[SCOPE]], file: [[FILE]], line: 18
; CHECK-DAG: [[VAL18_LOC]] = !DILocation(line: 18, column: 1, scope: [[SCOPE]])

declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.p196609i8.p524293i8(float, float, float, float, float, float, i8 addrspace(196609)*, i8 addrspace(524293)*, i32, i32, i32)

declare void @llvm.genx.GenISA.OUTPUT.f32(float, float, float, float, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GatingSimilarSamples.ll", directory: "/")
!2 = !{}
!3 = !{i32 20}
!4 = !{i32 18}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!14 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !14)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !14)
!20 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !10)
!21 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !10)
!22 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !10)
!23 = !DILocalVariable(name: "13", scope: !6, file: !1, line: 13, type: !10)
!24 = !DILocalVariable(name: "14", scope: !6, file: !1, line: 14, type: !10)
!25 = !DILocalVariable(name: "15", scope: !6, file: !1, line: 15, type: !10)
!26 = !DILocalVariable(name: "16", scope: !6, file: !1, line: 16, type: !10)
!27 = !DILocalVariable(name: "17", scope: !6, file: !1, line: 17, type: !10)
!28 = !DILocalVariable(name: "18", scope: !6, file: !1, line: 18, type: !10)
!29 = !DILocation(line: 1, column: 1, scope: !6)
!30 = !DILocation(line: 2, column: 1, scope: !6)
!31 = !DILocation(line: 3, column: 1, scope: !6)
!32 = !DILocation(line: 4, column: 1, scope: !6)
!33 = !DILocation(line: 5, column: 1, scope: !6)
!34 = !DILocation(line: 6, column: 1, scope: !6)
!35 = !DILocation(line: 7, column: 1, scope: !6)
!36 = !DILocation(line: 8, column: 1, scope: !6)
!37 = !DILocation(line: 9, column: 1, scope: !6)
!38 = !DILocation(line: 10, column: 1, scope: !6)
!39 = !DILocation(line: 11, column: 1, scope: !6)
!40 = !DILocation(line: 12, column: 1, scope: !6)
!41 = !DILocation(line: 13, column: 1, scope: !6)
!42 = !DILocation(line: 14, column: 1, scope: !6)
!43 = !DILocation(line: 15, column: 1, scope: !6)
!44 = !DILocation(line: 16, column: 1, scope: !6)
!45 = !DILocation(line: 17, column: 1, scope: !6)
!46 = !DILocation(line: 18, column: 1, scope: !6)
!47 = !DILocation(line: 19, column: 1, scope: !6)
!48 = !DILocation(line: 20, column: 1, scope: !6)
