;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-bfloat-builtins-resolution -S 2>&1 < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
declare spir_func bfloat @_Z36__builtin_spirv_ConvertE4M3ToBF16EXTc(i8)

; CHECK-LABEL: define spir_kernel void @test_kernel_a
define spir_kernel void @test_kernel_a(ptr addrspace(1) %inbuf, ptr addrspace(1) %outbuf) {
entry:
  %globalId = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %loadIdx = getelementptr inbounds i8, ptr addrspace(1) %inbuf, i64 %globalId
  %loaded = load i8, ptr addrspace(1) %loadIdx, align 2
  %call1 = call spir_func bfloat @_Z36__builtin_spirv_ConvertE4M3ToBF16EXTc(i8 %loaded)
  ; CHECK: %[[OUTPUT1:[A-z0-9]*]] = call i16 @_Z36__builtin_spirv_ConvertE4M3ToBF16EXTc(i8 %loaded)
  ; CHECK: %[[OUTPUT1_CASTED:[A-z0-9]*]] = bitcast i16 %[[OUTPUT1]] to bfloat
  %storeIdx = getelementptr inbounds bfloat, ptr addrspace(1) %outbuf, i64 %globalId
  store bfloat %call1, ptr addrspace(1) %storeIdx, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_kernel_b
define spir_kernel void @test_kernel_b(ptr addrspace(1) %inbuf, ptr addrspace(1) %outbuf) {
entry:
  %globalId = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %loadIdx = getelementptr inbounds i8, ptr addrspace(1) %inbuf, i64 %globalId
  %loaded = load i8, ptr addrspace(1) %loadIdx, align 2
  %call1 = call spir_func bfloat @_Z36__builtin_spirv_ConvertE4M3ToBF16EXTc(i8 %loaded)
  ; CHECK: %[[OUTPUT2:[A-z0-9]*]] = call i16 @_Z36__builtin_spirv_ConvertE4M3ToBF16EXTc(i8 %loaded)
  ; CHECK: %[[OUTPUT2_CASTED:[A-z0-9]*]] = bitcast i16 %[[OUTPUT2]] to bfloat
  %storeIdx = getelementptr inbounds bfloat, ptr addrspace(1) %outbuf, i64 %globalId
  store bfloat %call1, ptr addrspace(1) %storeIdx, align 4
  ret void
}
