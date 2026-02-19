;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-fix-alignment -S < %s | FileCheck %s
; AlignmentAnalysis
; ------------------------------------------------
; This test checks that debug info is properly handled by AlignmentAnalysis pass
;
; This pass updates alignment info, check that affected IR doesn't drop debug info
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_align{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ALLOCA_V:%[A-z0-9]*]] = alloca {{.*}} !dbg [[ALLOCA_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata %opencl.image2d_t.read_only addrspace(1)** [[ALLOCA_V]], metadata [[ALLOCA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCA_LOC]]
; CHECK: store {{.*}} !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: [[ITOPTR_V:%[A-z0-9]*]] = inttoptr i64{{.*}} !dbg [[ITOPTR_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32* [[ITOPTR_V]], metadata [[ITOPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ITOPTR_LOC]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: [[GEP_V:%[A-z0-9]*]] = getelementptr{{.*}} !dbg [[GEP_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 addrspace(2)* [[GEP_V]], metadata [[GEP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP_LOC]]
; CHECK: store i32{{.*}} !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: store i32{{.*}} !dbg [[STORE3_LOC:![0-9]*]]
; CHECK: call void @llvm.memcpy{{.*}} !dbg [[CMEMC_LOC:![0-9]*]]
; CHECK: call void @llvm.memset{{.*}} !dbg [[CMEMS_LOC:![0-9]*]]

%opencl.image2d_t.read_only = type opaque
%struct._st_foo = type { i32, <4 x float>, [2 x i64] }

@b = internal addrspace(1) constant i32 0

define void @test_align(%opencl.image2d_t.read_only addrspace(1)* %input, i32* %output, i32 addrspace(2)* %sample, i32 addrspace(2)* %sample_b, %struct._st_foo* byval(%struct._st_foo) %src) !dbg !10 {
  %1 = alloca %opencl.image2d_t.read_only addrspace(1)*, !dbg !30
  call void @llvm.dbg.value(metadata %opencl.image2d_t.read_only addrspace(1)** %1, metadata !13, metadata !DIExpression()), !dbg !30
  store %opencl.image2d_t.read_only addrspace(1)* %input, %opencl.image2d_t.read_only addrspace(1)** %1, !dbg !31
  %2 = load i32, i32* %output, !dbg !32
  call void @llvm.dbg.value(metadata i32 %2, metadata !15, metadata !DIExpression()), !dbg !32
  %3 = inttoptr i64 14 to i32*, !dbg !33
  call void @llvm.dbg.value(metadata i32* %3, metadata !17, metadata !DIExpression()), !dbg !33
  %4 = load i32, i32 addrspace(1)* @b, !dbg !34
  call void @llvm.dbg.value(metadata i32 %4, metadata !18, metadata !DIExpression()), !dbg !34
  %5 = add i32 16, %2, !dbg !35
  call void @llvm.dbg.value(metadata i32 %5, metadata !19, metadata !DIExpression()), !dbg !35
  %6 = mul i32 4, %5, !dbg !36
  call void @llvm.dbg.value(metadata i32 %6, metadata !20, metadata !DIExpression()), !dbg !36
  %7 = shl i32 %5, 2, !dbg !37
  call void @llvm.dbg.value(metadata i32 %7, metadata !21, metadata !DIExpression()), !dbg !37
  %8 = and i32 %7, %4, !dbg !38
  call void @llvm.dbg.value(metadata i32 %8, metadata !22, metadata !DIExpression()), !dbg !38
  %9 = getelementptr inbounds i32, i32 addrspace(2)* %sample_b, i64 0, !dbg !39
  call void @llvm.dbg.value(metadata i32 addrspace(2)* %9, metadata !23, metadata !DIExpression()), !dbg !39
  %10 = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 0, !dbg !40
  call void @llvm.dbg.value(metadata i32* %10, metadata !24, metadata !DIExpression()), !dbg !40
  %11 = ptrtoint i32* %10 to i64, !dbg !41
  call void @llvm.dbg.value(metadata i64 %11, metadata !25, metadata !DIExpression()), !dbg !41
  %12 = trunc i64 %11 to i32, !dbg !42
  call void @llvm.dbg.value(metadata i32 %12, metadata !26, metadata !DIExpression()), !dbg !42
  store i32 %12, i32 addrspace(2)* %9, !dbg !43
  store i32 %2, i32* %3, !dbg !44
  %13 = inttoptr i8 42 to i8 addrspace(4)*, !dbg !45
  call void @llvm.dbg.value(metadata i8 addrspace(4)* %13, metadata !27, metadata !DIExpression()), !dbg !45
  %14 = addrspacecast i32* %output to i32 addrspace(4)*, !dbg !46
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %14, metadata !28, metadata !DIExpression()), !dbg !46
  %15 = bitcast i32 addrspace(4)* %14 to i8 addrspace(4)*, !dbg !47
  call void @llvm.dbg.value(metadata i8 addrspace(4)* %15, metadata !29, metadata !DIExpression()), !dbg !47
  call void @llvm.memcpy.p4i8.p4i8.i32(i8 addrspace(4)* %13, i8 addrspace(4)* %15, i32 8, i1 false), !dbg !48
  call void @llvm.memset.p4i8.i32(i8 addrspace(4)* %15, i8 0, i32 4, i1 false), !dbg !49
  ret void, !dbg !50
}


; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "AlignmentAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_align", linkageName: "test_align", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOCA_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ITOPTR_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[ITOPTR_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GEP_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[GEP_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CMEMC_LOC]] = !DILocation(line: 19, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CMEMS_LOC]] = !DILocation(line: 20, column: 1, scope: [[SCOPE]])

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p4i8.p4i8.i32(i8 addrspace(4)* noalias nocapture writeonly, i8 addrspace(4)* noalias nocapture readonly, i32, i1 immarg) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p4i8.i32(i8 addrspace(4)* nocapture writeonly, i8, i32, i1 immarg) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (%opencl.image2d_t.read_only addrspace(1)*, i32*, i32 addrspace(2)*, i32 addrspace(2)*, %struct._st_foo*)* @test_align, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "AlignmentAnalysis.ll", directory: "/")
!6 = !{}
!7 = !{i32 21}
!8 = !{i32 15}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_align", linkageName: "test_align", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 3, type: !16)
!16 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 4, type: !14)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 5, type: !16)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 6, type: !16)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 7, type: !16)
!21 = !DILocalVariable(name: "7", scope: !10, file: !5, line: 8, type: !16)
!22 = !DILocalVariable(name: "8", scope: !10, file: !5, line: 9, type: !16)
!23 = !DILocalVariable(name: "9", scope: !10, file: !5, line: 10, type: !14)
!24 = !DILocalVariable(name: "10", scope: !10, file: !5, line: 11, type: !14)
!25 = !DILocalVariable(name: "11", scope: !10, file: !5, line: 12, type: !14)
!26 = !DILocalVariable(name: "12", scope: !10, file: !5, line: 13, type: !16)
!27 = !DILocalVariable(name: "13", scope: !10, file: !5, line: 16, type: !14)
!28 = !DILocalVariable(name: "14", scope: !10, file: !5, line: 17, type: !14)
!29 = !DILocalVariable(name: "15", scope: !10, file: !5, line: 18, type: !14)
!30 = !DILocation(line: 1, column: 1, scope: !10)
!31 = !DILocation(line: 2, column: 1, scope: !10)
!32 = !DILocation(line: 3, column: 1, scope: !10)
!33 = !DILocation(line: 4, column: 1, scope: !10)
!34 = !DILocation(line: 5, column: 1, scope: !10)
!35 = !DILocation(line: 6, column: 1, scope: !10)
!36 = !DILocation(line: 7, column: 1, scope: !10)
!37 = !DILocation(line: 8, column: 1, scope: !10)
!38 = !DILocation(line: 9, column: 1, scope: !10)
!39 = !DILocation(line: 10, column: 1, scope: !10)
!40 = !DILocation(line: 11, column: 1, scope: !10)
!41 = !DILocation(line: 12, column: 1, scope: !10)
!42 = !DILocation(line: 13, column: 1, scope: !10)
!43 = !DILocation(line: 14, column: 1, scope: !10)
!44 = !DILocation(line: 15, column: 1, scope: !10)
!45 = !DILocation(line: 16, column: 1, scope: !10)
!46 = !DILocation(line: 17, column: 1, scope: !10)
!47 = !DILocation(line: 18, column: 1, scope: !10)
!48 = !DILocation(line: 19, column: 1, scope: !10)
!49 = !DILocation(line: 20, column: 1, scope: !10)
!50 = !DILocation(line: 21, column: 1, scope: !10)
