;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-agg-arg -S < %s | FileCheck %s
; ------------------------------------------------
; ResolveAggregateArguments
; ------------------------------------------------
; This test checks that ResolveAggregateArguments pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; typedef struct _st_foo
; {
;     int a;
;     float4 b;
;     long c[2];
; }st_foo;
;
; __kernel void bar(struct _st_foo src)
; {
;     int aa  = src.a;
;     int bb = (int)src.b.y;
;     int cc = src.c[1];
; }
;
; ------------------------------------------------


; Check that declare calls are properly updated:
; CHECK: define {{.*}} @bar
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK: call void @llvm.dbg.declare(metadata %struct
; CHECK-SAME: metadata [[STRUCT_MD:![0-9]*]]
; CHECK-SAME: metadata !DIExpression()
; CHECK-SAME: !dbg [[STRUCT_LOC:![0-9]*]]

; CHECK: call void @llvm.dbg.declare(metadata i32*
; CHECK-SAME: metadata [[AA_MD:![0-9]*]]
; CHECK-SAME: metadata !DIExpression()
; CHECK-SAME: !dbg [[AA_LOC:![0-9]*]]

; CHECK-DAG: load i32, i32* [[A_PTR:%[A-z0-9]*]], {{.*}} !dbg [[A_LOC:![0-9]*]]
; CHECK-DAG: [[A_PTR]] = getelementptr {{.*}} !dbg [[A_LOC]]

; CHECK: call void @llvm.dbg.declare(metadata i32*
; CHECK-SAME: metadata [[BB_MD:![0-9]*]]
; CHECK-SAME: metadata !DIExpression()
; CHECK-SAME: !dbg [[BB_LOC:![0-9]*]]

; CHECK-DAG: load <4 x float>, <4 x float>* [[B_PTR:%[A-z0-9]*]], {{.*}} !dbg [[BL_LOC:![0-9]*]]
; CHECK-DAG: [[B_PTR]] = getelementptr {{.*}} !dbg [[B_LOC:![0-9]*]]

; CHECK: call void @llvm.dbg.declare(metadata i32*
; CHECK-SAME: metadata [[CC_MD:![0-9]*]]
; CHECK-SAME: metadata !DIExpression()
; CHECK-SAME: !dbg [[CC_LOC:![0-9]*]]

; CHECK-DAG: load i64, i64* [[C_PTR:%[A-z0-9]*]], {{.*}} !dbg [[CL_LOC:![0-9]*]]
; CHECK-DAG: [[C_PTR]] = getelementptr {{.*}} !dbg [[C_LOC:[0-9]*]]

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%struct._st_foo = type { i32, <4 x float>, [2 x i64] }

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @bar(%struct._st_foo* byval(%struct._st_foo) %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %const_reg_dword, float %const_reg_fp32, float %const_reg_fp321, float %const_reg_fp322, float %const_reg_fp323, i64 %const_reg_qword, i64 %const_reg_qword4) #0 !dbg !29 {
entry:
  %aa = alloca i32, align 4
  %bb = alloca i32, align 4
  %cc = alloca i32, align 4
  call void @llvm.dbg.declare(metadata %struct._st_foo* %src, metadata !50, metadata !DIExpression()), !dbg !51
  call void @llvm.dbg.declare(metadata i32* %aa, metadata !52, metadata !DIExpression()), !dbg !53
  %a = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 0, !dbg !54
  %0 = load i32, i32* %a, align 16, !dbg !54
  store i32 %0, i32* %aa, align 4, !dbg !53
  call void @llvm.dbg.declare(metadata i32* %bb, metadata !55, metadata !DIExpression()), !dbg !56
  %b = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 1, !dbg !57
  %1 = load <4 x float>, <4 x float>* %b, align 16, !dbg !58
  %2 = extractelement <4 x float> %1, i32 1, !dbg !58
  %conv.i = fptosi float %2 to i32, !dbg !59
  store i32 %conv.i, i32* %bb, align 4, !dbg !56
  call void @llvm.dbg.declare(metadata i32* %cc, metadata !60, metadata !DIExpression()), !dbg !61
  %c = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 2, !dbg !62
  %arrayidx = getelementptr inbounds [2 x i64], [2 x i64]* %c, i64 0, i64 1, !dbg !63
  %3 = load i64, i64* %arrayidx, align 8, !dbg !63
  %conv.i1 = trunc i64 %3 to i32, !dbg !63
  store i32 %conv.i1, i32* %cc, align 4, !dbg !61
  ret void, !dbg !64
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ResolveAggregateArguments.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "bar", scope: null, file: [[FILE]], line: 8
; CHECK-DAG: [[STRUCT_MD]] = !DILocalVariable(name: "src", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[STRUCT_LOC]] = !DILocation(line: 8, column: 34, scope: [[SCOPE]])
; CHECK-DAG: [[AA_MD]] = !DILocalVariable(name: "aa", scope: [[SCOPE]], file: !30, line: 10
; CHECK-DAG: [[AA_LOC]] = !DILocation(line: 10, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[A_LOC]] = !DILocation(line: 10, column: 19, scope: [[SCOPE]])
; CHECK-DAG: [[BB_MD]] = !DILocalVariable(name: "bb", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[BB_LOC]] = !DILocation(line: 11, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[B_LOC]] = !DILocation(line: 11, column: 23, scope: [[SCOPE]])
; CHECK-DAG: [[BL_LOC]] = !DILocation(line: 11, column: 19, scope: [[SCOPE]])
; CHECK-DAG: [[CC_MD]] = !DILocalVariable(name: "cc", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[CC_LOC]] = !DILocation(line: 12, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[C_LOC]] = !DILocation(line: 12, column: 18, scope: [[SCOPE]])
; CHECK-DAG: [[CL_LOC]] = !DILocation(line: 12, column: 14, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.CatchAllDebugLine() #2

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{!6}
!llvm.ident = !{!28, !28, !28, !28, !28}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{void (%struct._st_foo*, <8 x i32>, <8 x i32>, i8*, i32, float, float, float, float, i64, i64)* @bar, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12, !13, !16, !18, !20, !22, !24, !26}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 12}
!13 = !{i32 17, !14, !15}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{!"struct_arg_offset", i32 0}
!16 = !{i32 15, !14, !17}
!17 = !{!"struct_arg_offset", i32 16}
!18 = !{i32 15, !14, !19}
!19 = !{!"struct_arg_offset", i32 20}
!20 = !{i32 15, !14, !21}
!21 = !{!"struct_arg_offset", i32 24}
!22 = !{i32 15, !14, !23}
!23 = !{!"struct_arg_offset", i32 28}
!24 = !{i32 16, !14, !25}
!25 = !{!"struct_arg_offset", i32 32}
!26 = !{i32 16, !14, !27}
!27 = !{!"struct_arg_offset", i32 40}
!28 = !{!"clang version 10.0.0"}
!29 = distinct !DISubprogram(name: "bar", scope: null, file: !30, line: 8, type: !31, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!30 = !DIFile(filename: "ResolveAggregateArguments.ll", directory: "/")
!31 = !DISubroutineType(types: !32)
!32 = !{!33, !34}
!33 = !DIBasicType(name: "int", size: 4)
!34 = !DICompositeType(tag: DW_TAG_structure_type, name: "_st_foo", file: !30, line: 1, size: 384, elements: !35)
!35 = !{!36, !38, !45}
!36 = !DIDerivedType(tag: DW_TAG_member, name: "a", file: !30, line: 3, baseType: !37, size: 32)
!37 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!38 = !DIDerivedType(tag: DW_TAG_member, name: "b", file: !30, line: 4, baseType: !39, size: 128, offset: 128)
!39 = !DIDerivedType(tag: DW_TAG_typedef, name: "float4", file: !40, baseType: !41)
!40 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!41 = !DICompositeType(tag: DW_TAG_array_type, baseType: !42, size: 128, flags: DIFlagVector, elements: !43)
!42 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!43 = !{!44}
!44 = !DISubrange(count: 4)
!45 = !DIDerivedType(tag: DW_TAG_member, name: "c", file: !30, line: 5, baseType: !46, size: 128, offset: 256)
!46 = !DICompositeType(tag: DW_TAG_array_type, baseType: !47, size: 128, elements: !48)
!47 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!48 = !{!49}
!49 = !DISubrange(count: 2)
!50 = !DILocalVariable(name: "src", arg: 1, scope: !29, file: !30, line: 8, type: !34)
!51 = !DILocation(line: 8, column: 34, scope: !29)
!52 = !DILocalVariable(name: "aa", scope: !29, file: !30, line: 10, type: !37)
!53 = !DILocation(line: 10, column: 9, scope: !29)
!54 = !DILocation(line: 10, column: 19, scope: !29)
!55 = !DILocalVariable(name: "bb", scope: !29, file: !30, line: 11, type: !37)
!56 = !DILocation(line: 11, column: 9, scope: !29)
!57 = !DILocation(line: 11, column: 23, scope: !29)
!58 = !DILocation(line: 11, column: 19, scope: !29)
!59 = !DILocation(line: 11, column: 14, scope: !29)
!60 = !DILocalVariable(name: "cc", scope: !29, file: !30, line: 12, type: !37)
!61 = !DILocation(line: 12, column: 9, scope: !29)
!62 = !DILocation(line: 12, column: 18, scope: !29)
!63 = !DILocation(line: 12, column: 14, scope: !29)
!64 = !DILocation(line: 13, column: 1, scope: !29)
