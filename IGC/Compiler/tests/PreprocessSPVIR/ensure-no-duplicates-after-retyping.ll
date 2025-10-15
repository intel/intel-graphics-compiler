;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; Verify that the PreprocessSPVIR pass does not leave duplicate alloca instructions.
; During callsite adaptation the pass may emit temporary allocas when function arguments
; are retyped, these allocas are required between transformations to ensure IR
; correctness but should not remain duplicated in the final IR.

%"class.sycl::_V1::sampler" = type { %"class.sycl::_V1::detail::sampler_impl", [8 x i8] }
%"class.sycl::_V1::detail::sampler_impl" = type { target("spirv.Sampler") }
%"class._ZTSZZN13kernel_args__11kernel_args3runERN8sycl_cts4util6loggerEENKUlRN4sycl3_V17handlerEE0_clES8_EUlvE_" = type { %"class.sycl::_V1::sampler" }

define spir_kernel void @_ZTSN13kernel_args__14sampler_kernelE(target("spirv.Sampler") %0) {
  %2 = alloca target("spirv.Sampler"), align 8
  %3 = alloca %class._ZTSZZN13kernel_args__11kernel_args3runERN8sycl_cts4util6loggerEENKUlRN4sycl3_V17handlerEE0_clES8_EUlvE_, align 8
  %4 = addrspacecast ptr %2 to ptr addrspace(4)
  %5 = addrspacecast ptr %3 to ptr addrspace(4)
  store target("spirv.Sampler") %0, ptr addrspace(4) %4, align 8
  %6 = getelementptr inbounds %class._ZTSZZN13kernel_args__11kernel_args3runERN8sycl_cts4util6loggerEENKUlRN4sycl3_V17handlerEE0_clES8_EUlvE_, ptr addrspace(4) %5, i32 0, i32 0
  %7 = getelementptr inbounds %class._ZTSZZN13kernel_args__11kernel_args3runERN8sycl_cts4util6loggerEENKUlRN4sycl3_V17handlerEE0_clES8_EUlvE_, ptr addrspace(4) %5, i32 0, i32 0
  %8 = load target("spirv.Sampler"), ptr addrspace(4) %4, align 8
  call spir_func void @_ZN4sycl3_V17sampler6__initE11ocl_sampler(ptr addrspace(4) align 8 %7, target("spirv.Sampler") %8)
  call spir_func void @_ZZZN13kernel_args__11kernel_args3runERN8sycl_cts4util6loggerEENKUlRN4sycl3_V17handlerEE0_clES8_ENKUlvE_clEv(ptr addrspace(4) align 8 %5)
  ret void
}

define internal spir_func void @_ZN4sycl3_V17sampler6__initE11ocl_sampler(ptr addrspace(4) align 8 %0, target("spirv.Sampler") %1) {
  ret void
}

define internal spir_func void @_ZZZN13kernel_args__11kernel_args3runERN8sycl_cts4util6loggerEENKUlRN4sycl3_V17handlerEE0_clES8_ENKUlvE_clEv(ptr addrspace(4) align 8 %0) {
  ret void
}

; CHECK-LABEL: define spir_kernel void @_ZTSN13kernel_args__14sampler_kernelE
; CHECK: alloca
; CHECK: alloca
; CHECK-NOT: alloca
; CHECK: ret void
