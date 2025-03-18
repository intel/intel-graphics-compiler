;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-callgraphscc-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; BuiltinCallGraphAnalysis
; ------------------------------------------------
; This test checks that debug info is properly handled by BuiltinCallGraphAnalysis pass.

; This is analysis pass that updates function(subroutine) attributes, check that debuginfo is not affected.
; ------------------------------------------------

%struct._st_foo = type { i32, i32 }

; CHECK: define {{.*}} @foo
; CHECK-SAME: !dbg [[FOO_SCOPE:![0-9]*]]

; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}[[S_V:%[A-z0-9]*]],
; CHECK-SAME: metadata [[S_MD:![0-9]*]], metadata !DIExpression()), !dbg [[S_LOC:![0-9]*]]
; CHECK: getelementptr {{.*}} [[S_V]]
; CHECK-SAME: !dbg [[A_LOC:![0-9]*]]
; CHECK: getelementptr {{.*}} [[S_V]]
; CHECK-SAME: !dbg [[B_LOC:![0-9]*]]

define internal spir_func i32 @foo(%struct._st_foo* byval(%struct._st_foo) %s) #2 !dbg !32 {
entry:
  call void @llvm.dbg.declare(metadata %struct._st_foo* %s, metadata !41, metadata !DIExpression()), !dbg !42
  %a = getelementptr inbounds %struct._st_foo, %struct._st_foo* %s, i32 0, i32 0, !dbg !43
  %0 = load i32, i32* %a, align 4, !dbg !43
  %b = getelementptr inbounds %struct._st_foo, %struct._st_foo* %s, i32 0, i32 1, !dbg !44
  %1 = load i32, i32* %b, align 4, !dbg !44
  %add = add nsw i32 %0, %1, !dbg !45
  ret i32 %add, !dbg !46
}

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; CHECK: define {{.*}} @bar(
; CHECK-SAME: !dbg [[BAR_SCOPE:![0-9]*]]

; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}[[BDST_V:%[A-z0-9.]*]],
; CHECK-SAME: metadata [[BDST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BDST_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}[[BS1_V:%[A-z0-9.]*]],
; CHECK-SAME: metadata [[BS1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BS1_LOC:![0-9]*]]
; CHECK: load {{.*}} [[BS1_V]]
; CHECK-SAME: !dbg [[BS1V_LOC:![0-9]*]]
; CHECK: load {{.*}} [[BDST_V]]
; CHECK-SAME: !dbg [[BDSTV_LOC:![0-9]*]]

define spir_kernel void @bar(i32 addrspace(1)* %bdst, i32 %bs1) #2 !dbg !47 {
entry:
  %bdst.addr = alloca i32 addrspace(1)*, align 8
  %bs1.addr = alloca i32, align 4
  store i32 addrspace(1)* %bdst, i32 addrspace(1)** %bdst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %bdst.addr, metadata !56, metadata !DIExpression()), !dbg !57
  store i32 %bs1, i32* %bs1.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %bs1.addr, metadata !58, metadata !DIExpression()), !dbg !59
  %0 = load i32, i32* %bs1.addr, align 4, !dbg !60
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %bdst.addr, align 8, !dbg !61
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 1, !dbg !61
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !62
  ret void, !dbg !63
}

; CHECK: define {{.*}} @test_arg(
; CHECK-SAME: !dbg [[TEST_SCOPE:![0-9]*]]

; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}[[DST_V:%[A-z0-9.]*]],
; CHECK-SAME: metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}[[SRC_V:%[A-z0-9.]*]],
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SRC_LOC:![0-9]*]]
; CHECK: call {{.*}} @foo
; CHECK-SAME: !dbg [[CALLF_LOC:![0-9]*]]
; CHECK: call {{.*}} @bar.1
; CHECK-SAME: !dbg [[CALLB_LOC:![0-9]*]]

define spir_kernel void @test_arg(i32 addrspace(1)* %dst, %struct._st_foo* byval(%struct._st_foo) %src) #0 !dbg !64 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !68, metadata !DIExpression()), !dbg !69
  call void @llvm.dbg.declare(metadata %struct._st_foo* %src, metadata !70, metadata !DIExpression()), !dbg !71
  %call = call spir_func i32 @foo(%struct._st_foo* byval(%struct._st_foo) %src) #2, !dbg !72
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !73
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0, !dbg !73
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4, !dbg !74
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !75
  %a = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 0, !dbg !76
  %2 = load i32, i32* %a, align 4, !dbg !76
  call spir_kernel void @bar.1(i32 addrspace(1)* %1, i32 %2) #2, !dbg !77
  ret void, !dbg !78
}

; CHECK: define {{.*}} @bar.1(
; CHECK-SAME: !dbg [[BAR1_SCOPE:![0-9]*]]

; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}[[BBDST_V:%[A-z0-9.]*]],
; CHECK-SAME: metadata [[BBDST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BBDST_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}[[BBS1_V:%[A-z0-9.]*]],
; CHECK-SAME: metadata [[BBS1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BBS1_LOC:![0-9]*]]
; CHECK: load {{.*}} [[BBS1_V]]
; CHECK-SAME: !dbg [[BBS1V_LOC:![0-9]*]]
; CHECK: load {{.*}} [[BBDST_V]]
; CHECK-SAME: !dbg [[BBDSTV_LOC:![0-9]*]]

define internal spir_kernel void @bar.1(i32 addrspace(1)* %bdst, i32 %bs1) #0 !dbg !79 {
entry:
  %bdst.addr = alloca i32 addrspace(1)*, align 8
  %bs1.addr = alloca i32, align 4
  store i32 addrspace(1)* %bdst, i32 addrspace(1)** %bdst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %bdst.addr, metadata !81, metadata !DIExpression()), !dbg !82
  store i32 %bs1, i32* %bs1.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %bs1.addr, metadata !83, metadata !DIExpression()), !dbg !84
  %0 = load i32, i32* %bs1.addr, align 4, !dbg !85
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %bdst.addr, align 8, !dbg !86
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 1, !dbg !86
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !87
  ret void, !dbg !88
}

; FOO_F

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "BuiltinCallGraphAnalysis.ll", directory: "/")
; CHECK-DAG: [[FOO_SCOPE]] = distinct !DISubprogram(name: "foo", scope: null, file: [[FILE]], line: 7, type: [[FOO_TYPE:![0-9]*]]
; CHECK-DAG: [[FOO_TYPE]] = !DISubroutineType(types: [[FOO_ARGS:![0-9]*]])
; CHECK-DAG: [[FOO_ARGS]] = !{[[INT_TYPE:![0-9]*]], [[STRUCT_TYPE:![0-9]*]]}
; CHECK-DAG: [[INT_TYPE]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
; CHECK-DAG: [[STRUCT_TYPE]] = !DICompositeType(tag: DW_TAG_structure_type, name: "_st_foo", file: [[FILE]], line: 1, size: 64

; CHECK-DAG: [[S_MD]] = !DILocalVariable(name: "s", arg: 1, scope: [[FOO_SCOPE]], file: [[FILE]], line: 7, type: [[STRUCT_TYPE]])
; CHECK-DAG: [[S_LOC]] = !DILocation(line: 7, column: 24, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[A_LOC]] = !DILocation(line: 9, column: 12, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[B_LOC]] = !DILocation(line: 9, column: 18, scope: [[FOO_SCOPE]])

; BAR_K

; CHECK-DAG: [[BAR_SCOPE]] = distinct !DISubprogram(name: "bar", scope: null, file: [[FILE]], line: 12, type: [[BAR_TYPE:![0-9]*]]
; CHECK-DAG: [[BAR_TYPE]] = !DISubroutineType(types: [[BAR_ARGS:![0-9]*]])
; CHECK-DAG: [[BAR_ARGS]] = !{[[AS_TYPE:![0-9]*]], [[PTR_TYPE:![0-9]*]], [[INT_TYPE]]}
; CHECK-DAG: [[AS_TYPE]] = !DIBasicType(name: "int", size: 4)
; CHECK-DAG: [[PTR_TYPE]] = !DIDerivedType(tag: DW_TAG_pointer_type

; CHECK-DAG: [[BDST_MD]] = !DILocalVariable(name: "bdst", arg: 1, scope: [[BAR_SCOPE]], file: [[FILE]], line: 12, type: [[PTR_TYPE]])
; CHECK-DAG: [[BDST_LOC]] = !DILocation(line: 12, column: 34, scope: [[BAR_SCOPE]])
; CHECK-DAG: [[BS1_MD]] = !DILocalVariable(name: "bs1", arg: 2, scope: [[BAR_SCOPE]], file: [[FILE]], line: 12, type: [[INT_TYPE]])
; CHECK-DAG: [[BS1_LOC]] = !DILocation(line: 12, column: 44, scope: [[BAR_SCOPE]])
; CHECK-DAG: [[BS1V_LOC]] = !DILocation(line: 14, column: 13, scope: [[BAR_SCOPE]])
; CHECK-DAG: [[BDSTV_LOC]] = !DILocation(line: 14, column: 3, scope: [[BAR_SCOPE]])

; TEST_K

; CHECK-DAG: [[TEST_SCOPE]] = distinct !DISubprogram(name: "test_arg", scope: null, file: [[FILE]], line: 17, type: [[TEST_TYPE:![0-9]*]]
; CHECK-DAG: [[TEST_TYPE]] = !DISubroutineType(types: [[TEST_ARGS:![0-9]*]])
; CHECK-DAG: [[TEST_ARGS]] = !{[[AS_TYPE]], [[PTR_TYPE]], [[STRUCT_TYPE]]}

; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[TEST_SCOPE]], file: [[FILE]], line: 17, type: [[PTR_TYPE]])
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 17, column: 41, scope: [[TEST_SCOPE]])
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 2, scope: [[TEST_SCOPE]], file: [[FILE]], line: 17, type: [[STRUCT_TYPE]])
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 17, column: 61, scope: [[TEST_SCOPE]])
; CHECK-DAG: [[CALLF_LOC]] = !DILocation(line: 19, column: 12, scope: [[TEST_SCOPE]])
; CHECK-DAG: [[CALLB_LOC]] = !DILocation(line: 20, column: 3, scope: [[TEST_SCOPE]])

; BAR_F

; CHECK-DAG: [[BAR1_SCOPE]] = distinct !DISubprogram(name: "bar", scope: null, file: [[FILE]], line: 12, type: [[BAR_TYPE]]

; CHECK-DAG: [[BBDST_MD]] = !DILocalVariable(name: "bdst", arg: 1, scope: [[BAR1_SCOPE]], file: [[FILE]], line: 12, type: [[PTR_TYPE]])
; CHECK-DAG: [[BBDST_LOC]] = !DILocation(line: 12, column: 34, scope: [[BAR1_SCOPE]])
; CHECK-DAG: [[BBS1_MD]] = !DILocalVariable(name: "bs1", arg: 2, scope: [[BAR1_SCOPE]], file: [[FILE]], line: 12, type: [[INT_TYPE]])
; CHECK-DAG: [[BBS1_LOC]] = !DILocation(line: 12, column: 44, scope: [[BAR1_SCOPE]])
; CHECK-DAG: [[BBS1V_LOC]] = !DILocation(line: 14, column: 13, scope: [[BAR1_SCOPE]])
; CHECK-DAG: [[BBDSTV_LOC]] = !DILocation(line: 14, column: 3, scope: [[BAR1_SCOPE]])

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind "visaStackCall" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!IGCMetadata = !{!6}
!igc.functions = !{!13, !20, !28, !31}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9, !11, !12}
!8 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32)* @bar}
!9 = !{!"FuncMDValue[0]", !10}
!10 = !{!"funcArgs"}
!11 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, %struct._st_foo*)* @test_arg}
!12 = !{!"FuncMDValue[1]", !10}
!13 = !{void (i32 addrspace(1)*, i32)* @bar, !14}
!14 = !{!15, !16}
!15 = !{!"function_type", i32 0}
!16 = !{!"implicit_arg_desc", !17, !18, !19}
!17 = !{i32 0}
!18 = !{i32 1}
!19 = !{i32 13}
!20 = !{void (i32 addrspace(1)*, %struct._st_foo*)* @test_arg, !21}
!21 = !{!15, !22}
!22 = !{!"implicit_arg_desc", !17, !18, !19, !23, !26}
!23 = !{i32 18, !24, !25}
!24 = !{!"explicit_arg_num", i32 1}
!25 = !{!"struct_arg_offset", i32 0}
!26 = !{i32 18, !24, !27}
!27 = !{!"struct_arg_offset", i32 4}
!28 = !{i32 (%struct._st_foo*)* @foo, !29}
!29 = !{!30}
!30 = !{!"function_type", i32 2}
!31 = !{void (i32 addrspace(1)*, i32)* @bar.1, !29}
!32 = distinct !DISubprogram(name: "foo", scope: null, file: !33, line: 7, type: !34, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!33 = !DIFile(filename: "BuiltinCallGraphAnalysis.ll", directory: "/")
!34 = !DISubroutineType(types: !35)
!35 = !{!36, !37}
!36 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!37 = !DICompositeType(tag: DW_TAG_structure_type, name: "_st_foo", file: !33, line: 1, size: 64, elements: !38)
!38 = !{!39, !40}
!39 = !DIDerivedType(tag: DW_TAG_member, name: "a", file: !33, line: 3, baseType: !36, size: 32)
!40 = !DIDerivedType(tag: DW_TAG_member, name: "b", file: !33, line: 4, baseType: !36, size: 32, offset: 32)
!41 = !DILocalVariable(name: "s", arg: 1, scope: !32, file: !33, line: 7, type: !37)
!42 = !DILocation(line: 7, column: 24, scope: !32)
!43 = !DILocation(line: 9, column: 12, scope: !32)
!44 = !DILocation(line: 9, column: 18, scope: !32)
!45 = !DILocation(line: 9, column: 14, scope: !32)
!46 = !DILocation(line: 9, column: 3, scope: !32)
!47 = distinct !DISubprogram(name: "bar", scope: null, file: !33, line: 12, type: !48, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!48 = !DISubroutineType(types: !49)
!49 = !{!50, !51, !36}
!50 = !DIBasicType(name: "int", size: 4)
!51 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !52, size: 64)
!52 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !53, baseType: !54)
!53 = !DIFile(filename: "header.h", directory: "/")
!54 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!55 = !DILocation(line: 12, scope: !47)
!56 = !DILocalVariable(name: "bdst", arg: 1, scope: !47, file: !33, line: 12, type: !51)
!57 = !DILocation(line: 12, column: 34, scope: !47)
!58 = !DILocalVariable(name: "bs1", arg: 2, scope: !47, file: !33, line: 12, type: !36)
!59 = !DILocation(line: 12, column: 44, scope: !47)
!60 = !DILocation(line: 14, column: 13, scope: !47)
!61 = !DILocation(line: 14, column: 3, scope: !47)
!62 = !DILocation(line: 14, column: 11, scope: !47)
!63 = !DILocation(line: 15, column: 1, scope: !47)
!64 = distinct !DISubprogram(name: "test_arg", scope: null, file: !33, line: 17, type: !65, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!65 = !DISubroutineType(types: !66)
!66 = !{!50, !51, !37}
!67 = !DILocation(line: 17, scope: !64)
!68 = !DILocalVariable(name: "dst", arg: 1, scope: !64, file: !33, line: 17, type: !51)
!69 = !DILocation(line: 17, column: 41, scope: !64)
!70 = !DILocalVariable(name: "src", arg: 2, scope: !64, file: !33, line: 17, type: !37)
!71 = !DILocation(line: 17, column: 61, scope: !64)
!72 = !DILocation(line: 19, column: 12, scope: !64)
!73 = !DILocation(line: 19, column: 3, scope: !64)
!74 = !DILocation(line: 19, column: 10, scope: !64)
!75 = !DILocation(line: 20, column: 7, scope: !64)
!76 = !DILocation(line: 20, column: 16, scope: !64)
!77 = !DILocation(line: 20, column: 3, scope: !64)
!78 = !DILocation(line: 21, column: 1, scope: !64)
!79 = distinct !DISubprogram(name: "bar", scope: null, file: !33, line: 12, type: !48, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!80 = !DILocation(line: 12, scope: !79)
!81 = !DILocalVariable(name: "bdst", arg: 1, scope: !79, file: !33, line: 12, type: !51)
!82 = !DILocation(line: 12, column: 34, scope: !79)
!83 = !DILocalVariable(name: "bs1", arg: 2, scope: !79, file: !33, line: 12, type: !36)
!84 = !DILocation(line: 12, column: 44, scope: !79)
!85 = !DILocation(line: 14, column: 13, scope: !79)
!86 = !DILocation(line: 14, column: 3, scope: !79)
!87 = !DILocation(line: 14, column: 11, scope: !79)
!88 = !DILocation(line: 15, column: 1, scope: !79)
