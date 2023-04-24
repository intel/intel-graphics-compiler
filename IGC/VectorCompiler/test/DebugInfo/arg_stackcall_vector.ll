;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check stackcall argument definition
; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-debug  -generateDebugInfo -dumpcommonisa' -o /dev/null
; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_kernel_dwarf.elf | FileCheck %s

; RUN: FileCheck %s --input-file=kernel_f0.visaasm --check-prefix=CHECK_ARGSIZE

; CHECK_ARGSIZE: .kernel_attr ArgSize=1

; CHECK: DW_TAG_formal_parameter
; CHECK: DW_AT_name           : arg
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line : 5
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location  :

; ModuleID = 'Deserialized LLVM Module'
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind
define internal spir_func <4 x i32> @_Z4funcu2CMvb4_i(<4 x i32> %arg) #0 !dbg !16 !FuncArgSize !39 !FuncRetSize !40 {
  call void @llvm.dbg.value(metadata <4 x i32> %arg, metadata !24, metadata !DIExpression()), !dbg !26
  %add = add <4 x i32> %arg, <i32 1, i32 1, i32 1, i32 1>
  ret <4 x i32> %add
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i32 %status_buf, <4 x i32> %karg, i64 %privBase) #2 !dbg !29 {
  %call = call spir_func <4 x i32> @_Z4funcu2CMvb4_i(<4 x i32> %karg) #4, !dbg !38, !FuncArgSize !39, !FuncRetSize !40
  %sub = sub <4 x i32> %call, %karg
  call void @llvm.genx.oword.st.v4i32(i32 1, i32 0, <4 x i32> %sub)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>) #3

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind "CMStackCall" }
attributes #1 = { nounwind readnone speculatable willreturn }
attributes #2 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #3 = { nounwind }
attributes #4 = { noinline nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!genx.kernels = !{!8}
!genx.kernel.internal = !{!12}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "src.cm", directory: "./")
!4 = !{}
!5 = !{i32 0, i32 0}
!8 = !{void (i32, <4 x i32>, i64)* @kernel, !"kernel", !9, i32 0, !10, !5, !11, i32 0}
!9 = !{i32 2, i32 0, i32 96}
!10 = !{i32 72, i32 80, i32 64}
!11 = !{!"buffer_t read_write", !""}
!12 = !{void (i32, <4 x i32>, i64)* @kernel, !13, !14, !4, !15}
!13 = !{i32 0, i32 0, i32 0}
!14 = !{i32 0, i32 1, i32 2}
!15 = !{i32 1, i32 -1, i32 2}
!16 = distinct !DISubprogram(name: "func", linkageName: "_Z4funcu2CMvb4_i", scope: null, file: !3, line: 5, type: !17, scopeLine: 5, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, templateParams: !4, retainedNodes: !23)
!17 = !DISubroutineType(types: !18)
!18 = !{!19, !19}
!19 = !DICompositeType(tag: DW_TAG_array_type, baseType: !20, size: 128, flags: DIFlagVector, elements: !21)
!20 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!21 = !{!22}
!22 = !DISubrange(count: 4)
!23 = !{!24}
!24 = !DILocalVariable(name: "arg", arg: 1, scope: !16, file: !3, line: 5, type: !19)
!26 = !DILocation(line: 0, scope: !16)
!29 = distinct !DISubprogram(name: "kernel", scope: null, file: !3, line: 12, type: !30, scopeLine: 12, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !34)
!30 = !DISubroutineType(types: !31)
!31 = !{null, !32, !19}
!32 = !DIDerivedType(tag: DW_TAG_typedef, name: "SurfaceIndex_buffer_t", file: !3, baseType: !33)
!33 = !DIBasicType(name: "SurfaceIndex", size: 32, encoding: DW_ATE_unsigned)
!34 = !{!35, !36}
!35 = !DILocalVariable(name: "status_buf", arg: 1, scope: !29, file: !3, line: 12, type: !32)
!36 = !DILocalVariable(name: "karg", arg: 2, scope: !29, file: !3, line: 12, type: !19)
!38 = !DILocation(line: 13, column: 24, scope: !29)
!39 = !{i32 1}
!40 = !{i32 1}
