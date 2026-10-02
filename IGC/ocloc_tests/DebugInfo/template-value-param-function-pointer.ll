;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A function-pointer non-type template argument has no address IGC can
; describe, so its DIE gets no location.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-16-plus, regkeys, oneapi-readelf

; RUN: llvm-as %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info %t_OCL_simd8_test_kernel.elf | FileCheck %s

; CHECK:      DW_TAG_template_value_param
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_name{{.*}}: F
; CHECK-NOT:  DW_AT_location

; CHECK:      Abbrev Number:

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define internal spir_func void @helper(ptr addrspace(1) %out) #0 !dbg !9 {
  store i32 1, ptr addrspace(1) %out, align 4, !dbg !10
  ret void, !dbg !10
}

define spir_kernel void @test_kernel(ptr addrspace(1) %out) !dbg !5 {
  call spir_func void @helper(ptr addrspace(1) %out), !dbg !8
  ret void, !dbg !8
}

attributes #0 = { noinline optnone "visaStackCall" }

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: false, emissionKind: FullDebug)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !DISubroutineType(types: !4)
!4 = !{null}
!5 = distinct !DISubprogram(name: "test_kernel", scope: null, file: !2, line: 10, type: !3, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !1, templateParams: !6)
!6 = !{!7}
!7 = !DITemplateValueParameter(name: "F", type: !11, value: ptr @helper)
!8 = !DILocation(line: 11, column: 5, scope: !5)
!9 = distinct !DISubprogram(name: "helper", scope: null, file: !2, line: 3, type: !3, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !1)
!10 = !DILocation(line: 4, column: 5, scope: !9)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !3, size: 64)
