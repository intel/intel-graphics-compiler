;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test checks compilation does not crash if DISubroutineType infromation is missed.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-gas-ret-value-propagator | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

@foo.val = internal addrspace(3) global i32 undef, align 4, !dbg !0, !spirv.Decorations !11

; CHECK: define internal spir_func ptr addrspace(3) @test1(ptr addrspace(3) %val)
; CHECK-NOT: %1 = addrspacecast ptr addrspace(3) %0 to ptr addrspace(4)
; CHECK: ret ptr addrspace(3) %0

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func ptr addrspace(4) @test1(ptr addrspace(3) %val) #0 !dbg !24 {
entry:
  %val.addr = alloca ptr addrspace(3), align 8, !spirv.Decorations !28
  store ptr addrspace(3) %val, ptr %val.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %val.addr, metadata !30, metadata !DIExpression()), !dbg !31
  %0 = load ptr addrspace(3), ptr %val.addr, align 8, !dbg !32
  %1 = addrspacecast ptr addrspace(3) %0 to ptr addrspace(4), !dbg !32
  ret ptr addrspace(4) %1, !dbg !33
}

; CHECK: define internal spir_func ptr addrspace(3) @test2(ptr addrspace(3) %val)
; CHECK-NOT: %1 = addrspacecast ptr addrspace(3) %0 to ptr addrspace(4)
; CHECK: ret ptr addrspace(3) %0

define internal spir_func ptr addrspace(4) @test2(ptr addrspace(3) %val) #0 !dbg !38 {
entry:
  %val.addr = alloca ptr addrspace(3), align 8, !spirv.Decorations !28
  store ptr addrspace(3) %val, ptr %val.addr, align 8
  call void @llvm.dbg.declare(metadata ptr %val.addr, metadata !45, metadata !DIExpression()), !dbg !42
  %0 = load ptr addrspace(3), ptr %val.addr, align 8, !dbg !43
  %1 = addrspacecast ptr addrspace(3) %0 to ptr addrspace(4), !dbg !43
  ret ptr addrspace(4) %1, !dbg !44
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; CHECK: %0 = call spir_func ptr addrspace(3) @test1(ptr addrspace(3) @foo.val)
; CHECK: %1 = addrspacecast ptr addrspace(3) %0 to ptr addrspace(4)
; CHECK: store ptr addrspace(4) %1, ptr %ptr1, align 8
; CHECK: %2 = call spir_func ptr addrspace(3) @test2(ptr addrspace(3) @foo.val)
; CHECK: %3 = addrspacecast ptr addrspace(3) %2 to ptr addrspace(4)
; CHECK: store ptr addrspace(4) %3, ptr %ptr2, align 8

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @foo() #2 !dbg !2 {
entry:
  %ptr1 = alloca ptr addrspace(4), align 8, !spirv.Decorations !28
  call void @llvm.dbg.declare(metadata ptr %ptr1, metadata !34, metadata !DIExpression()), !dbg !35
  %res1 = call spir_func ptr addrspace(4) @test1(ptr addrspace(3) @foo.val) #3, !dbg !36
  store ptr addrspace(4) %res1, ptr %ptr1, align 8, !dbg !35
  %ptr2 = alloca ptr addrspace(4), align 8, !spirv.Decorations !28
  call void @llvm.dbg.declare(metadata ptr %ptr2, metadata !34, metadata !DIExpression()), !dbg !35
  %res2 = call spir_func ptr addrspace(4) @test2(ptr addrspace(3) @foo.val) #3, !dbg !36
  store ptr addrspace(4) %res2, ptr %ptr2, align 8, !dbg !35
  ret void, !dbg !37
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" "visaStackCall" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!6}
!llvm.module.flags = !{!13, !14, !15}
!igc.functions = !{!16, !19, !41}
!opencl.ocl.version = !{!22, !22, !22, !22, !22}
!opencl.spir.version = !{!22, !22, !22, !22, !22}
!llvm.ident = !{!23, !23, !23, !23, !23}


!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "val", scope: !2, file: !3, line: 8, type: !10, isLocal: true, isDefinition: true)
!2 = distinct !DISubprogram(name: "foo", scope: null, file: !3, line: 6, type: !4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !6, templateParams: !8, retainedNodes: !8)
!3 = !DIFile(filename: "xxx", directory: "xxx")
!4 = !DISubroutineType(types: !5)
!5 = !{null}
!6 = distinct !DICompileUnit(language: DW_LANG_OpenCL, file: !7, producer: "xxx", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !8, globals: !9)
!7 = !DIFile(filename: "xxx", directory: "xxx")
!8 = !{}
!9 = !{!0}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !{!12}
!12 = !{i32 44, i32 4}
!13 = !{i32 2, !"Dwarf Version", i32 4}
!14 = !{i32 2, !"Debug Info Version", i32 3}
!15 = !{i32 1, !"wchar_size", i32 4}
!16 = !{ptr @foo, !17}
!17 = !{!18}
!18 = !{!"function_type", i32 0}
!19 = !{ptr addrspace(4) (ptr addrspace(3))* @test1, !20}
!20 = !{!21}
!21 = !{!"function_type", i32 2}
!22 = !{i32 2, i32 0}
!23 = !{!"xxx"}
!24 = distinct !DISubprogram(name: "test1", scope: null, file: !3, line: 1, type: !25, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !6, templateParams: !8, retainedNodes: !8)
!25 = !DISubroutineType(types: !26)
!26 = !{null}
!27 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !10, size: 64, dwarfAddressSpace: 4294967295)
!28 = !{!29}
!29 = !{i32 44, i32 8}
!30 = !DILocalVariable(name: "val.addr", arg: 1, scope: !24, file: !3, line: 1, type: !27)
!31 = !DILocation(line: 1, column: 24, scope: !24)
!32 = !DILocation(line: 3, column: 11, scope: !24)
!33 = !DILocation(line: 3, column: 4, scope: !24)
!34 = !DILocalVariable(name: "ptr1", scope: !2, file: !3, line: 9, type: !27)
!35 = !DILocation(line: 9, column: 10, scope: !2)
!36 = !DILocation(line: 9, column: 18, scope: !2)
!37 = !DILocation(line: 10, column: 1, scope: !2)
!38 = distinct !DISubprogram(name: "test2", scope: null, file: !3, line: 1, type: !39, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !6, templateParams: !8, retainedNodes: !8)
!39 = !DISubroutineType(types: !40)
!40 = !{}
!41 = !{ptr addrspace(4) (ptr addrspace(3))* @test2, !20}
!42 = !DILocation(line: 1, column: 24, scope: !38)
!43 = !DILocation(line: 3, column: 11, scope: !38)
!44 = !DILocation(line: 3, column: 4, scope: !38)
!45 = !DILocalVariable(name: "val.addr", arg: 1, scope: !38, file: !3, line: 1, type: !27)
