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
; Check that sampler builtin call is replaced and debuginfo not lost
; ------------------------------------------------

; CHECK: void @test_biimport{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[CONV_V:%[A-z0-9]*]] = {{.*}}, !dbg [[CONV_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[CONV_V]], metadata [[CONV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CONV_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]
; CHECK: ret{{.*}} !dbg [[RET_LOC:![0-9]*]]

define void @test_biimport(i32* %a, i64* %b) !dbg !6 {
  %1 = call i32 @__builtin_IB_convert_sampler_to_int(i64* %b), !dbg !11
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !11
  store i32 %1, i32* %a, !dbg !12
  ret void, !dbg !13
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "convert_sample.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_biimport", linkageName: "test_biimport", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CONV_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[CONV_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[RET_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

declare i32 @__builtin_IB_convert_sampler_to_int(i64*)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "convert_sample.ll", directory: "/")
!2 = !{}
!3 = !{i32 3}
!4 = !{i32 1}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_biimport", linkageName: "test_biimport", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
