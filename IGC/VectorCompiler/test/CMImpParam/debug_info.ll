;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam \
; RUN: -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix=CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam \
; RUN: -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix=CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMImpParam \
; RUN: -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix=CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMImpParam \
; RUN: -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown \
; RUN: -S < %s | FileCheck %s --check-prefix=CHECK-OPAQUE-PTRS

; Test case #1
; Ensures that when we replace intrinsics with a read from a global location
; is preserved
; CHECK-TYPED-PTRS: <3 x i16>* @__imparg_llvm.genx.local.id16{{(, align 1)?}}, !dbg
; CHECK-OPAQUE-PTRS: ptr @__imparg_llvm.genx.local.id16{{(, align 1)?}}, !dbg
define dllexport spir_kernel void @test_implicit_line() #0 !dbg !6 {
  %1 = call <3 x i32> @llvm.genx.local.id.v3i32(), !dbg !5
  ret void
}

declare <3 x i32> @llvm.genx.local.id.v3i32() #0

attributes #0 = { "target-cpu"="XeHPG" }

!genx.kernels = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!4}

!0 = !{void ()* @test_implicit_line, !"test_implicit_line", !1, i32 0, i32 0, !1, !1, i32 0}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "the_test.ll", directory: "/dev/null")
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !DILocation(line: 1, column: 1, scope: !6)
!6 = distinct !DISubprogram(name: "test_implicit_line", linkageName: "test_implicit_line", scope: null, file: !3, line: 1, type: !7, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !1)
!7 = !DISubroutineType(types: !1)
