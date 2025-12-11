;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-builtin-import -disable-verify -S < %s | FileCheck %s

; CHECK-NOT: call {{.*}}builtin_IB_cast_object_to_generic_ptr
; CHECK: addrspacecast
; CHECK: ptrtoint

define spir_kernel void @test(ptr addrspace(1) %a, <2 x i32> %b) {
entry:
  ; ImageRead uses __builtin_IB_cast_object_to_generic_ptr in its implementation.
  %call = call spir_func <4 x float> @_Z25__spirv_ImageRead_Rfloat4PU3AS133__spirv_Image__void_1_0_0_0_0_0_0Dv2_i(ptr addrspace(1) %a, <2 x i32> %b)
  ret void
}

declare spir_func <4 x float> @_Z25__spirv_ImageRead_Rfloat4PU3AS133__spirv_Image__void_1_0_0_0_0_0_0Dv2_i(ptr addrspace(1), <2 x i32>)
