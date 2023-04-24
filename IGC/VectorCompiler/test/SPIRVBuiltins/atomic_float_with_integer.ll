;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func float @_Z18__spirv_AtomicLoadPU3AS3fii(float addrspace(3)*, i32, i32)

define spir_func float @spirv_atomic_float_load(float addrspace(3)* %ptr) {
  %res = call spir_func float @_Z18__spirv_AtomicLoadPU3AS3fii(float addrspace(3)* %ptr, i32 0, i32 1)
  ret float %res
}

; COM: Check that no generic address space used and no integer-floating point conversion present.
; CHECK: define internal spir_func {{(noundef )?}}float @_Z18__spirv_AtomicLoadPU3AS3fii(float addrspace(3)*
; CHECK-NOT: addrspacecast
; CHECK-NOT: sitofp
; CHECK-NOT: fptosi
