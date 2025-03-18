;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: opaque-ptr-fix, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-agg-arg-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; AggregateArgumentsAnalysis
; ------------------------------------------------
; This test checks that AggregateArgumentsAnalysis pass follows
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
;     struct _st_foo temp = src;
;     int aa  = temp.a;
;     int bb = (int)temp.b.y;
;     int cc = src.c[1];
; }
;
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%struct._st_foo = type { i32, <4 x float>, [2 x i64] }

; This is an analysis pass that only modifies MD
; Checks:
; * Debug info in IR is not modified

; IR check:
;
; CHECK: define spir_kernel void @bar(%struct._st_foo* byval(%struct._st_foo) %src) #0 !dbg [[KERNEL_SCOPE:![0-9]*]]
; CHECK-NEXT: entry:
; CHECK: @llvm.dbg.declare(metadata %struct._st_foo* %src, metadata [[SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SRC_LOC:![0-9]*]]
; CHECK: @llvm.dbg.declare(metadata %struct._st_foo* %temp, metadata [[TEMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TEMP_LOC:![0-9]*]]
; CHECK-NEXT: bitcast {{.*}}, !dbg [[COPY_LOC:![0-9]*]]
; CHECK-NEXT: bitcast {{.*}}, !dbg [[COPY_LOC]]
; CHECK-NEXT: @llvm.memcpy{{.*}}, !dbg [[COPY_LOC]]
; CHECK-NEXT: @llvm.dbg.declare(metadata i32* %aa, metadata [[AA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AA_LOC:![0-9]*]]
; CHECK-NEXT: %a = getelementptr {{.*}}, !dbg [[AG_LOC:![0-9]*]]
; CHECK-NEXT: load {{.*}}, !dbg [[AG_LOC]]
; CHECK-NEXT: store {{.*}}, !dbg [[AA_LOC]]
; CHECK-NEXT: @llvm.dbg.declare(metadata i32* %bb, metadata [[BB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BB_LOC:![0-9]*]]
; CHECK-NEXT: %b = getelementptr {{.*}}, !dbg [[BG1_LOC:![0-9]*]]
; CHECK-NEXT: load {{.*}}, !dbg [[BG2_LOC:![0-9]*]]
; CHECK-NEXT: extractelement {{.*}}, !dbg [[BG2_LOC]]
; CHECK-NEXT: fptosi {{.*}}, !dbg [[CONV_LOC:![0-9]*]]
; CHECK-NEXT: store {{.*}}, !dbg [[BB_LOC]]
; CHECK-NEXT: @llvm.dbg.declare(metadata i32* %cc, metadata [[CC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CC_LOC:![0-9]*]]
; CHECK-NEXT: %c = getelementptr {{.*}}, !dbg [[CG1_LOC:![0-9]*]]
; CHECK-NEXT: %arrayidx = getelementptr {{.*}}, !dbg [[CG2_LOC:![0-9]*]]
; CHECK-NEXT: load {{.*}}, !dbg [[CG2_LOC]]
; CHECK-NEXT: trunc {{.*}}, !dbg [[CG2_LOC]]
; CHECK-NEXT: store i32 %conv.i1, i32* %cc, align 4, !dbg [[CC_LOC]]
; CHECK-NEXT: ret void, !dbg [[RET_LOC:![0-9]*]]

; Debug MD check:
; CHECK-DAG: [[KERNEL_SCOPE]] = distinct !DISubprogram(name: "bar", scope: null, file: [[FILE:![0-9]*]], line: 7, type: [[KERNEL_TYPE:![0-9]*]]
; CHECK-DAG: [[FILE]] = !DIFile(filename: "AggregateArgumentsAnalysis.ll", directory: "/")
; CHECK-DAG: [[KERNEL_TYPE]] =  !DISubroutineType(types: [[ARG_TYPES:![0-9]*]])
; CHECK-DAG: [[INT4_TY:![0-9]*]] = !DIBasicType(name: "int", size: 4)
; CHECK-DAG: [[STRUCT_TY:![0-9]*]] = !DICompositeType(tag: DW_TAG_structure_type, name: "_st_foo", file: [[FILE]], line: 1, size: 384
; CHECK-DAG: [[ARG_TYPES]] = !{[[INT4_TY]], [[STRUCT_TY]]}
; CHECK-DAG: [[INT32_TY:![0-9]*]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 1, scope: [[KERNEL_SCOPE]], file: [[FILE]], line: 7, type: [[STRUCT_TY]])
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 7, column: 34, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[TEMP_MD]] = !DILocalVariable(name: "temp", scope: [[KERNEL_SCOPE]], file: [[FILE]], line: 9, type: [[STRUCT_TY]])
; CHECK-DAG: [[TEMP_LOC]] = !DILocation(line: 9, column: 20, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[COPY_LOC]] = !DILocation(line: 9, column: 27, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[AA_MD]] = !DILocalVariable(name: "aa", scope: [[KERNEL_SCOPE]], file: [[FILE]], line: 10, type: [[INT32_TY]])
; CHECK-DAG: [[AA_LOC]] = !DILocation(line: 10, column: 9, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[AG_LOC]] = !DILocation(line: 10, column: 20, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[BB_MD]] = !DILocalVariable(name: "bb", scope: [[KERNEL_SCOPE]], file: [[FILE]], line: 11, type: [[INT32_TY]])
; CHECK-DAG: [[BB_LOC]] = !DILocation(line: 11, column: 9, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[BG1_LOC]] = !DILocation(line: 11, column: 24, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[BG2_LOC]] = !DILocation(line: 11, column: 19, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[CONV_LOC]] = !DILocation(line: 11, column: 14, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[CC_MD]] = !DILocalVariable(name: "cc", scope: [[KERNEL_SCOPE]], file: [[FILE]], line: 12, type: [[INT32_TY]])
; CHECK-DAG: [[CC_LOC]] = !DILocation(line: 12, column: 9, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[CG1_LOC]] = !DILocation(line: 12, column: 18, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[CG2_LOC]] = !DILocation(line: 12, column: 14, scope: [[KERNEL_SCOPE]])
; CHECK-DAG: [[RET_LOC]] = !DILocation(line: 13, column: 1, scope: [[KERNEL_SCOPE]])

define spir_kernel void @bar(%struct._st_foo* byval(%struct._st_foo) %src) #0 !dbg !14 {
entry:
  %temp = alloca %struct._st_foo, align 16
  %aa = alloca i32, align 4
  %bb = alloca i32, align 4
  %cc = alloca i32, align 4
  call void @llvm.dbg.declare(metadata %struct._st_foo* %src, metadata !35, metadata !DIExpression()), !dbg !36
  call void @llvm.dbg.declare(metadata %struct._st_foo* %temp, metadata !37, metadata !DIExpression()), !dbg !38
  %0 = bitcast %struct._st_foo* %temp to i8*, !dbg !39
  %1 = bitcast %struct._st_foo* %src to i8*, !dbg !39
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %0, i8* align 16 %1, i64 48, i1 false), !dbg !39
  call void @llvm.dbg.declare(metadata i32* %aa, metadata !40, metadata !DIExpression()), !dbg !41
  %a = getelementptr inbounds %struct._st_foo, %struct._st_foo* %temp, i32 0, i32 0, !dbg !42
  %2 = load i32, i32* %a, align 16, !dbg !42
  store i32 %2, i32* %aa, align 4, !dbg !41
  call void @llvm.dbg.declare(metadata i32* %bb, metadata !43, metadata !DIExpression()), !dbg !44
  %b = getelementptr inbounds %struct._st_foo, %struct._st_foo* %temp, i32 0, i32 1, !dbg !45
  %3 = load <4 x float>, <4 x float>* %b, align 16, !dbg !46
  %4 = extractelement <4 x float> %3, i32 1, !dbg !46
  %conv.i = fptosi float %4 to i32, !dbg !47
  store i32 %conv.i, i32* %bb, align 4, !dbg !44
  call void @llvm.dbg.declare(metadata i32* %cc, metadata !48, metadata !DIExpression()), !dbg !49
  %c = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 2, !dbg !50
  %arrayidx = getelementptr inbounds [2 x i64], [2 x i64]* %c, i64 0, i64 1, !dbg !51
  %5 = load i64, i64* %arrayidx, align 8, !dbg !51
  %conv.i1 = trunc i64 %5 to i32, !dbg !51
  store i32 %conv.i1, i32* %cc, align 4, !dbg !49
  ret void, !dbg !52
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1) #2

attributes #0 = { convergent noinline nounwind }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { argmemonly nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{!6}
!llvm.ident = !{!13, !13, !13, !13, !13}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{void (%struct._st_foo*)* @bar, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 13}
!13 = !{!"clang version 10.0.0"}
!14 = distinct !DISubprogram(name: "bar", scope: null, file: !15, line: 7, type: !16, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!15 = !DIFile(filename: "AggregateArgumentsAnalysis.ll", directory: "/")
!16 = !DISubroutineType(types: !17)
!17 = !{!18, !19}
!18 = !DIBasicType(name: "int", size: 4)
!19 = !DICompositeType(tag: DW_TAG_structure_type, name: "_st_foo", file: !15, line: 1, size: 384, elements: !20)
!20 = !{!21, !23, !30}
!21 = !DIDerivedType(tag: DW_TAG_member, name: "a", file: !15, line: 3, baseType: !22, size: 32)
!22 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!23 = !DIDerivedType(tag: DW_TAG_member, name: "b", file: !15, line: 4, baseType: !24, size: 128, offset: 128)
!24 = !DIDerivedType(tag: DW_TAG_typedef, name: "float4", file: !25, baseType: !26)
!25 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!26 = !DICompositeType(tag: DW_TAG_array_type, baseType: !27, size: 128, flags: DIFlagVector, elements: !28)
!27 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!28 = !{!29}
!29 = !DISubrange(count: 4)
!30 = !DIDerivedType(tag: DW_TAG_member, name: "c", file: !15, line: 5, baseType: !31, size: 128, offset: 256)
!31 = !DICompositeType(tag: DW_TAG_array_type, baseType: !32, size: 128, elements: !33)
!32 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!33 = !{!34}
!34 = !DISubrange(count: 2)
!35 = !DILocalVariable(name: "src", arg: 1, scope: !14, file: !15, line: 7, type: !19)
!36 = !DILocation(line: 7, column: 34, scope: !14)
!37 = !DILocalVariable(name: "temp", scope: !14, file: !15, line: 9, type: !19)
!38 = !DILocation(line: 9, column: 20, scope: !14)
!39 = !DILocation(line: 9, column: 27, scope: !14)
!40 = !DILocalVariable(name: "aa", scope: !14, file: !15, line: 10, type: !22)
!41 = !DILocation(line: 10, column: 9, scope: !14)
!42 = !DILocation(line: 10, column: 20, scope: !14)
!43 = !DILocalVariable(name: "bb", scope: !14, file: !15, line: 11, type: !22)
!44 = !DILocation(line: 11, column: 9, scope: !14)
!45 = !DILocation(line: 11, column: 24, scope: !14)
!46 = !DILocation(line: 11, column: 19, scope: !14)
!47 = !DILocation(line: 11, column: 14, scope: !14)
!48 = !DILocalVariable(name: "cc", scope: !14, file: !15, line: 12, type: !22)
!49 = !DILocation(line: 12, column: 9, scope: !14)
!50 = !DILocation(line: 12, column: 18, scope: !14)
!51 = !DILocation(line: 12, column: 14, scope: !14)
!52 = !DILocation(line: 13, column: 1, scope: !14)
