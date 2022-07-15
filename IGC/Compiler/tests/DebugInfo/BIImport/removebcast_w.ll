;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-builtin-import -disable-verify -S < %s | FileCheck %s
; ------------------------------------------------
; BIImport
; ------------------------------------------------
; This test checks that debug info is properly handled by BIImport pass
;
; Check that indirect function call is correcty substitued and debuginfo not lost
; Testcase without return value
; ------------------------------------------------



; CHECK: void @test_biimport{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: call{{.*}}, !dbg [[CALL_LOC:![0-9]*]]
; CHECK: ret{{.*}} !dbg [[RET_LOC:![0-9]*]]

define void @test_biimport(i32* %a) !dbg !6 {
  call void bitcast (void (i8*)* @foo to void (i32*)*)(i32* %a), !dbg !8
  ret void, !dbg !9
}

; CHECK: @foo
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]
define void @foo(i8* %b) {
  store i8 13, i8* %b, !dbg !11
  ret void, !dbg !12
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "removebcast_w.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_biimport", linkageName: "test_biimport", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "removebcast_w.ll", directory: "/")
!2 = !{}
!3 = !{i32 4}
!4 = !{i32 0}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_biimport", linkageName: "test_biimport", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !2)
!7 = !DISubroutineType(types: !2)
!8 = !DILocation(line: 1, column: 1, scope: !6)
!9 = !DILocation(line: 2, column: 1, scope: !6)
!11 = !DILocation(line: 3, column: 1, scope: !6)
!12 = !DILocation(line: 4, column: 1, scope: !6)
