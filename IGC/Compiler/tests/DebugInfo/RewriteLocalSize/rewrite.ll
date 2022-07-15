;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-rewrite-local-size -S < %s | FileCheck %s
; ------------------------------------------------
; RewriteLocalSize
; ------------------------------------------------
; This test checks that RewriteLocalSize pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

source_filename = "RewriteLocalSize.ll"

define spir_kernel void @test_rewrite_local(i32 %src1) !dbg !6 {
; Testcase 1
; Check that get_local_size call is substituted
; CHECK: [[LSIZE_V:%[0-9]*]] = call spir_func i32 @__builtin_IB_get_enqueued_local_size({{.*}} !dbg [[LSIZE_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i32 [[LSIZE_V]],  metadata [[LSIZE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LSIZE_LOC]]
  %1 = call spir_func i32 @__builtin_IB_get_local_size(i32 %src1), !dbg !11
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !11
  ret void, !dbg !12
}
; Testcase 1 MD:
; CHECK-NOT: __builtin_IB_get_local_size
; CHECK-DAG: [[LSIZE_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[LSIZE_MD]] = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_size(i32) local_unnamed_addr #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent nounwind readnone }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "RewriteLocalSize.ll", directory: "/")
!2 = !{}
!3 = !{i32 2}
!4 = !{i32 1}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_rewrite_local", linkageName: "test_rewrite_local", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
