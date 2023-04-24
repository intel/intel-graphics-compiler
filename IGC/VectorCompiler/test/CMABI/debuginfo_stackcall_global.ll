;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck --check-prefix=CHECK_S1 %s
; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck --check-prefix=CHECK_K1 %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

@x = internal global i32 0, align 4, !dbg !0

; CHECK_S1-LABEL: @S1
; CHECK_S1-SAME: (i32 %0, i32 %x.in)
; CHECK_S1: [[S1_ALLOCA:%[^ ]+]] = alloca i32
; CHECK_S1: call void @llvm.dbg.declare(metadata i32* [[S1_ALLOCA]], metadata ![[#S1VAR:]], metadata !DIExpression()), !dbg ![[#S1LOC:]]
; CHECK_S1-DAG: ![[#S1_SP:]] = distinct !DISubprogram(name: "S1",
; CHECK_S1-DAG: ![[#S1VAR]] = !DILocalVariable(name: "x", scope: ![[#S1_SP]], file: ![[#]], type: ![[#S1TYPE:]], flags: DIFlagArtificial)
; CHECK_S1-DAG: ![[#S1TYPE]] = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
define internal spir_func <8 x i32> @S1(i32 %0) #1 !dbg !17 {
  %2 = load i32, i32* @x, align 4
  %3 = add i32 %2, 1
  store i32 %3, i32* @x, align 4
  ret <8 x i32> zeroinitializer
}

; CHECK_K1-LABEL: @K1
; CHECK_K1-SAME: (i32 %0, i64 %privBase)
; CHECK_K1: [[K1_ALLOCA:%[^ ]+]] = alloca i32
; CHECK_K1: call void @llvm.dbg.declare(metadata i32* [[K1_ALLOCA]], metadata ![[#K1VAR:]], metadata !DIExpression()), !dbg ![[#K1LOC:]]
; CHECK_K1-DAG: ![[#K1_SP:]] = distinct !DISubprogram(name: "K1",
; CHECK_K1-DAG: ![[#K1VAR]] = !DILocalVariable(name: "x", scope: ![[#K1_SP]], file: ![[#]], type: ![[#K1TYPE:]], flags: DIFlagArtificial)
; CHECK_K1-DAG: ![[#K1TYPE]] = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
define dllexport spir_kernel void @K1(i32 %0, i64 %privBase) #5 !dbg !34 {
  %2 = call spir_func <8 x i32> @S1(i32 %0) #7, !dbg !38
  %3 = load i32, i32* @x, align 4
  ret void
}

attributes #1 = { noinline nounwind "CMStackCall" }
attributes #5 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #7 = { noinline nounwind }

!llvm.module.flags = !{!7, !8}
!llvm.dbg.cu = !{!2}
!genx.kernels = !{!12}
!genx.kernel.internal = !{!16}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "x", scope: !2, file: !3, line: 3, type: !6, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "1K1S.cpp", directory: "/the_directory")
!4 = !{}
!5 = !{!0}
!6 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!7 = !{i32 2, !"Dwarf Version", i32 4}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = !{i32 0, i32 0}
!10 = !{i32 1, i32 2}
!11 = !{i16 6, i16 14}
!12 = !{void (i32, i64)* @K1, !"K1", !13, i32 0, i32 0, !14, !15, i32 0}
!13 = !{i32 2, i32 96}
!14 = !{i32 0}
!15 = !{!"buffer_t read_write"}
!16 = !{void (i32, i64)* @K1, null, null, !4, null}
!17 = distinct !DISubprogram(name: "S1", scope: !3, file: !3, line: 4, type: !18, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, templateParams: !4, retainedNodes: !24)
!18 = !DISubroutineType(types: !19)
!19 = !{!20, !23}
!20 = !DICompositeType(tag: DW_TAG_array_type, baseType: !6, size: 256, flags: DIFlagVector, elements: !21)
!21 = !{!22}
!22 = !DISubrange(count: 8)
!23 = !DIBasicType(name: "SurfaceIndex", size: 32, encoding: DW_ATE_unsigned)
!24 = !{}
!34 = distinct !DISubprogram(name: "K1", scope: !3, file: !3, line: 12, type: !35, scopeLine: 12, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !37)
!35 = !DISubroutineType(types: !36)
!36 = !{null, !23}
!37 = !{}
!38 = !DILocation(line: 14, column: 8, scope: !34)
!46 = !{i32 1}
!47 = !{i32 7747}
