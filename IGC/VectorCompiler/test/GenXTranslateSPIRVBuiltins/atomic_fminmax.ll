;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s

declare spir_func float @_Z21__spirv_AtomicFMinEXTPU3AS4fiif(float addrspace(4)*, i32, i32, float) #0
declare spir_func float @_Z21__spirv_AtomicFMaxEXTPU3AS4fiif(float addrspace(4)*, i32, i32, float) #0

define spir_func void @foo(float addrspace(4)* %Ptr, float %Value) {
  ; CHECK: %res.fmin = call float @llvm.vc.internal.atomic.fmin.f32.p4f32.f32(float addrspace(4)* %Ptr, i32 4, i32 16, float %Value)
  %res.fmin = call spir_func float @_Z21__spirv_AtomicFMinEXTPU3AS4fiif(float addrspace(4)* %Ptr, i32 4, i32 16, float %Value) #0

  ; CHECK: %res.fmax = call float @llvm.vc.internal.atomic.fmax.f32.p4f32.f32(float addrspace(4)* %Ptr, i32 1, i32 8, float %Value)
  %res.fmax = call spir_func float @_Z21__spirv_AtomicFMaxEXTPU3AS4fiif(float addrspace(4)* %Ptr, i32 1, i32 8, float %Value) #0
  ret void
}

attributes #0 = { nounwind }