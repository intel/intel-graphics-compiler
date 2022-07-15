;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -PromoteConstant -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteConstant
; ------------------------------------------------
; This test checks that PromoteConstant pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_promoteconst{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: bb1:
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]

@a = private addrspace(2) constant [4 x i32] [i32 -13, i32 42, i32 13, i32 -42], align 1
@b = private addrspace(2) constant [16 x i32] [i32 -13, i32 42, i32 13, i32 -42, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12], align 1

; Function Attrs: convergent noinline nounwind
define spir_kernel void @test_promoteconst(i32 %a, i32* %b) #0 !dbg !6 {
entry:
  %0 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(2)* @a, i32 0, i32 %a, !dbg !17
  call void @llvm.dbg.value(metadata i32 addrspace(2)* %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = getelementptr inbounds [16 x i32], [16 x i32] addrspace(2)* @b, i32 0, i32 %a, !dbg !18
  call void @llvm.dbg.value(metadata i32 addrspace(2)* %1, metadata !11, metadata !DIExpression()), !dbg !18
  br label %bb1, !dbg !19

bb1:                                              ; preds = %bb1, %entry
  %2 = load i32, i32 addrspace(2)* %0, !dbg !20
  call void @llvm.dbg.value(metadata i32 %2, metadata !12, metadata !DIExpression()), !dbg !20
  %3 = load i32, i32 addrspace(2)* %1, !dbg !21
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !21
  %4 = icmp sgt i32 %2, %3, !dbg !22
  call void @llvm.dbg.value(metadata i1 %4, metadata !15, metadata !DIExpression()), !dbg !22
  br i1 %4, label %bb1, label %end, !dbg !23

end:                                              ; preds = %bb1
  store i32 %3, i32* %b, !dbg !24
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PromoteConstant.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promoteconst", linkageName: "test_promoteconst", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "PromoteConstant.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_promoteconst", linkageName: "test_promoteconst", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !13)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !16)
!16 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
