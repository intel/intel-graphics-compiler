;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, debug

; Before the LLVM 17 compatibility fix in DwarfDebug, this test could assert in
; cast_if_present when retainedNodes contained DILabel in addition to
; DILocalVariable.

; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa -simd-mode 8 -regkey EnableDebugging -regkey DumpVISAASMToConsole < %s 2>&1 | FileCheck %s --allow-empty --implicit-check-not="internal compiler error" --implicit-check-not="assertion failed"

define spir_kernel void @test_retained_label(i32 %a) !dbg !6 {
entry:
  call void @llvm.dbg.value(metadata i32 %a, metadata !9, metadata !DIExpression()), !dbg !10
  %add = add i32 %a, 1, !dbg !10
  call void @llvm.dbg.value(metadata i32 %add, metadata !9, metadata !DIExpression()), !dbg !10
  ret void, !dbg !11
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!20}
!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "llvm17-retained-nodes-dilabel.ll", directory: "/")
!2 = !{}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_retained_label", linkageName: "test_retained_label", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !13}
!9 = !DILocalVariable(name: "a", arg: 1, scope: !6, file: !1, line: 1, type: !12)
!10 = !DILocation(line: 2, column: 1, scope: !6)
!11 = !DILocation(line: 3, column: 1, scope: !6)
!12 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!13 = !DILabel(scope: !6, name: "top", file: !1, line: 2)

!20 = !{ptr @test_retained_label, !21}
!21 = !{!22}
!22 = !{!"function_type", i32 0}
