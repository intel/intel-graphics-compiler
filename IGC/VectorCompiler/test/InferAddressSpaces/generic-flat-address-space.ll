;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -infer-address-spaces -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -infer-address-spaces -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
;
; Verifies that the GenX TargetTransformInfo reports the generic address space
; as the flat address space (GenXTTIImpl::getFlatAddressSpace). LLVM's
; InferAddressSpaces relies on TTI::getFlatAddressSpace() to know which address
; space to infer through. When the hook is reported correctly, a
; local -> generic addrspacecast that only feeds a memory operation is inferred
; back to the underlying local pointer and the cast is removed. Otherwise the
; generic access survives and gets expanded by GenXGASDynamicResolution into
; expensive runtime address-space resolution (extra control flow / phis).
;
; This is a regression guard for the LLVM 22 TTI virtualization: the override
; must be 'const' to actually override the (now virtual) base method, otherwise
; getFlatAddressSpace() returns the base default (InvalidAddressSpace) and
; InferAddressSpaces becomes a no-op.

target datalayout = "e-p:64:64-p3:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i32 addrspace(3)* %local) {
; CHECK-LABEL: spir_kernel void @test(
; CHECK-NOT:         addrspacecast
; CHECK-TYPED-PTRS:  %v = load i32, i32 addrspace(3)* %local, align 4
; CHECK-OPAQUE-PTRS: %v = load i32, ptr addrspace(3) %local, align 4
; CHECK-NOT:         addrspacecast
  %g = addrspacecast i32 addrspace(3)* %local to i32 addrspace(4)*
  %v = load i32, i32 addrspace(4)* %g, align 4
  store i32 %v, i32 addrspace(3)* %local, align 4
  ret void
}
