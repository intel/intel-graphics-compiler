;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt --opaque-pointers --igc-bfloat-builtins-resolution -S 2>&1 < %s | FileCheck %s --dump-input-filter all

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

; CHECK-LABEL: define spir_kernel void @test_cast_to_i16_with_pointer
declare spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDF16biPU3AS1i(bfloat, i32, ptr addrspace(1))
define spir_kernel void @test_cast_to_i16_with_pointer(ptr addrspace(1) %inbuf, ptr addrspace(1) %outbuf, ptr addrspace(1) %inseed, ptr addrspace(1) %outseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds bfloat, ptr addrspace(1) %inbuf, i64 %globalId0
  %inputLoaded = load bfloat, ptr addrspace(1) %inputAddr, align 2
  %inSeedAddr = getelementptr inbounds i32, ptr addrspace(1) %inseed, i64 %globalId0
  %seedLoaded = load i32, ptr addrspace(1) %inSeedAddr, align 4
  %outSeedAddr = getelementptr inbounds i32, ptr addrspace(1) %outseed, i64 %globalId0
  ; CHECK: %[[INPUT_CASTED:[A-z0-9]*]] = bitcast bfloat %inputLoaded to i16
  ; CHECK: %[[OUTPUT:[A-z0-9]*]] = call i8 @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELsiPU3AS1i(i16 %[[INPUT_CASTED]], i32 %seedLoaded, ptr addrspace(1) %outSeedAddr)
  %result = call spir_func signext i8 @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDF16biPU3AS1i(bfloat %inputLoaded, i32 %seedLoaded, ptr addrspace(1) %outSeedAddr)
  %outputAddr = getelementptr inbounds i8, ptr addrspace(1) %outbuf, i64 %globalId0
  store i8 %result, ptr addrspace(1) %outputAddr, align 1
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_cast_to_v2i16_with_pointer
declare spir_func <2 x i8> @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDv2_DF16biPU3AS4i(<2 x bfloat>, i32, ptr addrspace(4))
define spir_kernel void @test_cast_to_v2i16_with_pointer(ptr addrspace(1) %inbuf, ptr addrspace(1) %outbuf, ptr addrspace(1) %inseed, ptr addrspace(1) %outseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <2 x bfloat>, ptr addrspace(1) %inbuf, i64 %globalId0
  %inputLoaded = load <2 x bfloat>, ptr addrspace(1) %inputAddr, align 4
  %inSeedAddr = getelementptr inbounds i32, ptr addrspace(1) %inseed, i64 %globalId0
  %seedLoaded = load i32, ptr addrspace(1) %inSeedAddr, align 4
  %outseedAddrspaceCast = addrspacecast ptr addrspace(1) %outseed to ptr addrspace(4)
  %outSeedAddr = getelementptr inbounds i32, ptr addrspace(4) %outseedAddrspaceCast, i64 %globalId0
  ; CHECK: %[[INPUT_CASTED:[A-z0-9]*]] = bitcast <2 x bfloat> %inputLoaded to <2 x i16>
  ; CHECK: %[[OUTPUT:[A-z0-9]*]] = call <2 x i8> @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDv2_siPU3AS4i(<2 x i16> %[[INPUT_CASTED]], i32 %seedLoaded, ptr addrspace(4) %outSeedAddr)
  %result = call spir_func <2 x i8> @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDv2_DF16biPU3AS4i(<2 x bfloat> %inputLoaded, i32 %seedLoaded, ptr addrspace(4) %outSeedAddr)
  %outputAddr = getelementptr inbounds <2 x i8>, ptr addrspace(1) %outbuf, i64 %globalId0
  store <2 x i8> %result, ptr addrspace(1) %outputAddr, align 2
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_cast_to_v4i16_no_pointer
declare spir_func <4 x i8> @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDv4_DF16bi(<4 x bfloat>, i32)
define spir_kernel void @test_cast_to_v4i16_no_pointer(ptr addrspace(1) %inbuf, ptr addrspace(1) %outbuf, ptr addrspace(1) %inseed) {
entry:
  %globalId0 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %inputAddr = getelementptr inbounds <4 x bfloat>, ptr addrspace(1) %inbuf, i64 %globalId0
  %inputLoaded = load <4 x bfloat>, ptr addrspace(1) %inputAddr, align 8
  %inSeedAddr = getelementptr inbounds i32, ptr addrspace(1) %inseed, i64 %globalId0
  %seedLoaded = load i32, ptr addrspace(1) %inSeedAddr, align 4
  ; CHECK: %[[INPUT_CASTED:[A-z0-9]*]] = bitcast <4 x bfloat> %inputLoaded to <4 x i16>
  ; CHECK: %[[OUTPUT:[A-z0-9]*]] = call <4 x i8> @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDv4_si(<4 x i16> %[[INPUT_CASTED]], i32 %seedLoaded)
  %result = call spir_func <4 x i8> @_Z46__builtin_spirv_StochasticRoundBF16ToE5M2INTELDv4_DF16bi(<4 x bfloat> %inputLoaded, i32 %seedLoaded)
  %outputAddr = getelementptr inbounds <4 x i8>, ptr addrspace(1) %outbuf, i64 %globalId0
  store <4 x i8> %result, ptr addrspace(1) %outputAddr, align 4
  ret void
}
