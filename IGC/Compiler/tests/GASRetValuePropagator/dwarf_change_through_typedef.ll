;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; The return type is a typedef of a pointer. After the return value is changed from generic to
; SLM, the typedef's base type must be replaced by the pointer with the SLM DWARF address space.

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-gas-ret-value-propagator | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

@foo.val = internal addrspace(3) global i32 undef, align 4

; CHECK: define internal spir_func ptr addrspace(3) @test(ptr addrspace(3) %val)
define internal spir_func ptr addrspace(4) @test(ptr addrspace(3) %val) #0 !dbg !10 {
entry:
  %0 = addrspacecast ptr addrspace(3) %val to ptr addrspace(4), !dbg !16
  ret ptr addrspace(4) %0, !dbg !16
}

define spir_kernel void @foo(ptr addrspace(1) %out) #1 !dbg !17 {
entry:
  %call = call spir_func ptr addrspace(4) @test(ptr addrspace(3) @foo.val), !dbg !19
  store ptr addrspace(4) %call, ptr addrspace(1) %out, align 8, !dbg !19
  ret void, !dbg !19
}

; CHECK-DAG: !DISubroutineType(types: ![[TYPES:[0-9]+]])
; CHECK-DAG: ![[TYPES]] = !{![[TYPEDEF:[0-9]+]], !{{[0-9]+}}}
; CHECK-DAG: ![[TYPEDEF]] = !DIDerivedType(tag: DW_TAG_typedef, name: "slm_int_ptr", {{.*}}baseType: ![[PTR:[0-9]+]])
; CHECK-DAG: ![[PTR]] = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !{{[0-9]+}}, size: 64, dwarfAddressSpace: 3)

attributes #0 = { noinline nounwind "visaStackCall" }
attributes #1 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3}
!igc.functions = !{!4, !7}

!0 = distinct !DICompileUnit(language: DW_LANG_OpenCL, file: !1, producer: "test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "test.cl", directory: "/tmp")
!2 = !{}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{ptr @foo, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}
!7 = !{ptr @test, !8}
!8 = !{!9}
!9 = !{!"function_type", i32 2}
!10 = distinct !DISubprogram(name: "test", scope: null, file: !1, line: 1, type: !11, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !14}
!13 = !DIDerivedType(tag: DW_TAG_typedef, name: "slm_int_ptr", file: !1, line: 1, baseType: !14)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!16 = !DILocation(line: 2, column: 3, scope: !10)
!17 = distinct !DISubprogram(name: "foo", scope: null, file: !1, line: 5, type: !18, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!18 = !DISubroutineType(types: !2)
!19 = !DILocation(line: 6, column: 3, scope: !17)
