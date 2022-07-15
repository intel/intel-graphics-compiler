;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-programscope-constant-resolve -S < %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantResolution
; ------------------------------------------------
; This test checks that ProgramScopeConstantResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; ------------------------------------------------
;
; Pass replaces global variable uses, check that affected IR preserves debug info


@a = internal addrspace(2) constant [2 x i32] [i32 0, i32 1], align 4, !dbg !0
@d = internal addrspace(1) global i32 addrspace(2)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(2)* @a, i32 0, i32 0), align 8, !dbg !12
@c = internal addrspace(1) global i32 0, align 4, !dbg !10
@b = common addrspace(1) global i32 0, align 4, !dbg !6
@llvm.used = appending global [3 x i8*] [i8* addrspacecast (i8 addrspace(2)* bitcast ([2 x i32] addrspace(2)* @a to i8 addrspace(2)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(1)* @c to i8 addrspace(1)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(2)* addrspace(1)* @d to i8 addrspace(1)*) to i8*)], section "llvm.metadata"

; CHECK: @test_program{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: entry:
; CHECK: [[GEP_V:%[A-z0-9]*]] = getelementptr{{.*}} !dbg [[GEP_LOC:![0-9]*]]
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = load i32 addrspace(2){{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 addrspace(2)* [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: store i32{{.*}} !dbg [[STORE1_LOC:![0-9]*]]

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_program(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i8 addrspace(1)* %globalBase, i8* %privateBase, i32 %bufferOffset) #0 !dbg !85 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %aa = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !89, metadata !DIExpression()), !dbg !90
  %0 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(2)* @a, i64 0, i64 1, !dbg !98
  %1 = load i32, i32 addrspace(2)* %0, align 4, !dbg !93
  store i32 %1, i32* %aa, align 4, !dbg !92
  %2 = load i32 addrspace(2)*, i32 addrspace(2)* addrspace(1)* @d, align 8, !dbg !94
  call void @llvm.dbg.value(metadata i32 addrspace(2)* %2, metadata !91, metadata !DIExpression()), !dbg !94
  %3 = load i32, i32 addrspace(2)* %2, align 4, !dbg !95
  store i32 %3, i32 addrspace(1)* @c, align 4, !dbg !96
  ret void, !dbg !97
}

;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ProgramScopeConstantResolution.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_program", scope: null, file: [[FILE]], line: 6
; CHECK-DAG: [[GEP_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "aa", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 9, column: 10, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 9, column: 7, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!18, !19, !20}
!IGCMetadata = !{!21}
!igc.functions = !{!71}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "a", scope: !2, file: !8, line: 1, type: !15, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "<stdin>", directory: "/")
!4 = !{}
!5 = !{!6, !10, !0, !12}
!6 = !DIGlobalVariableExpression(var: !7, expr: !DIExpression())
!7 = distinct !DIGlobalVariable(name: "b", scope: !2, file: !8, line: 2, type: !9, isLocal: true, isDefinition: true)
!8 = !DIFile(filename: "ProgramScopeConstantResolution.ll", directory: "/")
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !DIGlobalVariableExpression(var: !11, expr: !DIExpression())
!11 = distinct !DIGlobalVariable(name: "c", scope: !2, file: !8, line: 3, type: !9, isLocal: true, isDefinition: true)
!12 = !DIGlobalVariableExpression(var: !13, expr: !DIExpression())
!13 = distinct !DIGlobalVariable(name: "d", scope: !2, file: !8, line: 4, type: !14, isLocal: true, isDefinition: true)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!15 = !DICompositeType(tag: DW_TAG_array_type, baseType: !9, size: 64, elements: !16)
!16 = !{!17}
!17 = !DISubrange(count: 2)
!18 = !{i32 2, !"Dwarf Version", i32 4}
!19 = !{i32 2, !"Debug Info Version", i32 3}
!20 = !{i32 1, !"wchar_size", i32 4}
!21 = !{!"ModuleMD", !22, !24, !64}
!22 = !{!"compOpt", !23}
!23 = !{!"OptDisable", i1 true}
!24 = !{!"FuncMD", !25, !26 }
!25 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8 addrspace(1)*, i8*, i32)* @test_program}
!26 = !{!"FuncMDValue[0]", !27, !28, !32, !50, !52, !54, !56, !58, !60}
!27 = !{!"localOffsets"}
!28 = !{!"workGroupWalkOrder", !29, !30, !31}
!29 = !{!"dim0", i32 0}
!30 = !{!"dim1", i32 0}
!31 = !{!"dim2", i32 0}
!32 = !{!"resAllocMD", !33, !34}
!33 = !{!"samplersNumType", i32 0}
!34 = !{!"argAllocMDList", !35, !39, !42, !43, !45, !47, !49}
!35 = !{!"argAllocMDListVec[0]", !36, !37, !38}
!36 = !{!"type", i32 1}
!37 = !{!"extensionType", i32 -1}
!38 = !{!"indexType", i32 0}
!39 = !{!"argAllocMDListVec[1]", !40, !37, !41}
!40 = !{!"type", i32 0}
!41 = !{!"indexType", i32 -1}
!42 = !{!"argAllocMDListVec[2]", !40, !37, !41}
!43 = !{!"argAllocMDListVec[3]", !36, !37, !44}
!44 = !{!"indexType", i32 1}
!45 = !{!"argAllocMDListVec[4]", !36, !37, !46}
!46 = !{!"indexType", i32 2}
!47 = !{!"argAllocMDListVec[5]", !36, !37, !48}
!48 = !{!"indexType", i32 3}
!49 = !{!"argAllocMDListVec[6]", !40, !37, !41}
!50 = !{!"m_OpenCLArgAddressSpaces", !51}
!51 = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
!52 = !{!"m_OpenCLArgAccessQualifiers", !53}
!53 = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
!54 = !{!"m_OpenCLArgTypes", !55}
!55 = !{!"m_OpenCLArgTypesVec[0]", !"int*"}
!56 = !{!"m_OpenCLArgBaseTypes", !57}
!57 = !{!"m_OpenCLArgBaseTypesVec[0]", !"int*"}
!58 = !{!"m_OpenCLArgTypeQualifiers", !59}
!59 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!60 = !{!"m_OpenCLArgNames", !61}
!61 = !{!"m_OpenCLArgNamesVec[0]", !"dst"}
!64 = !{!"inlineProgramScopeOffsets", !65, !66, !67, !68, !69, !70}
!65 = !{!"inlineProgramScopeOffsetsMap[0]", [2 x i32] addrspace(2)* @a}
!66 = !{!"inlineProgramScopeOffsetsValue[0]", i32 0}
!67 = !{!"inlineProgramScopeOffsetsMap[1]", i32 addrspace(1)* @c}
!68 = !{!"inlineProgramScopeOffsetsValue[1]", i32 8}
!69 = !{!"inlineProgramScopeOffsetsMap[2]", i32 addrspace(2)* addrspace(1)* @d}
!70 = !{!"inlineProgramScopeOffsetsValue[2]", i32 0}
!71 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8 addrspace(1)*, i8*, i32)* @test_program, !72}
!72 = !{!73, !74}
!73 = !{!"function_type", i32 0}
!74 = !{!"implicit_arg_desc", !75, !76, !77, !78, !79, !80}
!75 = !{i32 0}
!76 = !{i32 1}
!77 = !{i32 10}
!78 = !{i32 11}
!79 = !{i32 12}
!80 = !{i32 14, !81}
!81 = !{!"explicit_arg_num", i32 0}
!85 = distinct !DISubprogram(name: "test_program", scope: null, file: !8, line: 6, type: !86, flags: DIFlagPrototyped, unit: !2, templateParams: !4, retainedNodes: !4)
!86 = !DISubroutineType(types: !87)
!87 = !{!88, !14}
!88 = !DIBasicType(name: "int", size: 4)
!89 = !DILocalVariable(name: "dst", arg: 1, scope: !85, file: !8, line: 6, type: !14)
!90 = !DILocation(line: 6, column: 33, scope: !85)
!91 = !DILocalVariable(name: "aa", scope: !85, file: !8, line: 9, type: !9)
!92 = !DILocation(line: 8, column: 9, scope: !85)
!93 = !DILocation(line: 8, column: 14, scope: !85)
!94 = !DILocation(line: 9, column: 10, scope: !85)
!95 = !DILocation(line: 9, column: 9, scope: !85)
!96 = !DILocation(line: 9, column: 7, scope: !85)
!97 = !DILocation(line: 10, column: 1, scope: !85)
!98 = !DILocation(line: 7, column: 1, scope: !85)
