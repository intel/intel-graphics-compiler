;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; COM: verify update of debug metadata (shl/shr => trunk/ext)

; CHECK: void @test_debug_shlshr{{.*}}

define void @test_debug_shlshr(<4 x i32> %a, <4 x i32>* %b) {
entry:
  %0 = shl <4 x i32> %a, <i32 16, i32 16, i32 16, i32 16>, !dbg !31
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !17, metadata !DIExpression()), !dbg !31
  %1 = lshr <4 x i32> %0, <i32 16, i32 16, i32 16, i32 16>, !dbg !32
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !18, metadata !DIExpression()), !dbg !32
  store <4 x i32> %1, <4 x i32>* %b
  ret void
}

; CHECK: void @llvm.dbg.value(metadata{{( <4 x i32>)?}} [[EMPTY_MD:![0-9]*|undef]]
; CHECK: [[TRUNC_V:%[A-z0-9.]*]] = trunc <4 x i32> %a to <4 x i16>{{.*}} !dbg [[TRUNC_LOC:![0-9]*]]
; CHECK: [[EXT_V:%[A-z0-9.]*]] = zext <4 x i16> [[TRUNC_V]] {{.*}}, !dbg [[TRUNC_LOC]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[EXT_V]], metadata [[EXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TRUNC_LOC]]

; CHECK-NOT: [[EMPTY_MD]] = !DILocalVariable
; CHECK-DAG: [[EXT_MD]] = !DILocalVariable(name:
; CHECK-DAG: [[TRUNC_LOC]] = !DILocation(line: 8, column: 1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "debug-shlshr.ll", directory: "/")
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_debug_shlsh", linkageName: "test_debug_shlshr", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !{})
!7 = !DISubroutineType(types: !{})
!16 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !16)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !16)
!31 = !DILocation(line: 7, column: 1, scope: !6)
!32 = !DILocation(line: 8, column: 1, scope: !6)
