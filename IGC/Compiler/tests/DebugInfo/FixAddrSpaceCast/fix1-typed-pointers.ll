;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-addrspacecast-fix -S < %s | FileCheck %s
; ------------------------------------------------
; FixAddrSpaceCast
; ------------------------------------------------
; This test checks that FixAddrSpaceCast pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_fixaddr{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[ITOPTR_V:%[0-9A-z.]*]] = bitcast{{.*}}, !dbg [[ITOPTR_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata i64* [[ITOPTR_V]], metadata [[ITOPTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ITOPTR_LOC]]
; CHECK: [[ACAST_V:%[0-9A-z.]*]] = addrspacecast{{.*}}, !dbg [[ACAST_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata i64 addrspace(2)* [[ACAST_V]], metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC]]
; CHECK: store {{.*}}, !dbg [[STORE_LOC:![0-9]*]]

define void @test_fixaddr(i64* %src) !dbg !6 {
  %1 = ptrtoint i64* %src to i64, !dbg !14
  call void @llvm.dbg.value(metadata i64 %1, metadata !9, metadata !DIExpression()), !dbg !14
  %2 = inttoptr i64 %1 to i64*, !dbg !15
  call void @llvm.dbg.value(metadata i64* %2, metadata !11, metadata !DIExpression()), !dbg !15
  %3 = ptrtoint i64* %2 to i64, !dbg !16
  call void @llvm.dbg.value(metadata i64 %3, metadata !12, metadata !DIExpression()), !dbg !16
  %4 = inttoptr i64 %3 to i64 addrspace(2)*, !dbg !17
  call void @llvm.dbg.value(metadata i64 addrspace(2)* %4, metadata !13, metadata !DIExpression()), !dbg !17
  store i64 13, i64 addrspace(2)* %4, !dbg !18
  ret void, !dbg !19
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fix1.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_fixaddr", linkageName: "test_fixaddr", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ITOPTR_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[ITOPTR_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "fix1.ll", directory: "/")
!2 = !{}
!3 = !{i32 6}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_fixaddr", linkageName: "test_fixaddr", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocation(line: 1, column: 1, scope: !6)
!15 = !DILocation(line: 2, column: 1, scope: !6)
!16 = !DILocation(line: 3, column: 1, scope: !6)
!17 = !DILocation(line: 4, column: 1, scope: !6)
!18 = !DILocation(line: 5, column: 1, scope: !6)
!19 = !DILocation(line: 6, column: 1, scope: !6)
