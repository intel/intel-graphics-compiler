;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Check that we reduce alloca sinking threshold for functions with optnone attribute.
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey AllocaRAPressureThreshold=22 --regkey AllocaSinkingOptNoneAllowance=20 --igc-private-mem-resolution -S %s | FileCheck %s

; CHECK: for.cond:
; CHECK: for.body:
; CHECK: call{{.*}}llvm.genx.GenISA.StackAlloca
; CHECK: br label %for.cond


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @call_func_in_loop() #0 {
entry:
  %out.addr = alloca ptr addrspace(1), align 8
  %gid = alloca i32, align 4
  %j = alloca i32, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  br label %for.body

for.body:                                         ; preds = %for.cond
  %0 = load i32, ptr %j, align 4
  br label %for.cond
}

declare spir_func void @add_one()

attributes #0 = { "visaStackCall" optnone noinline}

!igc.functions = !{!0, !4}

!0 = !{ptr @add_one, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 2}
!3 = !{!"implicit_arg_desc"}
!4 = !{ptr @call_func_in_loop, !5}
!5 = !{!6, !7}
!6 = !{!"function_type", i32 0}
!7 = !{!"implicit_arg_desc", !8, !9, !10, !11, !12, !13, !14, !15, !17}
!8 = !{i32 0}
!9 = !{i32 2}
!10 = !{i32 7}
!11 = !{i32 8}
!12 = !{i32 9}
!13 = !{i32 10}
!14 = !{i32 13}
!15 = !{i32 15, !16}
!16 = !{!"explicit_arg_num", i32 0}
!17 = !{i32 59, !16}
