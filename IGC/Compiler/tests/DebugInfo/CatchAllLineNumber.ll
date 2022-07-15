;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-catch-all-linenum -S < %s | FileCheck %s
; ------------------------------------------------
; CatchAllLineNumber
; ------------------------------------------------
; This test checks that CatchAllLineNumber pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; Testcase 1:
; Check that debug info is not lost
; and CatchAllLineNumber call has proper dbg node.
;
; CHECK: define {{.*}} @catchline{{.*}} !dbg [[CL_SCOPE:![0-9]*]]
; CHECK-DAG: [[ADD_V:%[0-9]*]] = add i32 {{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK-DAG: call {{.*}}CatchAllDebugLine(), !dbg [[CALL_LOC:![0-9]*]]
; CHECK: dbg.value(metadata i32 [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK-NEXT: br label %end, !dbg [[BR_LOC:![0-9]*]]
;
define spir_kernel void @catchline(i32 %a) !dbg !6 {
entry:
  %0 = add i32 %a, 1, !dbg !12
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !12
  br label %end, !dbg !13

end:                                              ; preds = %entry
  %1 = call i32 @no_catchline(i32 %0), !dbg !14
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !14
  ret void, !dbg !15
}

; Testcase 2:
; Check that debug info is preserved and lost
; and no CatchAllLineNumber call were added

; CHECK: define {{.*}} @no_catchline{{.*}} !dbg [[NCL_SCOPE:![0-9]*]]
; CHECK-NOT: CatchAllDebugLine
; CHECK: [[ADD1_V:%[0-9]*]] = add i32 {{.*}} !dbg [[ADD1_LOC:![0-9]*]]
; CHECK: dbg.value(metadata i32 [[ADD1_V]], metadata [[ADD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD1_LOC]]

define spir_func i32 @no_catchline(i32 %a) !dbg !16 {
  %1 = add i32 %a, 1, !dbg !19
  call void @llvm.dbg.value(metadata i32 %1, metadata !18, metadata !DIExpression()), !dbg !19
  ret i32 %1, !dbg !20
}

; Testcase 1 MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "CatchAllLineNumber.ll", directory: "/")
; CHECK-DAG: [[CL_SCOPE]] = distinct !DISubprogram(name: "catchline", linkageName: "catchline", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 3, column: 1, scope: [[CL_SCOPE]])
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "1", scope: [[CL_SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[BR_LOC]] = !DILocation(line: 4, column: 1, scope: [[CL_SCOPE]])


; Testcase 2 MD:
;
; CHECK-DAG: [[NCL_SCOPE]] = distinct !DISubprogram(name: "no_catchline", linkageName: "no_catchline", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[ADD1_LOC]] = !DILocation(line: 7, column: 1, scope: [[NCL_SCOPE]])
; CHECK-DAG: [[ADD1_MD]] = !DILocalVariable(name: "3", scope: [[NCL_SCOPE]], file: [[FILE]], line: 6

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "CatchAllLineNumber.ll", directory: "/")
!2 = !{}
!3 = !{i32 6}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "catchline", linkageName: "catchline", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 3, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 4, type: !10)
!12 = !DILocation(line: 3, column: 1, scope: !6)
!13 = !DILocation(line: 4, column: 1, scope: !6)
!14 = !DILocation(line: 5, column: 1, scope: !6)
!15 = !DILocation(line: 6, column: 1, scope: !6)
!16 = distinct !DISubprogram(name: "no_catchline", linkageName: "no_catchline", scope: null, file: !1, line: 5, type: !7, scopeLine: 5, unit: !0, retainedNodes: !17)
!17 = !{!18}
!18 = !DILocalVariable(name: "3", scope: !16, file: !1, line: 6, type: !10)
!19 = !DILocation(line: 7, column: 1, scope: !16)
!20 = !DILocation(line: 8, column: 1, scope: !16)
