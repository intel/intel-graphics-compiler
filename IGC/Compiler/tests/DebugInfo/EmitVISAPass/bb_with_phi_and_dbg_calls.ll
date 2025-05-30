;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; The test checks if the BB with phi and dbg calls is treated as empty BB and
; the compiler doesn't emit this BB in visaasm.

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -platformdg2 -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s

define spir_kernel void @test(i32 addrspace(3)* align 4 %a, i32 addrspace(3)* align 4 %b, i32 addrspace(3)* align 4 %result, <8 x i32> %r0, <3 x i32> %globalOffset) #0 !dbg !473 {
entry:
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %b, metadata !480, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %result, metadata !481, metadata !DIExpression()), !dbg !483
  %0 = load i32, i32 addrspace(3)* null, align 4294967296, !dbg !484
  %1 = load i32, i32 addrspace(3)* %b, align 4, !dbg !485
  %add = add nsw i32 %0, %1, !dbg !486, !spirv.Decorations !487
  %cmp = icmp eq i32 %add, 0
  br i1 %cmp, label %bb0, label %exit
bb0:
  %phi.1 = phi i1 [ %cmp, %entry ]
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %b, metadata !480, metadata !DIExpression()), !dbg !483
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %result, metadata !481, metadata !DIExpression()), !dbg !483
  br label %exit
exit:
  store i32 %add, i32 addrspace(3)* %result, align 4, !dbg !489
  ret void, !dbg !490, !stats.blockFrequency.digits !491, !stats.blockFrequency.scale !492
}

; CHECK: .function "[[FUNC:[^ ]+]]"
; CHECK: [[FUNC]]:
; CHECK:    FILE "/test.cl"
; CHECK:    LOC 3
; CHECK:    mov (M1_NM, 1) Trunc(0,0)<1> 0x0:ud
; CHECK:    lsc_load.slm (M1_NM, 1)  V0032:d32t  flat[Trunc]:a32
; CHECK:    mov (M1_NM, 1) bTrunc(0,0)<1> b(0,0)<0;1,0>
; CHECK:    lsc_load.slm (M1_NM, 1)  V0033:d32t  flat[bTrunc]:a32
; CHECK:    add (M1_NM, 1) add_(0,0)<1> V0032(0,0)<0;1,0> V0033(0,0)<0;1,0>
; CHECK:    cmp.eq (M1, 16) P1 add_(0,0)<0;1,0> 0x0:d
; CHECK:    cmp.eq (M5, 16) P1 add_(0,0)<0;1,0> 0x0:d
; CHECK:    (!P1) goto (M1, 32) [[BB_EXIT:[^ ]+]]
; CHECK: [[BB_EXIT]]
; CHECK:    mov (M1_NM, 1) resultTrunc(0,0)<1> result(0,0)<0;1,0>
; CHECK:    lsc_store.slm (M1_NM, 1)  flat[resultTrunc]:a32  add_:d32t
; CHECK:    LOC 4
; CHECK:    ret (M1, 1)

declare void @llvm.dbg.value(metadata, metadata, metadata) #1

!llvm.module.flags = !{!0, !1, !2}
!llvm.dbg.cu = !{!3}
!spirv.MemoryModel = !{!5}
!spirv.Source = !{!6}
!spirv.Generator = !{!7}
!igc.functions = !{!8}
!IGCMetadata = !{!14}
!opencl.ocl.version = !{!471, !471}
!opencl.spir.version = !{!471, !471}
!llvm.ident = !{!472, !472}
!printf.strings = !{}
!0 = !{i32 7, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = distinct !DICompileUnit(language: DW_LANG_OpenCL, file: !4, producer: "clang version 15.0.0", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!4 = !DIFile(filename: "test.cl", directory: "/")
!5 = !{i32 2, i32 2}
!6 = !{i32 3, i32 102000}
!7 = !{i16 6, i16 14}
!8 = !{void (i32 addrspace(3)*, i32 addrspace(3)*, i32 addrspace(3)*, <8 x i32>, <3 x i32>)* @test, !9}
!9 = !{!10, !11}
!10 = !{!"function_type", i32 0}
!11 = !{!"implicit_arg_desc", !12, !13}
!12 = !{i32 0}
!13 = !{i32 2}
!14 = !{!"ModuleMD", !15}
!15 = !{!"isPrecise", i1 false}
!471 = !{i32 2, i32 0}
!472 = !{!"clang version 15.0.0"}
!473 = distinct !DISubprogram(name: "test", scope: null, file: !4, line: 1, type: !474, scopeLine: 2, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !3, templateParams: !482, retainedNodes: !478)
!474 = !DISubroutineType(types: !475)
!475 = !{null, !476, !476, !476}
!476 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !477, size: 64, dwarfAddressSpace: 3)
!477 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!478 = !{!479, !480, !481}
!479 = !DILocalVariable(name: "a", arg: 1, scope: !473, file: !4, line: 1, type: !476)
!480 = !DILocalVariable(name: "b", arg: 2, scope: !473, file: !4, line: 1, type: !476)
!481 = !DILocalVariable(name: "result", arg: 3, scope: !473, file: !4, line: 1, type: !476)
!482 = !{}
!483 = !DILocation(line: 0, scope: !473)
!484 = !DILocation(line: 3, column: 17, scope: !473)
!485 = !DILocation(line: 3, column: 24, scope: !473)
!486 = !DILocation(line: 3, column: 22, scope: !473)
!487 = !{!488}
!488 = !{i32 4469}
!489 = !DILocation(line: 3, column: 15, scope: !473)
!490 = !DILocation(line: 4, column: 1, scope: !473)
!491 = !{!"80"}
!492 = !{!"-3"}
