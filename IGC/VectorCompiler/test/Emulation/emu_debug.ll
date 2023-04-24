;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s


; COM: verify debug locations of 'xor' are propagated to all expanded instructions
; CHECK: void @test_xor{{.*}}
; CHECK: {{.*}}, !dbg [[XOR_LOC:![0-9]*]]
; CHECK-COUNT-2: {{.*}}, !dbg [[XOR_LOC]]
; CHECK: store i64
; CHECK-NEXT: ret void

define void @test_xor(i64 %src, i64* %dst) {
  %val = xor i64 %src, -1, !dbg !6
  store i64 %val, i64* %dst
  ret void
}

; CHECK: [[XOR_LOC]] = !DILocation(line: 1, column: 1

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "custom", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "emu_debug.ll", directory: "/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "test_xor", linkageName: "test_xor", scope: null, file: !1, line: 1, type: !4, scopeLine: 1, unit: !0, retainedNodes: !{})
!4 = !DISubroutineType(types: !{})
!6 = !DILocation(line: 1, column: 1, scope: !3)
