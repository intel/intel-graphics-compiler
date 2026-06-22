;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Checks getMaxBVHLevels() special logic for wrapping 0 to max value of 8.

; RUN: igc_opt --opaque-pointers -enable-debugify --igc-resolve-ocl-raytracing-builtins -platformdg2 -S < %s 2>&1 | FileCheck %s

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

declare spir_func ptr addrspace(4) @__builtin_IB_intel_get_rt_stack(ptr addrspace(1))

define spir_func ptr addrspace(4) @test_maxbvhlevels_decode(ptr addrspace(1) %rtglobals) {
; CHECK-LABEL: @test_maxbvhlevels_decode
; CHECK: [[ISZERO:%.*]] = icmp eq i32 %{{.*}}, 0
; CHECK: [[DECODED:%.*]] = select i1 [[ISZERO]], i32 8, i32 %{{.*}}
; CHECK: mul i32 {{.*}}, [[DECODED]]
  %ptr = call spir_func ptr addrspace(4) @__builtin_IB_intel_get_rt_stack(ptr addrspace(1) %rtglobals)
  ret ptr addrspace(4) %ptr
}
