;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --opaque-pointers -igc-pressure-publisher -S %s | FileCheck %s
; RUN: igc_opt --typed-pointers -igc-pressure-publisher -S %s | FileCheck %s

; Test checks that RegisterPressurePublisher writes max_reg_pressure in metadata

; CHECK: "max_reg_pressure", i32 {{[0-9]+}}

define spir_kernel void @testNoUnif(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %0 = extractelement <3 x i32> %localSize, i64 0
  %1 = extractelement <3 x i32> %localSize, i64 1
  %localIdZ2 = zext i16 %localIdZ to i32
  %mul.i.i = mul i32 %1, %localIdZ2
  %localIdY4 = zext i16 %localIdY to i32
  %add.i.i = add i32 %mul.i.i, %localIdY4
  %mul4.i.i = mul i32 %0, %add.i.i
  %localIdX6 = zext i16 %localIdX to i32
  %add6.i.i = add i32 %mul4.i.i, %localIdX6
  %conv.i = zext i32 %add6.i.i to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in, i64 %conv.i
  %2 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i
  store float %2, float addrspace(1)* %arrayidx1, align 4
  ret void
}

!igc.functions = !{!1}

!1 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32, i32)* @testNoUnif, !2}
!2 = !{!3, !4, !5}
!3 = !{!"function_type", i32 0}
!4 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13, !15}
!5 = !{!"thread_group_size", i32 16, i32 32, i32 32}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 6}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 10}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{i32 15, !16}
!16 = !{!"explicit_arg_num", i32 1}
