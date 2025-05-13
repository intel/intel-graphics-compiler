;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -prepare-inline-sampler-for-bindless --igc-serialize-metadata -S %s | FileCheck %s
target triple = "spir64-unknown-unknown"

%spirv.Sampler = type opaque


define spir_kernel void @test() {
  %1 = call %spirv.Sampler addrspace(2)* @__bindless_sampler_initializer(i32 17)
  ret void
}

declare spir_func %spirv.Sampler addrspace(2)* @__bindless_sampler_initializer(i32 noundef) local_unnamed_addr

!igc.functions = !{!0}
!IGCMetadata = !{!3}

; CHECK: !{{[0-9]*}} = !{!"explicit_arg_num", i32 17}
!0 = !{void ()* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD"}
; CHECK: !{{[0-9]*}} = !{!"inlineSamplersMDVec[0]",
; CHECK: !{{[0-9]*}} = !{!"m_Value", i32 17}
