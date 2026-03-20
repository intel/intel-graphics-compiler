;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -resolve-inline-sampler-for-bindless --igc-serialize-metadata -S %s | FileCheck %s
target triple = "spir64-unknown-unknown"

%spirv.Sampler = type opaque


define spir_kernel void @test(i64 %inlineSampler) {
; CHECK-NOT: @__bindless_sampler_initializer
; CHECK: [[ARG:%[0-9]*]] = inttoptr i64 %inlineSampler
; CHECK: call void @foo({{.*}} [[ARG]])
  %1 = call %spirv.Sampler addrspace(2)* @__bindless_sampler_initializer(i32 17)
  call void @foo(%spirv.Sampler addrspace(2)* %1)
  ret void
}

declare spir_func %spirv.Sampler addrspace(2)* @__bindless_sampler_initializer(i32 noundef) local_unnamed_addr
declare spir_func void @foo(%spirv.Sampler addrspace(2)*)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (i64)* @test, !1}
!1 = !{!2, !4}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !7}
!4 = !{!"implicit_arg_desc", !5}
!5 = !{i32 32, !6}
!6 = !{!"explicit_arg_num", i32 17}
!7 = !{!"FuncMD"}
