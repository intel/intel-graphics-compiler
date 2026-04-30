;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S --platformPtl -simd-mode 16 -igc-emit-visa -regkey DumpVISAASMToConsole=1 | FileCheck %s

; CHECK-LABEL: .kernel "test"
; CHECK:       _main_0:
; CHECK: -autoGRFSelection
; CHECK: -maxGRFNum 128

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(ptr addrspace(1) %output, ptr addrspace(1) %eltwise0_input0, ptr addrspace(1) %eltwise1_input0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3, !20, !21}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8, !9, !10, !11, !12, !14, !16, !18}
!4 = !{i32 0}
!5 = !{i32 2}
!6 = !{i32 6}
!7 = !{i32 7}
!8 = !{i32 8}
!9 = !{i32 9}
!10 = !{i32 10}
!11 = !{i32 13}
!12 = !{i32 15, !13}
!13 = !{!"explicit_arg_num", i32 0}
!14 = !{i32 15, !15}
!15 = !{!"explicit_arg_num", i32 1}
!16 = !{i32 15, !17}
!17 = !{!"explicit_arg_num", i32 2}
!18 = !{i32 15, !19}
!19 = !{!"explicit_arg_num", i32 3}
!20 = !{!"thread_group_size", i32 1024, i32 1, i32 1}
!21 = !{!"max_reg_pressure", i32 204}
