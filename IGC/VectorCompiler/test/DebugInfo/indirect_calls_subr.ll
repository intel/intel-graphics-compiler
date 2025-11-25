;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeHPG \
; RUN: -vc-skip-ocl-runtime-info \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo -dumpcommonisa' -o /dev/null
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPG \
; RUN: -vc-skip-ocl-runtime-info \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo -dumpcommonisa' -o /dev/null


; REQUIRES: oneapi-readelf
; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_foo___vyfvyf_dwarf.elf | FileCheck %s  --check-prefix=CHECK_MAIN_FOO
; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_bar___vyfvyf_dwarf.elf | FileCheck %s  --check-prefix=CHECK_MAIN_BAR
; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_f_f_dwarf.elf | FileCheck %s  --check-prefix=CHECK_MAIN_FF

; RUN: oneapi-readelf --debug-dump dbginfo_NOAT_%basename_t_foo___vyfvyf_dwarf.elf | FileCheck %s  --check-prefix=CHECK_MAIN_NO_AT_LINKAGE_FOO
; RUN: oneapi-readelf --debug-dump dbginfo_NOAT_%basename_t_bar___vyfvyf_dwarf.elf | FileCheck %s  --check-prefix=CHECK_MAIN_NO_AT_LINKAGE_BAR
; RUN: oneapi-readelf --debug-dump dbginfo_NOAT_%basename_t_f_f_dwarf.elf | FileCheck %s  --check-prefix=CHECK_MAIN_NO_AT_LINKAGE_FF

; RUN: FileCheck %s --input-file=result_f0.visaasm --check-prefix=CHECK_ARGSIZE
; RUN: FileCheck %s --input-file=result_f1.visaasm --check-prefix=CHECK_ARGSIZE

; CHECK_ARGSIZE: .kernel_attr ArgSize=5

; CHECK_MAIN_FOO: DW_TAG_subprogram
; CHECK_MAIN_FOO: DW_AT_linkage_name: foo___vyfvyf
; CHECK_MAIN_FOO: DW_AT_name : foo
; CHECK_MAIN_BAR: DW_TAG_subprogram
; CHECK_MAIN_BAR: DW_AT_linkage_name: bar___vyfvyf
; CHECK_MAIN_BAR: DW_AT_name : bar
; CHECK_MAIN_FF: DW_TAG_subprogram
; CHECK_MAIN_FF-NOT: DW_AT_linkage_name
; CHECK_MAIN_FF: DW_AT_name : f_f

; CHECK_MAIN_NO_AT_LINKAGE_FOO: DW_TAG_subprogram
; CHECK_MAIN_NO_AT_LINKAGE_FOO-NOT: DW_AT_linkage_name
; CHECK_MAIN_NO_AT_LINKAGE_FOO: DW_AT_name : foo
; CHECK_MAIN_NO_AT_LINKAGE_BAR: DW_TAG_subprogram
; CHECK_MAIN_NO_AT_LINKAGE_BAR-NOT: DW_AT_linkage_name
; CHECK_MAIN_NO_AT_LINKAGE_BAR: DW_AT_name : bar
; CHECK_MAIN_NO_AT_LINKAGE_FF: DW_TAG_subprogram
; CHECK_MAIN_NO_AT_LINKAGE_FF-NOT: DW_AT_linkage_name
; CHECK_MAIN_NO_AT_LINKAGE_FF: DW_AT_name : f_f


target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

; Function Attrs: nounwind readonly
declare <16 x float> @llvm.genx.svm.block.ld.v16f32.i64(i64) #5

; Function Attrs: nounwind
declare void @llvm.genx.svm.block.st.i64.v16f32(i64, <16 x float>) #6

; Function Attrs: noinline norecurse nounwind readnone
define internal spir_func <16 x float> @subroutine(<16 x float> %0, <16 x float> %1) local_unnamed_addr #6 {
  %3 = fadd <16 x float> %0, %1
  ret <16 x float> %3
}

; Function Attrs: noinline nounwind readnone
define internal spir_func <16 x float> @foo___vyfvyf(<16 x float> %a, <16 x float> %b, <16 x i1> %__mask) addrspace(9) #0 !dbg !78 !FuncArgSize !156 !FuncRetSize !157 {
allocas:
  call void @llvm.dbg.value(metadata <16 x float> %a, metadata !89, metadata !DIExpression()), !dbg !93
  call void @llvm.dbg.value(metadata <16 x float> %b, metadata !90, metadata !DIExpression()), !dbg !93
  %l = fadd <16 x float> %a, %b, !dbg !94
  %k = tail call spir_func <16 x float> @subroutine(<16 x float> %l, <16 x float> %l)
  ret <16 x float> %k, !dbg !94
}

; Function Attrs: noinline nounwind
define internal spir_func <16 x float> @bar___vyfvyf(<16 x float> %a, <16 x float> %b, <16 x i1> %__mask) addrspace(9) #2 !dbg !96 !FuncArgSize !156 !FuncRetSize !157 {
allocas:
  call void @llvm.dbg.value(metadata <16 x float> %a, metadata !100, metadata !DIExpression()), !dbg !104
  call void @llvm.dbg.value(metadata <16 x float> %b, metadata !101, metadata !DIExpression()), !dbg !104
  %l = fsub <16 x float> %a, %b, !dbg !111
  %k = tail call spir_func <16 x float> @subroutine(<16 x float> %l, <16 x float> %l)
  ret <16 x float> %k, !dbg !111
}

; Function Attrs: nounwind readnone
declare i1 @llvm.genx.any.v16i1(<16 x i1>) #3

; Function Attrs: nounwind
define dllexport void @f_f(float* noalias %RET, float* noalias %aFOO, i64 %privBase) #4 !dbg !113 {
  %svm_ld_ptrtoint = ptrtoint float* %aFOO to i64
  %src1 = call <16 x float> @llvm.genx.svm.block.ld.v16f32.i64(i64 %svm_ld_ptrtoint), !dbg !133

  %foo_raw = ptrtoint <16 x float> (<16 x float>, <16 x float>, <16 x i1>) addrspace(9)* @bar___vyfvyf to i64, !dbg !137
  %bar_raw = ptrtoint <16 x float> (<16 x float>, <16 x float>, <16 x i1>) addrspace(9)* @foo___vyfvyf to i64, !dbg !137

  %foo_raw_vect = insertelement <1 x i64> undef, i64 %foo_raw, i32 0, !dbg !137
  %bar_raw_vect = insertelement <1 x i64> undef, i64 %bar_raw, i32 0, !dbg !137

  %cmp = icmp eq i64 %privBase, 0
  %rawaddr_v = select i1 %cmp, <1 x i64> %foo_raw_vect, <1 x i64> %bar_raw_vect
  %rawaddr = extractelement <1 x i64> %rawaddr_v, i32 0
  %fptr = inttoptr i64 %rawaddr to <16 x float> (<16 x float>, <16 x float>, <16 x i1>) addrspace(9)*, !dbg !138
  %calltmp = call spir_func addrspace(9) <16 x float> %fptr(<16 x float> %src1, <16 x float> %src1, <16 x i1> zeroinitializer) #6, !dbg !138, !FuncArgSize !156, !FuncRetSize !157
  %k = tail call spir_func <16 x float> @subroutine(<16 x float> %calltmp, <16 x float> %calltmp)

  %svm_st_ptrtoint = ptrtoint float* %RET to i64
  call void @llvm.genx.svm.block.st.i64.v16f32(i64 %svm_st_ptrtoint, <16 x float> %k), !dbg !139

  ret void, !dbg !140
}

; Function Attrs: nounwind
define dllexport void @result(float* noalias %RET, i64 %privBase) #4 !dbg !142 {
allocas:
  ret void, !dbg !155
}

attributes #0 = { noinline nounwind readnone "CMStackCall" }
attributes #1 = { nounwind readnone speculatable willreturn }
attributes #2 = { noinline nounwind "CMStackCall" }
attributes #3 = { nounwind readnone }
attributes #4 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #5 = { nounwind readonly }
attributes #6 = { nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!spirv.Source = !{!61}
!opencl.spir.version = !{!62}
!opencl.ocl.version = !{!61}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!63}
!genx.kernels = !{!64, !68}
!VC.Debug.Enable = !{}
!genx.kernel.internal = !{!73, !76}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "funcptr-uniform-9.ispc", directory: "/the_directory")
!4 = !{}
!5 = !{}
!8 = !DIBasicType(name: "int32", size: 32, encoding: DW_ATE_signed)
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !8, size: 512, flags: DIFlagVector, elements: !12)
!12 = !{!13}
!13 = !DISubrange(count: 16)
!42 = !DINamespace(name: "ispc", scope: !43)
!43 = !DIFile(filename: "stdlib.ispc", directory: "/the_directory")
!49 = !{!50}
!50 = !DISubrange(count: 255)
!61 = !{i32 0, i32 0}
!62 = !{i32 1, i32 2}
!63 = !{i16 6, i16 14}
!64 = !{void (float*, float*, i64)* @f_f, !"f_f", !65, i32 0, !66, !61, !67, i32 0}
!65 = !{i32 0, i32 0, i32 96}
!66 = !{i32 72, i32 80, i32 64}
!67 = !{!"", !""}
!68 = !{void (float*, i64)* @result, !"result", !69, i32 0, !70, !71, !72, i32 0}
!69 = !{i32 0, i32 96}
!70 = !{i32 72, i32 64}
!71 = !{i32 0}
!72 = !{!""}
!73 = !{void (float*, float*, i64)* @f_f, !74, !75, !4, null}
!74 = !{i32 0, i32 0, i32 0}
!75 = !{i32 0, i32 1, i32 2}
!76 = !{void (float*, i64)* @result, !61, !77, !4, null}
!77 = !{i32 0, i32 1}

!78 = distinct !DISubprogram(name: "foo", linkageName: "foo___vyfvyf", scope: !79, file: !3, line: 4, type: !80, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, templateParams: !4, retainedNodes: !84)
!79 = !DINamespace(name: "ispc", scope: !3)
!80 = !DISubroutineType(types: !81)
!81 = !{!82, !82, !82}
!82 = !DICompositeType(tag: DW_TAG_array_type, baseType: !83, size: 512, flags: DIFlagVector, elements: !12)
!83 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!84 = !{!89, !90}
!89 = !DILocalVariable(name: "a", arg: 1, scope: !78, file: !3, line: 3, type: !82)
!90 = !DILocalVariable(name: "b", arg: 2, scope: !78, file: !3, line: 3, type: !82)
!92 = distinct !DILexicalBlock(scope: !78, file: !3, line: 4, column: 5)
!93 = !DILocation(line: 0, scope: !78)
!94 = !DILocation(line: 4, column: 12, scope: !95)
!95 = distinct !DILexicalBlock(scope: !92, file: !3, line: 4, column: 5)

!96 = distinct !DISubprogram(name: "bar", linkageName: "bar___vyfvyf", scope: !79, file: !3, line: 8, type: !80, scopeLine: 8, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !2, templateParams: !4, retainedNodes: !97)
!97 = !{!100, !101}
!100 = !DILocalVariable(name: "a", arg: 1, scope: !96, file: !3, line: 7, type: !82)
!101 = !DILocalVariable(name: "b", arg: 2, scope: !96, file: !3, line: 7, type: !82)
!104 = !DILocation(line: 0, scope: !96)
!106 = distinct !DILexicalBlock(scope: !96, file: !3, line: 8, column: 5)
!111 = !DILocation(line: 11, column: 16, scope: !96)

!113 = distinct !DISubprogram(name: "f_f", scope: !79, file: !3, line: 16, type: !114, scopeLine: 16, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !117)
!114 = !DISubroutineType(types: !115)
!115 = !{null, !116, !116}
!116 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !83, size: 64)
!117 = !{!122, !124}
!119 = distinct !DILexicalBlock(scope: !113, file: !3, line: 16, column: 5)
!122 = !DILocalVariable(name: "a", scope: !123, file: !3, line: 17, type: !82)
!123 = distinct !DILexicalBlock(scope: !119, file: !3, line: 16, column: 5)
!124 = !DILocalVariable(name: "b", scope: !123, file: !3, line: 18, type: !82)
!126 = !DICompositeType(tag: DW_TAG_array_type, baseType: !127, size: 128, elements: !128)
!127 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !80, size: 64)
!128 = !{!129}

!129 = !DISubrange(count: 2)
!130 = !DILocation(line: 0, scope: !131)
!131 = distinct !DILexicalBlock(scope: !113, file: !3, line: 16, column: 5)
!132 = !DILocation(line: 0, scope: !113)
!133 = !DILocation(line: 17, column: 15, scope: !123)
!134 = !DILocation(line: 0, scope: !123)
!137 = !DILocation(line: 19, column: 43, scope: !123)
!138 = !DILocation(line: 19, column: 25, scope: !123)
!139 = !DILocation(line: 19, column: 5, scope: !123)
!140 = !DILocation(line: 19, column: 5, scope: !131)

!142 = distinct !DISubprogram(name: "result", scope: !79, file: !3, line: 23, type: !143, scopeLine: 23, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !145)
!143 = !DISubroutineType(types: !144)
!144 = !{null, !116}
!145 = !{!148}
!147 = distinct !DILexicalBlock(scope: !142, file: !3, line: 23, column: 5)
!148 = !DILocalVariable(name: "RET", arg: 1, scope: !142, file: !3, line: 22, type: !116)
!153 = distinct !DILexicalBlock(scope: !142, file: !3, line: 23, column: 5)
!155 = !DILocation(line: 24, column: 5, scope: !153)
!156 = !{i32 5}
!157 = !{i32 2}
