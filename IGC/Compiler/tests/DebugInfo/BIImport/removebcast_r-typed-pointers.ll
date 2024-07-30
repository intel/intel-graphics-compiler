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
; Testcase with return value
; ------------------------------------------------

; CHECK: void @test_biimport{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[CALL_V:%[A-z0-9]*]] = {{.*}}, !dbg [[CALL_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[CALL_V]], metadata [[CALL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]
; CHECK: ret{{.*}} !dbg [[RET_LOC:![0-9]*]]

define void @test_biimport(i32* %a) !dbg !6 {
  %1 = call i32 bitcast (i32 (i8*)* @foo to i32 (i32*)*)(i32* %a), !dbg !11
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !11
  store i32 %1, i32* %a, !dbg !12
  ret void, !dbg !13
}

; CHECK: @foo
; CHECK: [[PTOI_V:%[A-z0-9]*]] = {{.*}}, !dbg [[PTOI_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[PTOI_V]], metadata [[PTOI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTOI_LOC]]

define i32 @foo(i8* %b) {
  %1 = ptrtoint i8* %b to i32, !dbg !17
  call void @llvm.dbg.value(metadata i32 %1, metadata !16, metadata !DIExpression()), !dbg !17
  ret i32 %1, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "removebcast_r.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_biimport", linkageName: "test_biimport", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[RET_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PTOI_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[PTOI_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "removebcast_r.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_biimport", linkageName: "test_biimport", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 4, type: !10)
!17 = !DILocation(line: 4, column: 1, scope: !6)
!18 = !DILocation(line: 5, column: 1, scope: !6)
