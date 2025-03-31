;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test generates DWARF with class enum (adds DW_AT_enum_class field)

; UNSUPPORTED: sys32

; REQUIRES: regkeys, oneapi-readelf, dg2-supported

; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

; CHECK: DW_TAG_enumeration_type
; CHECK:   DW_AT_type        : {{.*}}
; CHECK:   DW_AT_enum_class  : {{.*}}
; CHECK:   DW_AT_name        : {{.*}}
; CHECK:   DW_AT_byte_size   : {{.*}}

@a = addrspace(1) global i64 0, align 8, !dbg !0

; Function Attrs: nounwind
define spir_kernel void @foo() #0 !dbg !17 {
entry:
  %b = alloca i32, align 4
  call void @llvm.dbg.value(metadata i32* %b, metadata !20, metadata !22), !dbg !23
  store i32 0, i32* %b, align 4, !dbg !23
  ret void, !dbg !24
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!llvm.dbg.cu = !{!8}
!llvm.module.flags = !{!15, !16}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = !DIGlobalVariable(name: "a", scope: null, file: !2, line: 1, type: !3, isLocal: false, isDefinition: true)
!2 = !DIFile(filename: "enum.cpp", directory: "/tmp")
!3 = !DICompositeType(tag: DW_TAG_enumeration_type, name: "x", file: !2, line: 1, size: 64, align: 64, baseType: !21, flags: DIFlagEnumClass, elements: !4)
!4 = !{!5, !6, !7}
!5 = !DIEnumerator(name: "A", value: 0)
!6 = !DIEnumerator(name: "B", value: 20)
!7 = !DIEnumerator(name: "C", value: 60)
!8 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !2, producer: "clang version 14.0.5", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !9, globals: !14)
!9 = !{!3, !10}
!10 = !DICompositeType(tag: DW_TAG_enumeration_type, name: "y", file: !2, line: 2, size: 32, align: 32, baseType: !21, flags: DIFlagEnumClass, elements: !11)
!11 = !{!12}
!12 = !DIEnumerator(name: "D", value: 0)
!13 = !{}
!14 = !{!0}
!15 = !{i32 2, !"Dwarf Version", i32 4}
!16 = !{i32 1, !"Debug Info Version", i32 3}
!17 = distinct !DISubprogram(name: "foo", scope: !2, file: !2, line: 3, type: !18, flags: DIFlagPrototyped, unit: !8, retainedNodes: !13)
!18 = !DISubroutineType(types: !19)
!19 = !{null}
!20 = !DILocalVariable(name: "b", scope: !17, file: !2, line: 4, type: !21)
!21 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!22 = !DIExpression()
!23 = !DILocation(line: 4, scope: !17)
!24 = !DILocation(line: 5, scope: !17)
