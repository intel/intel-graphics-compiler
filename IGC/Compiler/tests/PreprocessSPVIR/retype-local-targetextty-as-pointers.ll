;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; This test verifies that the PreprocessSPVIR pass retypes instructions using
; TargetExtTy (as a type argument) to use pointer types instead. The test also
; verifies that the pointer types have correct address space mapping.

define internal spir_func void @testSamplerRetype(ptr addrspace(4) %p) {
  %A = alloca target("spirv.Sampler"), align 8
  %B = alloca target("spirv.Sampler"), align 8
  %castA = addrspacecast ptr %A to ptr addrspace(4)
  %castB = addrspacecast ptr %B to ptr addrspace(4)
  %val = load target("spirv.Sampler"), ptr addrspace(4) %castA, align 8
  store target("spirv.Sampler") %val, ptr addrspace(4) %castB, align 8
  ret void
}

; CHECK-LABEL: define internal spir_func void @testSamplerRetype(
; CHECK: %A = alloca ptr addrspace(2), align 8
; CHECK: %B = alloca ptr addrspace(2), align 8
; CHECK: %castA = addrspacecast ptr %A to ptr addrspace(4)
; CHECK: %castB = addrspacecast ptr %B to ptr addrspace(4)
; CHECK: %val = load ptr addrspace(2), ptr addrspace(4) %castA, align 8
; CHECK: store ptr addrspace(2) %val, ptr addrspace(4) %castB, align 8
; CHECK: ret void
