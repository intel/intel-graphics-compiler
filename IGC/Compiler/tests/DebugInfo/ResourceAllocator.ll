;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-resource-allocator -S < %s | FileCheck %s
; ------------------------------------------------
; ResourceAllocator
; ------------------------------------------------
; This test checks that ResourceAllocator pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; This pass updates Metadata, check that debug info is not affected

; CHECK: @test_ress{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ALLOCA_V:%[A-z0-9]*]] = alloca {{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata %opencl.image2d_t.read_only addrspace(1)** [[ALLOCA_V]], metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: [[LOAD3_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD3_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD3_V]], metadata [[LOAD3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD3_LOC]]
; CHECK: [[ADD1_V:%[A-z0-9]*]] = add i32{{.*}} !dbg [[ADD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD1_V]], metadata [[ADD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD1_LOC]]
; CHECK: [[ADD2_V:%[A-z0-9]*]] = add i32{{.*}} !dbg [[ADD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD2_V]], metadata [[ADD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD2_LOC]]
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]

%opencl.image2d_t.read_only = type opaque

define void @test_ress(%opencl.image2d_t.read_only addrspace(1)* %input, i32* %output, i32 addrspace(2)* %sample, i32 addrspace(2)* %sample_b) !dbg !24 {
  %1 = alloca %opencl.image2d_t.read_only addrspace(1)*, align 4, !dbg !35
  call void @llvm.dbg.value(metadata %opencl.image2d_t.read_only addrspace(1)** %1, metadata !27, metadata !DIExpression()), !dbg !35
  store %opencl.image2d_t.read_only addrspace(1)* %input, %opencl.image2d_t.read_only addrspace(1)** %1, !dbg !36
  %2 = load i32, i32* %output, !dbg !37
  call void @llvm.dbg.value(metadata i32 %2, metadata !29, metadata !DIExpression()), !dbg !37
  %3 = load i32, i32 addrspace(2)* %sample, !dbg !38
  call void @llvm.dbg.value(metadata i32 %3, metadata !31, metadata !DIExpression()), !dbg !38
  %4 = load i32, i32 addrspace(2)* %sample_b, !dbg !39
  call void @llvm.dbg.value(metadata i32 %4, metadata !32, metadata !DIExpression()), !dbg !39
  %5 = add i32 %2, %3, !dbg !40
  call void @llvm.dbg.value(metadata i32 %5, metadata !33, metadata !DIExpression()), !dbg !40
  %6 = add i32 %5, %4, !dbg !41
  call void @llvm.dbg.value(metadata i32 %6, metadata !34, metadata !DIExpression()), !dbg !41
  store i32 %6, i32* %output, !dbg !42
  ret void, !dbg !43
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ResourceAllocator.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_ress", linkageName: "test_ress", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD3_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LOAD3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD1_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[ADD1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD2_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[ADD2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!llvm.dbg.cu = !{!18}
!llvm.debugify = !{!21, !22}
!llvm.module.flags = !{!23}

!0 = !{void (%opencl.image2d_t.read_only addrspace(1)*, i32*, i32 addrspace(2)*, i32 addrspace(2)*)* @test_ress, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", void (%opencl.image2d_t.read_only addrspace(1)*, i32*, i32 addrspace(2)*, i32 addrspace(2)*)* @test_ress}
!7 = !{!"FuncMDValue[0]", !8, !13}
!8 = !{!"resAllocMD", !9, !10, !11, !12}
!9 = !{!"uavsNumType", i32 2}
!10 = !{!"srvsNumType", i32 1}
!11 = !{!"samplersNumType", i32 0}
!12 = !{!"argAllocMDList"}
!13 = !{!"m_OpenCLArgBaseTypes", !14, !15, !16, !17}
!14 = !{!"m_OpenCLArgBaseTypesVec[0]", !"image2d_t"}
!15 = !{!"m_OpenCLArgBaseTypesVec[1]", !""}
!16 = !{!"m_OpenCLArgBaseTypesVec[2]", !"sampler_t"}
!17 = !{!"m_OpenCLArgBaseTypesVec[3]", !"bindless_sampler_t"}
!18 = distinct !DICompileUnit(language: DW_LANG_C, file: !19, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !20)
!19 = !DIFile(filename: "ResourceAllocator.ll", directory: "/")
!20 = !{}
!21 = !{i32 9}
!22 = !{i32 6}
!23 = !{i32 2, !"Debug Info Version", i32 3}
!24 = distinct !DISubprogram(name: "test_ress", linkageName: "test_ress", scope: null, file: !19, line: 1, type: !25, scopeLine: 1, unit: !18, retainedNodes: !26)
!25 = !DISubroutineType(types: !20)
!26 = !{!27, !29, !31, !32, !33, !34}
!27 = !DILocalVariable(name: "1", scope: !24, file: !19, line: 1, type: !28)
!28 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!29 = !DILocalVariable(name: "2", scope: !24, file: !19, line: 3, type: !30)
!30 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!31 = !DILocalVariable(name: "3", scope: !24, file: !19, line: 4, type: !30)
!32 = !DILocalVariable(name: "4", scope: !24, file: !19, line: 5, type: !30)
!33 = !DILocalVariable(name: "5", scope: !24, file: !19, line: 6, type: !30)
!34 = !DILocalVariable(name: "6", scope: !24, file: !19, line: 7, type: !30)
!35 = !DILocation(line: 1, column: 1, scope: !24)
!36 = !DILocation(line: 2, column: 1, scope: !24)
!37 = !DILocation(line: 3, column: 1, scope: !24)
!38 = !DILocation(line: 4, column: 1, scope: !24)
!39 = !DILocation(line: 5, column: 1, scope: !24)
!40 = !DILocation(line: 6, column: 1, scope: !24)
!41 = !DILocation(line: 7, column: 1, scope: !24)
!42 = !DILocation(line: 8, column: 1, scope: !24)
!43 = !DILocation(line: 9, column: 1, scope: !24)
