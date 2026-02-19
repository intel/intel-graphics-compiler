;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-lower-gp-arg -S < %s | FileCheck %s
; ------------------------------------------------
; LowerGPCallArg
; ------------------------------------------------
; This test checks that LowerGPCallArg pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_kernel
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32{{.*}} %
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SRC_LOC:![0-9]*]]
; CHECK: [[DST_V:%[0-9]*]] = addrspacecast
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float{{.*}} [[DST_V]]
; CHECK-SAME: metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC]]
; CHECK: [[FOO_V:%[A-z0-9_]*]] = call {{.*}} @test_foo
; CHECK-SAME: !dbg [[FOO_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float{{.*}} [[FOO_V]]
; CHECK-SAME: metadata [[FOO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FOO_LOC]]
; CHECK: [[BAR_V:%[A-z0-9_]*]] = call {{.*}} @test_bar
; CHECK-SAME: !dbg [[BAR_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32{{.*}} [[BAR_V]]
; CHECK-SAME: metadata [[BAR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BAR_LOC]]

define spir_kernel void @test_kernel(i32 addrspace(1)* %src, float addrspace(1)* %dst) !dbg !15 {
  %1 = addrspacecast i32 addrspace(1)* %src to i32 addrspace(4)*, !dbg !25
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %1, metadata !18, metadata !DIExpression()), !dbg !25
  %2 = addrspacecast float addrspace(1)* %dst to float addrspace(4)*, !dbg !26
  call void @llvm.dbg.value(metadata float addrspace(4)* %2, metadata !20, metadata !DIExpression()), !dbg !26
  %3 = call float @test_foo(i32 addrspace(4)* %1, float addrspace(4)* %2), !dbg !27
  call void @llvm.dbg.value(metadata float %3, metadata !21, metadata !DIExpression()), !dbg !27
  %4 = bitcast float addrspace(4)* %2 to i32 addrspace(4)*, !dbg !28
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %4, metadata !23, metadata !DIExpression()), !dbg !28
  %5 = call i32 @test_bar(i32 addrspace(4)* %1, i32 addrspace(4)* %4), !dbg !29
  call void @llvm.dbg.value(metadata i32 %5, metadata !24, metadata !DIExpression()), !dbg !29
  store i32 %5, i32 addrspace(1)* %src, !dbg !30
  store float %3, float addrspace(1)* %dst, !dbg !31
  ret void, !dbg !32
}

; CHECK: define spir_func i32 @test_bar
; CHECK-SAME: !dbg [[BAR_SCOPE:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 {{.*}} [[BGP1_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[BGP1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BGP1_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 {{.*}} [[BGP2_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[BGP2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BGP2_LOC:![0-9]*]]
; CHECK: [[BGP_V:%[0-9]*]] = load
; CHECK-SAME: !dbg [[BGP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[BGP_V]]
; CHECK-SAME: metadata [[BGP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BGP_LOC]]

define spir_func i32 @test_bar(i32 addrspace(4)* %gp1, i32 addrspace(4)* %gp2) !dbg !33 {
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %gp1, metadata !36, metadata !DIExpression()), !dbg !37
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %gp2, metadata !38, metadata !DIExpression()), !dbg !39
  %1 = load i32, i32 addrspace(4)* %gp1, !dbg !40
  call void @llvm.dbg.value(metadata i32 %1, metadata !35, metadata !DIExpression()), !dbg !40
  store i32 %1, i32 addrspace(4)* %gp2, !dbg !41
  ret i32 %1, !dbg !42
}

; CHECK: define spir_func float @test_foo
; CHECK-SAME: !dbg [[FOO_SCOPE:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 {{.*}} [[FGP1_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[FGP1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FGP1_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata float {{.*}} [[FGP2_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[FGP2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FGP2_LOC:![0-9]*]]
; CHECK: [[FGP_V:%[0-9]*]] = load
; CHECK-SAME: !dbg [[FGP_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: metadata i32 [[FGP_V]]
; CHECK-SAME: metadata [[FGP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FGP_LOC]]

define spir_func float @test_foo(i32 addrspace(4)* %gp1, float addrspace(4)* %gp2) !dbg !43 {
  call void @llvm.dbg.value(metadata i32 addrspace(4)* %gp1, metadata !47, metadata !DIExpression()), !dbg !48
  call void @llvm.dbg.value(metadata float addrspace(4)* %gp2, metadata !49, metadata !DIExpression()), !dbg !50
  %1 = load i32, i32 addrspace(4)* %gp1, !dbg !51
  call void @llvm.dbg.value(metadata i32 %1, metadata !45, metadata !DIExpression()), !dbg !51
  %2 = bitcast i32 %1 to float, !dbg !52
  call void @llvm.dbg.value(metadata float %2, metadata !46, metadata !DIExpression()), !dbg !52
  store float %2, float addrspace(4)* %gp2, !dbg !53
  ret float %2, !dbg !54
}
; Kernel
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "LowerGPCallArg.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !10, line: 1
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FOO_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[FOO_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BAR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[BAR_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
;
; bar
; CHECK-DAG: [[BAR_SCOPE]] = distinct !DISubprogram(name: "test_bar", linkageName: "test_bar", scope: null, file: [[FILE]], line: 9
; CHECK-DAG: [[BGP1_MD]] = !DILocalVariable(name: "gp1", scope: [[BAR_SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[BGP1_LOC]] = !DILocation(line: 9, column: 11, scope: [[BAR_SCOPE]])
; CHECK-DAG: [[BGP2_MD]] = !DILocalVariable(name: "gp2", scope: [[BAR_SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[BGP2_LOC]] = !DILocation(line: 9, column: 21, scope: [[BAR_SCOPE]])
; CHECK-DAG: [[BGP_MD]] = !DILocalVariable(name: "6", scope: [[BAR_SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[BGP_LOC]] = !DILocation(line: 9, column: 1, scope: [[BAR_SCOPE]])
;
; foo
; CHECK-DAG: [[FOO_SCOPE]] = distinct !DISubprogram(name: "test_foo", linkageName: "test_foo", scope: null, file: [[FILE]], line: 12
; CHECK-DAG: [[FGP1_MD]] = !DILocalVariable(name: "fgp1", scope: [[FOO_SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[FGP1_LOC]] = !DILocation(line: 12, column: 11, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[FGP2_MD]] = !DILocalVariable(name: "fgp2", scope: [[FOO_SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[FGP2_LOC]] = !DILocation(line: 12, column: 21, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[FGP_MD]] = !DILocalVariable(name: "7", scope: [[FOO_SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[FGP_LOC]] = !DILocation(line: 12, column: 1, scope: [[FOO_SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0, !4, !7}
!llvm.dbg.cu = !{!9}
!llvm.debugify = !{!12, !13}
!llvm.module.flags = !{!14}

!0 = !{void (i32 addrspace(1)*, float addrspace(1)*)* @test_kernel, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{float (i32 addrspace(4)*, float addrspace(4)*)* @test_foo, !5}
!5 = distinct !{!5, !6}
!6 = !{!"function_type", i32 1}
!7 = !{i32 (i32 addrspace(4)*, i32 addrspace(4)*)* @test_bar, !8}
!8 = !{!6, !3}
!9 = distinct !DICompileUnit(language: DW_LANG_C, file: !10, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !11)
!10 = !DIFile(filename: "LowerGPCallArg.ll", directory: "/")
!11 = !{}
!12 = !{i32 15}
!13 = !{i32 8}
!14 = !{i32 2, !"Debug Info Version", i32 3}
!15 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !10, line: 1, type: !16, scopeLine: 1, unit: !9, retainedNodes: !17)
!16 = !DISubroutineType(types: !11)
!17 = !{!18, !20, !21, !23, !24}
!18 = !DILocalVariable(name: "1", scope: !15, file: !10, line: 1, type: !19)
!19 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "2", scope: !15, file: !10, line: 2, type: !19)
!21 = !DILocalVariable(name: "3", scope: !15, file: !10, line: 3, type: !22)
!22 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!23 = !DILocalVariable(name: "4", scope: !15, file: !10, line: 4, type: !19)
!24 = !DILocalVariable(name: "5", scope: !15, file: !10, line: 5, type: !22)
!25 = !DILocation(line: 1, column: 1, scope: !15)
!26 = !DILocation(line: 2, column: 1, scope: !15)
!27 = !DILocation(line: 3, column: 1, scope: !15)
!28 = !DILocation(line: 4, column: 1, scope: !15)
!29 = !DILocation(line: 5, column: 1, scope: !15)
!30 = !DILocation(line: 6, column: 1, scope: !15)
!31 = !DILocation(line: 7, column: 1, scope: !15)
!32 = !DILocation(line: 8, column: 1, scope: !15)
!33 = distinct !DISubprogram(name: "test_bar", linkageName: "test_bar", scope: null, file: !10, line: 9, type: !16, scopeLine: 9, unit: !9, retainedNodes: !34)
!34 = !{!35}
!35 = !DILocalVariable(name: "6", scope: !33, file: !10, line: 9, type: !22)
!36 = !DILocalVariable(name: "gp1", scope: !33, file: !10, line: 9, type: !19)
!37 = !DILocation(line: 9, column: 11, scope: !33)
!38 = !DILocalVariable(name: "gp2", scope: !33, file: !10, line: 9, type: !19)
!39 = !DILocation(line: 9, column: 21, scope: !33)
!40 = !DILocation(line: 9, column: 1, scope: !33)
!41 = !DILocation(line: 10, column: 1, scope: !33)
!42 = !DILocation(line: 11, column: 1, scope: !33)
!43 = distinct !DISubprogram(name: "test_foo", linkageName: "test_foo", scope: null, file: !10, line: 12, type: !16, scopeLine: 12, unit: !9, retainedNodes: !44)
!44 = !{!45, !46}
!45 = !DILocalVariable(name: "7", scope: !43, file: !10, line: 12, type: !22)
!46 = !DILocalVariable(name: "8", scope: !43, file: !10, line: 13, type: !22)
!47 = !DILocalVariable(name: "fgp1", scope: !43, file: !10, line: 12, type: !19)
!48 = !DILocation(line: 12, column: 11, scope: !43)
!49 = !DILocalVariable(name: "fgp2", scope: !43, file: !10, line: 12, type: !19)
!50 = !DILocation(line: 12, column: 21, scope: !43)
!51 = !DILocation(line: 12, column: 1, scope: !43)
!52 = !DILocation(line: 13, column: 1, scope: !43)
!53 = !DILocation(line: 14, column: 1, scope: !43)
!54 = !DILocation(line: 15, column: 1, scope: !43)
