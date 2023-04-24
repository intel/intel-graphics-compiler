;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; COM: verify update of debug metadata (ext+ext+op => op+ext)

; CHECK: void @test_debug_reduce{{.*}}

define void @test_debug_reduce(<4 x i16> %a, <4 x i16> %b, <4 x i32>* %c) {
entry:
  %0 = sext <4 x i16> %a to <4 x i32>, !dbg !35
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !21, metadata !DIExpression()), !dbg !35
  %1 = sext <4 x i16> %b to <4 x i32>, !dbg !36
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !22, metadata !DIExpression()), !dbg !36
  %2 = xor <4 x i32> %0, %1, !dbg !38
  call void @llvm.dbg.value(metadata <4 x i32> %2, metadata !24, metadata !DIExpression()), !dbg !38
  store <4 x i32> %2, <4 x i32>* %c
  ret void
}


; CHECK: void @llvm.dbg.value(metadata{{( <4 x i32>)?}} [[EMPTY_MD_1:![0-9]*|undef]]
; CHECK: void @llvm.dbg.value(metadata{{( <4 x i32>)?}} [[EMPTY_MD_2:![0-9]*|undef]]
; CHECK: [[OP_V:%[A-z0-9.]*]] = xor <4 x i16> %a, %b, !dbg [[OP_LOC:![0-9]*]]
; CHECK: [[EXT_V:%[A-z0-9.]*]] = sext <4 x i16> [[OP_V]] {{.*}}, !dbg [[EXT_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[EXT_V]], metadata [[EXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXT_LOC]]
; CHECK: store <4 x i32> [[EXT_V]]

; CHECK-NOT: [[EMPTY_MD_1]] = !DILocalVariable
; CHECK-NOT: [[EMPTY_MD_2]] = !DILocalVariable
; CHECK-DAG: [[EXT_MD]] = !DILocalVariable(name: "12"
; CHECK-DAG: [[EXT_LOC]] = !DILocation(line: 14, column: 1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "debug-reduce.ll", directory: "/")
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_debug_reduce", linkageName: "test_debug_reduce", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !{})
!7 = !DISubroutineType(types: !{})
!16 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!20 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 11, type: !16)
!22 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 12, type: !20)
!24 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 14, type: !16)
!35 = !DILocation(line: 11, column: 1, scope: !6)
!36 = !DILocation(line: 12, column: 1, scope: !6)
!38 = !DILocation(line: 14, column: 1, scope: !6)
