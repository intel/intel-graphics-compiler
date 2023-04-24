;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -genx-lower-aggr-copies -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLowerAggrCopies
; ------------------------------------------------
; This test checks that GenXLowerAggrCopies pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Const size mem instrinsics

; CHECK: void @test_loweraggr{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: bb1:
; CHECK: {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: bb2:
; CHECK: {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]

define void @test_loweraggr(i8 addrspace(4)* %a, i8 addrspace(4)* %b) !dbg !6 {
entry:
  call void @llvm.memcpy.p4i8.p4i8.i32(i8 addrspace(4)* %a, i8 addrspace(4)* %b, i32 128, i1 false), !dbg !8
  br label %bb1
bb1:
  call void @llvm.memmove.p4i8.p4i8.i32(i8 addrspace(4)* %a, i8 addrspace(4)* %b, i32 128, i1 false), !dbg !9
  br label %bb2
bb2:
  call void @llvm.memset.p4i8.i32(i8 addrspace(4)* %a, i8 13, i32 128, i1 false), !dbg !10
  br label %end
end:
  ret void, !dbg !11
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "const.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_loweraggr", linkageName: "test_loweraggr", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p4i8.p4i8.i32(i8 addrspace(4)* noalias nocapture writeonly, i8 addrspace(4)* noalias nocapture readonly, i32, i1 immarg) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.memmove.p4i8.p4i8.i32(i8 addrspace(4)* nocapture, i8 addrspace(4)* nocapture readonly, i32, i1 immarg) #0

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.memset.p4i8.i32(i8 addrspace(4)* nocapture writeonly, i8, i32, i1 immarg) #0

attributes #0 = { argmemonly nounwind willreturn }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "const.ll", directory: "/")
!2 = !{}
!3 = !{i32 4}
!4 = !{i32 0}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_loweraggr", linkageName: "test_loweraggr", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !2)
!7 = !DISubroutineType(types: !2)
!8 = !DILocation(line: 1, column: 1, scope: !6)
!9 = !DILocation(line: 2, column: 1, scope: !6)
!10 = !DILocation(line: 3, column: 1, scope: !6)
!11 = !DILocation(line: 4, column: 1, scope: !6)
