;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm_11_or_less
; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; Checks below ensure that sret attribute is discarded in cases function's
; retun type is transformed

; CHECK: call spir_func <3 x i16> @calle_1(%TestType addrspace(4)* noalias %ret.1.cast, <3 x i16> %{{[^ )]+}})
; CHECK: call spir_func void @calle_2(%TestType addrspace(4)* noalias sret(%TestType) %ret.2.cast)
; CHECK: define internal spir_func <3 x i16> @calle_1(%TestType addrspace(4)* noalias %0, <3 x i16> %{{[^ )]+}})
; CHECK: define spir_func void @calle_2(%TestType addrspace(4)* noalias sret(%TestType) %0)

; ModuleID = 'Deserialized LLVM Module'
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%TestType = type { %array }
%array = type { [1 x i64] }

@__imparg_llvm.genx.local.id16 = internal global <3 x i16> undef

; Function Attrs: noinline nounwind optnone
define dllexport spir_kernel void @kernel() #0 {
  %ret.1 = alloca %TestType, align 8
  %ret.2 = alloca %TestType, align 8
  %ret.1.cast = addrspacecast %TestType* %ret.1 to %TestType addrspace(4)*
  %ret.2.cast = addrspacecast %TestType* %ret.2 to %TestType addrspace(4)*
  call spir_func void @calle_1(%TestType addrspace(4)* noalias sret(%TestType) %ret.1.cast) #2
  call spir_func void @calle_2(%TestType addrspace(4)* noalias sret(%TestType) %ret.2.cast) #2
  ret void
}
; Function Attrs: noinline nounwind optnone
define internal spir_func void @calle_1(%TestType addrspace(4)* noalias sret(%TestType) %0) #1 {
  %2 = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
  ret void
}

define spir_func void @calle_2(%TestType addrspace(4)* noalias sret(%TestType) %0) #1 {
  %2 = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
  ret void
}

attributes #0 = { noinline nounwind optnone "CMGenxMain" "VCFunction" "VCSLMSize"="0" "oclrt"="1" }
attributes #1 = { noinline nounwind optnone "VCFunction" }
attributes #2 = { noinline nounwind optnone }

!genx.kernels = !{!1}
!genx.kernel.internal = !{!2}

!0 = !{}
!1 = !{void ()* @kernel, !"kernel", !0, i32 0, !0, !0, !0, i32 0, i32 0}
!2 = !{void ()* @kernel, null, null, null, null}
