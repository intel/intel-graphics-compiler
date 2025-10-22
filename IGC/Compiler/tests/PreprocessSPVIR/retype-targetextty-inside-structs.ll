;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; Verify that the PreprocessSPVIR pass retypes struct members of TargetExtTy
; type as pointers. This is needed so that the following passes can perform
; pointer optimizations correctly and the IR can be linked with builtins module
; coming from Clang (LLVM 16 Clang does not support TargetExtTy).

%struct.SamplerHolder = type { target("spirv.Sampler"), i32 }
%struct.Wrapper = type { %struct.SamplerHolder, i64 }

define spir_kernel void @Kernel(target("spirv.Sampler") %arg) {
entry:
  %holder = alloca %struct.SamplerHolder, align 8
  %wrapper = alloca %struct.Wrapper, align 8
  %holder.sampler.field.gep = getelementptr inbounds %struct.SamplerHolder, ptr %holder, i32 0, i32 0
  store target("spirv.Sampler") %arg, ptr %holder.sampler.field.gep, align 8
  %wrapper.holder.gep = getelementptr inbounds %struct.Wrapper, ptr %wrapper, i32 0, i32 0
  %wrapper.holder.sampler.gep = getelementptr inbounds %struct.SamplerHolder, ptr %wrapper.holder.gep, i32 0, i32 0
  %loaded.sampler = load target("spirv.Sampler"), ptr %holder.sampler.field.gep, align 8
  store target("spirv.Sampler") %loaded.sampler, ptr %wrapper.holder.sampler.gep, align 8
  call spir_func void @Helper(target("spirv.Sampler") %loaded.sampler)
  ret void
}

define internal spir_func void @Helper(target("spirv.Sampler") %S) {
entry:
  ret void
}

; Check that struct fields are retyped, including nested/wrapped structs.
; CHECK: %struct.SamplerHolder = type { ptr addrspace(2), i32 }
; CHECK: %struct.Wrapper = type { %struct.SamplerHolder, i64 }

; CHECK-LABEL: define spir_kernel void @Kernel(
; CHECK-SAME: ptr addrspace(2) %arg
; CHECK: %holder = alloca %struct.SamplerHolder
; CHECK: %wrapper = alloca %struct.Wrapper

; CHECK-NOT: target("spirv.Sampler")

