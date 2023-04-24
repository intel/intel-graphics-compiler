;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; COM: verify update of debug metadata (shl/shr => trunk/ext)

; CHECK: i16 @test_debug_shlshr{{.*}}

define i16 @test_debug_shlshr(i16 %a) {
entry:
  %0 = shl i16 %a, 8, !dbg !31
  call void @llvm.dbg.value(metadata i16 %0, metadata !17, metadata !DIExpression()), !dbg !31
  %1 = ashr i16 %0, 8, !dbg !32
  call void @llvm.dbg.value(metadata i16 %1, metadata !18, metadata !DIExpression()), !dbg !32
  ret i16 %1
}

; CHECK: void @llvm.dbg.value(metadata{{( i16)?}} [[EMPTY_MD:![0-9]*|undef]]
; COM: shl/ashr gets lowered to trunc+sext
; COM: trunc gets lowered to bitcast/rdregion
; CHECK: [[CAST_V:%[A-z0-9.]*]] = bitcast i16 %a to <2 x i8>, !dbg [[FINAL_LOC:![0-9]*]]
; CHECK: [[READ_V:%[A-z0-9.]*]] = call i8 @llvm.genx.rdregioni.i8.v2i8.i16(<2 x i8> [[CAST_V]], {{.*}}, !dbg [[FINAL_LOC]]
; CHECK: [[SEXT_V:%[A-z0-9.]*]] = sext i8 [[READ_V]] to i16, !dbg [[FINAL_LOC]]
; CHECK: void @llvm.dbg.value(metadata i16 [[SEXT_V]], metadata [[SEXT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FINAL_LOC]]

; CHECK-NOT: [[EMPTY_MD]] = !DILocalVariable
; CHECK-DAG: [[SEXT_MD]] = !DILocalVariable(name: "7"
; CHECK-DAG: [[FINAL_LOC]] = !DILocation(line: 8, column: 1

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
