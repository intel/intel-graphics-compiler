;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -igc-agg-arg-analysis -igc-add-implicit-args -igc-agg-arg -regkey PreservePaddingInAggregateArgumentsPass=0 -S < %s | FileCheck %s

; Struct kernel arguments passed by value, as in SPIR-V from non-Clang
; producers, are flattened into the payload like byval struct arguments.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.S = type { i32, [2 x i64] }

define spir_func void @use(%struct.S %s) {
  ret void
}

; CHECK-LABEL: define spir_kernel void @test(%struct.S %src, ptr addrspace(1) %out, i32 %const_reg_dword, i64 %const_reg_qword, i64 %const_reg_qword1)
; CHECK:         %src_alloca = alloca %struct.S, align 8
; CHECK:         store i32 %const_reg_dword
; CHECK:         store i64 %const_reg_qword
; CHECK:         store i64 %const_reg_qword1

; A use of the whole value gets it assembled from scalar loads.
; CHECK:         [[P0:%.*]] = getelementptr inbounds %struct.S, ptr %src_alloca, i32 0, i32 0
; CHECK-NEXT:    [[E0:%.*]] = load i32, ptr [[P0]], align 8
; CHECK-NEXT:    [[V0:%.*]] = insertvalue %struct.S poison, i32 [[E0]], 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr inbounds %struct.S, ptr %src_alloca, i32 0, i32 1, i32 0
; CHECK-NEXT:    [[E1:%.*]] = load i64, ptr [[P1]], align 8
; CHECK-NEXT:    [[V1:%.*]] = insertvalue %struct.S [[V0]], i64 [[E1]], 1, 0
; CHECK-NEXT:    [[P2:%.*]] = getelementptr inbounds %struct.S, ptr %src_alloca, i32 0, i32 1, i32 1
; CHECK-NEXT:    [[E2:%.*]] = load i64, ptr [[P2]], align 8
; CHECK-NEXT:    [[V2:%.*]] = insertvalue %struct.S [[V1]], i64 [[E2]], 1, 1

; Extractvalues, including nested ones, become loads of the addressed member.
; CHECK-NEXT:    [[PA:%.*]] = getelementptr inbounds %struct.S, ptr %src_alloca, i32 0, i32 1, i32 1
; CHECK-NEXT:    %a = load i64, ptr [[PA]], align 8
; CHECK:         [[PB:%.*]] = getelementptr inbounds %struct.S, ptr %src_alloca, i32 0, i32 1, i32 0
; CHECK-NEXT:    %b = load i64, ptr [[PB]], align 8
; CHECK:         call spir_func void @use(%struct.S [[V2]])
; CHECK-NOT:     extractvalue
; CHECK:         ret void
define spir_kernel void @test(%struct.S %src, ptr addrspace(1) %out) {
entry:
  %a = extractvalue %struct.S %src, 1, 1
  store i64 %a, ptr addrspace(1) %out, align 8
  %arr = extractvalue %struct.S %src, 1
  %b = extractvalue [2 x i64] %arr, 0
  store i64 %b, ptr addrspace(1) %out, align 8
  call spir_func void @use(%struct.S %src)
  ret void
}

!igc.functions = !{!0, !3}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @use, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 1}
