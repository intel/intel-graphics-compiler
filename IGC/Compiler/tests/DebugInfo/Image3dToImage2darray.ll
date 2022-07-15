;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-3d-to-2darray -S < %s | FileCheck %s
; ------------------------------------------------
; Image3dToImage2darray
; ------------------------------------------------
; This test checks that Image3dToImage2darray pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_i3dto2da{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ALLOCA_V:%[A-z0-9]*]] = alloca {{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <16 x i32>* [[ALLOCA_V]], metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: [[BCAST1_V:%[A-z0-9]*]] = bitcast i32*{{.*}} !dbg [[BCAST1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i8* [[BCAST1_V]], metadata [[BCAST1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST1_LOC]]
; CHECK: [[ITOPTR_V:%[A-z0-9]*]] = inttoptr i32{{.*}} !dbg [[ITOPTR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i8* [[ITOPTR_V]], metadata [[ITOPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ITOPTR_LOC]]
; CHECK: [[CALL1_V:%[A-z0-9]*]] = call <16 x i32>{{.*}} !dbg [[CALL1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <16 x i32> [[CALL1_V]], metadata [[CALL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL1_LOC]]
; CHECK: [[CALL2_V:%[A-z0-9]*]] = call <16 x i32>{{.*}} !dbg [[CALL2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <16 x i32> [[CALL2_V]], metadata [[CALL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL2_LOC]]
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: store {{.*}} !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: [[ALLOCAF_V:%[A-z0-9]*]] = alloca {{.*}} !dbg [[ALLOCAF_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <4 x float>* [[ALLOCAF_V]], metadata [[ALLOCAF_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCAF_LOC]]
; CHECK: [[BCAST2_V:%[A-z0-9]*]] = bitcast i32*{{.*}} !dbg [[BCAST2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float* [[BCAST2_V]], metadata [[BCAST2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST2_LOC]]
; CHECK: [[CALL3_V:%[A-z0-9]*]] = call <4 x float>{{.*}} !dbg [[CALL3_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <4 x float> [[CALL3_V]], metadata [[CALL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL3_LOC]]

define void @test_i3dto2da(i32* %a) !dbg !34 {
  %1 = alloca <16 x i32>, align 4, !dbg !48
  call void @llvm.dbg.value(metadata <16 x i32>* %1, metadata !37, metadata !DIExpression()), !dbg !48
  %2 = bitcast i32* %a to i8*, !dbg !49
  call void @llvm.dbg.value(metadata i8* %2, metadata !39, metadata !DIExpression()), !dbg !49
  %3 = inttoptr i32 42 to i8*, !dbg !50
  call void @llvm.dbg.value(metadata i8* %3, metadata !40, metadata !DIExpression()), !dbg !50
  %4 = call <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32 1, i32 2, i32 3, i32 4, i8* %2, i8* %2, i32 0, i32 1, i32 2), !dbg !51
  call void @llvm.dbg.value(metadata <16 x i32> %4, metadata !41, metadata !DIExpression()), !dbg !51
  %5 = call <16 x i32> @llvm.genx.GenISA.ldptr.16i32.const(i32 1, i32 2, i32 3, i32 4, i8 42, i8 42, i32 0, i32 1, i32 2), !dbg !52
  call void @llvm.dbg.value(metadata <16 x i32> %5, metadata !43, metadata !DIExpression()), !dbg !52
  store <16 x i32> %4, <16 x i32>* %1, !dbg !53
  store <16 x i32> %5, <16 x i32>* %1, !dbg !54
  %6 = alloca <4 x float>, align 4, !dbg !55
  call void @llvm.dbg.value(metadata <4 x float>* %6, metadata !44, metadata !DIExpression()), !dbg !55
  %7 = bitcast i32* %a to float*, !dbg !56
  call void @llvm.dbg.value(metadata float* %7, metadata !45, metadata !DIExpression()), !dbg !56
  %8 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.f32.i32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float* %7, i32* %a, i32 1, i32 2, i32 3), !dbg !57
  call void @llvm.dbg.value(metadata <4 x float> %8, metadata !46, metadata !DIExpression()), !dbg !57
  ret void, !dbg !58
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "Image3dToImage2darray.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_i3dto2da", linkageName: "test_i3dto2da", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[BCAST1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ITOPTR_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[ITOPTR_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL1_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL2_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[CALL2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOCAF_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[ALLOCAF_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST2_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[BCAST2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL3_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[CALL3_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

declare <16 x i32> @llvm.genx.GenISA.ldptr.16i32.const(i32, i32, i32, i32, i8, i8, i32, i32, i32)

declare <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32, i32, i32, i32, i8*, i8*, i32, i32, i32)

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.f32.i32(float, float, float, float, float, float*, i32*, i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!llvm.dbg.cu = !{!28}
!llvm.debugify = !{!31, !32}
!llvm.module.flags = !{!33}

!0 = !{void (i32*)* @test_i3dto2da, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (i32*)* @test_i3dto2da}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"resAllocMD", !9, !10}
!9 = !{!"samplersNumType", i32 0}
!10 = !{!"argAllocMDList", !11, !15, !17, !21, !22, !23, !24, !25, !26, !27}
!11 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!12 = !{!"type", i32 2}
!13 = !{!"extensionType", i32 0}
!14 = !{!"indexType", i32 42}
!15 = !{!"argAllocMDListVec[1]", !16, !13, !14}
!16 = !{!"type", i32 1}
!17 = !{!"argAllocMDListVec[2]", !18, !19, !20}
!18 = !{!"type", i32 4}
!19 = !{!"extensionType", i32 -1}
!20 = !{!"indexType", i32 5}
!21 = !{!"argAllocMDListVec[3]", !18, !19, !20}
!22 = !{!"argAllocMDListVec[4]", !18, !19, !20}
!23 = !{!"argAllocMDListVec[5]", !18, !19, !20}
!24 = !{!"argAllocMDListVec[6]", !18, !19, !20}
!25 = !{!"argAllocMDListVec[7]", !18, !19, !20}
!26 = !{!"argAllocMDListVec[8]", !18, !19, !20}
!27 = !{!"argAllocMDListVec[9]", !16, !19, !14}
!28 = distinct !DICompileUnit(language: DW_LANG_C, file: !29, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !30)
!29 = !DIFile(filename: "Image3dToImage2darray.ll", directory: "/")
!30 = !{}
!31 = !{i32 11}
!32 = !{i32 8}
!33 = !{i32 2, !"Debug Info Version", i32 3}
!34 = distinct !DISubprogram(name: "test_i3dto2da", linkageName: "test_i3dto2da", scope: null, file: !29, line: 1, type: !35, scopeLine: 1, unit: !28, retainedNodes: !36)
!35 = !DISubroutineType(types: !30)
!36 = !{!37, !39, !40, !41, !43, !44, !45, !46}
!37 = !DILocalVariable(name: "1", scope: !34, file: !29, line: 1, type: !38)
!38 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!39 = !DILocalVariable(name: "2", scope: !34, file: !29, line: 2, type: !38)
!40 = !DILocalVariable(name: "3", scope: !34, file: !29, line: 3, type: !38)
!41 = !DILocalVariable(name: "4", scope: !34, file: !29, line: 4, type: !42)
!42 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!43 = !DILocalVariable(name: "5", scope: !34, file: !29, line: 5, type: !42)
!44 = !DILocalVariable(name: "6", scope: !34, file: !29, line: 8, type: !38)
!45 = !DILocalVariable(name: "7", scope: !34, file: !29, line: 9, type: !38)
!46 = !DILocalVariable(name: "8", scope: !34, file: !29, line: 10, type: !47)
!47 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!48 = !DILocation(line: 1, column: 1, scope: !34)
!49 = !DILocation(line: 2, column: 1, scope: !34)
!50 = !DILocation(line: 3, column: 1, scope: !34)
!51 = !DILocation(line: 4, column: 1, scope: !34)
!52 = !DILocation(line: 5, column: 1, scope: !34)
!53 = !DILocation(line: 6, column: 1, scope: !34)
!54 = !DILocation(line: 7, column: 1, scope: !34)
!55 = !DILocation(line: 8, column: 1, scope: !34)
!56 = !DILocation(line: 9, column: 1, scope: !34)
!57 = !DILocation(line: 10, column: 1, scope: !34)
!58 = !DILocation(line: 11, column: 1, scope: !34)
