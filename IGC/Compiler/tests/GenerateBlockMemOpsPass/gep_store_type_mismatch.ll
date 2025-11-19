;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt %s --opaque-pointers --platformpvc --generate-block-mem-ops -S --regkey EnableOpaquePointersBackend=1 | FileCheck %s
; CHECK-NOT: call void @llvm.genx.GenISA.simdBlockWrite

; Make sure that the gep (arrayidx) whose result type (%struct.work_size_data) doesn't match the store type (i32)
; behaves like the gep (arrayidx2) matching the type and they both don't generate simdBlockWrite instruction.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.work_size_data = type { i32, i32, i32 }

define spir_kernel void @foo(ptr addrspace(1) %data, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %globalSize, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset) {
entry:
  %idxprom = zext i16 %localIdX to i64
  %arrayidx = getelementptr %struct.work_size_data, ptr addrspace(1) %data, i64 %idxprom
  store i32 0, ptr addrspace(1) %arrayidx, align 4
  %arrayidx2 = getelementptr %struct.work_size_data, ptr addrspace(1) %data, i64 %idxprom, i32 0
  store i32 0, ptr addrspace(1) %arrayidx2, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2, !3, !15}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8, !9, !10, !11, !12, !13}
!4 = !{i32 0}
!5 = !{i32 2}
!6 = !{i32 5}
!7 = !{i32 6}
!8 = !{i32 7}
!9 = !{i32 8}
!10 = !{i32 9}
!11 = !{i32 10}
!12 = !{i32 13}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{!"thread_group_size", i32 64, i32 1, i32 1}

