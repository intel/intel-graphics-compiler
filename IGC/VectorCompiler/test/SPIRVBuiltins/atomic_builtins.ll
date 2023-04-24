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

declare spir_func i32 @_Z33__spirv_AtomicCompareExchangeWeakPU3AS4iiiiii(i32 addrspace(4)*, i32, i32, i32, i32, i32)

define spir_func i32 @spirv_atomic_compare_exchange_weak(i32 addrspace(4)* %ptr, i32 %exp) {
  %res = call spir_func i32 @_Z33__spirv_AtomicCompareExchangeWeakPU3AS4iiiiii(i32 addrspace(4)* %ptr, i32 1, i32 8, i32 0, i32 1, i32 %exp)
  ret i32 %res
}

declare spir_func i32 @_Z29__spirv_AtomicCompareExchangePU3AS4iiiiii(i32 addrspace(4)*, i32, i32, i32, i32, i32)

define spir_func i32 @spirv_atomic_compare_exchange(i32 addrspace(4)* %ptr, i32 %val, i32 %exp) {
  %res = call spir_func i32 @_Z29__spirv_AtomicCompareExchangePU3AS4iiiiii(i32 addrspace(4)* %ptr, i32 1, i32 16, i32 16, i32 %val, i32 %exp)
  ret i32 %res
}
declare spir_func i32 @_Z18__spirv_AtomicLoadPU3AS4iii(i32 addrspace(4)*, i32, i32)

define spir_func i32 @spirv_atomic_load(i32 addrspace(4)* %ptr) {
  %res = call spir_func i32 @_Z18__spirv_AtomicLoadPU3AS4iii(i32 addrspace(4)* %ptr, i32 1, i32 16)
  ret i32 %res
}

declare spir_func void @_Z19__spirv_AtomicStorePU3AS4iiii(i32 addrspace(4)*, i32, i32, i32)

define spir_func void @spirv_atomic_store(i32 addrspace(4)* %ptr) {
  call spir_func void @_Z19__spirv_AtomicStorePU3AS4iiii(i32 addrspace(4)* %ptr, i32 1, i32 4, i32 0)
  ret void
}

; CHECK: define internal spir_func {{(noundef )?}}i32 @_Z18__spirv_AtomicLoadPU3AS4iii
; CHECK: define internal spir_func void @_Z19__spirv_AtomicStorePU3AS4iiii
; CHECK: define internal spir_func {{(noundef )?}}i32 @_Z29__spirv_AtomicCompareExchangePU3AS4iiiiii
; CHECK: define internal spir_func {{(noundef )?}}i32 @_Z33__spirv_AtomicCompareExchangeWeakPU3AS4iiiiii
