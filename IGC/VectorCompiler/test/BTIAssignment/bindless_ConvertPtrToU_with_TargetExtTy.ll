;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Bindless counterpart of workaround_ConvertPtrToU_with_TargetExtTy.ll.
;
; For bindless resources the argument carries the ExBSO rather than a binding
; table index, so the conversion cannot be folded into a constant. The pass has
; to retype the TargetExtTy argument to a pointer and turn the
; __spirv_ConvertPtrToU call into a real ptrtoint instead.
;
; The pointer must land in the address space the typed pointer representation
; used, which the SPIR-V reader also encodes in the mangled builtin name:
; addrspace(1) for images and buffer surfaces (PU3AS1), addrspace(2) for
; samplers (PU3AS2).

; REQUIRES: llvm_16_or_greater
; RUN: %opt_new_pm_opaque -passes=GenXBTIAssignment -march=genx64 -mcpu=XeHPG \
; RUN:   -vc-use-bindless-images -vc-use-bindless-buffers -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare void @use_value(i32)

declare spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS133__spirv_Image__void_1_0_0_0_0_0_0(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0))
declare spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS129__spirv_BufferSurfaceINTEL__2(target("spirv.BufferSurfaceINTEL", 2))
declare spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS215__spirv_Sampler(target("spirv.Sampler"))

; Surfaces: the arguments are retyped to global pointers and the conversions
; become ptrtoint of the argument, not of a materialized index.
; CHECK-LABEL: define dllexport spir_kernel void @test_bindless_surfaces(
; CHECK-SAME: ptr addrspace(1) %image
; CHECK-SAME: ptr addrspace(1) %buffer
define dllexport spir_kernel void @test_bindless_surfaces(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0) %image,
                                                          target("spirv.BufferSurfaceINTEL", 2) %buffer) #0 {
; CHECK-NOT: call {{.*}}@_Z26__spirv_ConvertPtrToU
; CHECK: [[IMG_BTI:%[^ ]+]] = ptrtoint ptr addrspace(1) %image to i32
; CHECK: [[BUF_BTI:%[^ ]+]] = ptrtoint ptr addrspace(1) %buffer to i32
; CHECK: call void @use_value(i32 [[IMG_BTI]])
; CHECK: call void @use_value(i32 [[BUF_BTI]])
  %img_bti = call spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS133__spirv_Image__void_1_0_0_0_0_0_0(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0) %image)
  %buf_bti = call spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS129__spirv_BufferSurfaceINTEL__2(target("spirv.BufferSurfaceINTEL", 2) %buffer)
  call void @use_value(i32 %img_bti)
  call void @use_value(i32 %buf_bti)
  ret void
}

; Samplers live in the constant address space, not the global one.
; CHECK-LABEL: define dllexport spir_kernel void @test_bindless_sampler(
; CHECK-SAME: ptr addrspace(2) %sampler
define dllexport spir_kernel void @test_bindless_sampler(target("spirv.Sampler") %sampler) #0 {
; CHECK-NOT: call {{.*}}@_Z26__spirv_ConvertPtrToU
; CHECK: [[SMP_BTI:%[^ ]+]] = ptrtoint ptr addrspace(2) %sampler to i32
; CHECK: call void @use_value(i32 [[SMP_BTI]])
  %smp_bti = call spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS215__spirv_Sampler(target("spirv.Sampler") %sampler)
  call void @use_value(i32 %smp_bti)
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0, !5}
!genx.kernel.internal = !{!4, !9}

; Bindless resources get the stateless index reserved for them.
; CHECK: !genx.kernel.internal = !{[[SURF_NODE:![0-9]+]], [[SMP_NODE:![0-9]+]]}
; CHECK-DAG: [[SURF_NODE]] = !{ptr @test_bindless_surfaces, null, null, null, [[SURF_BTIS:![0-9]+]], i32 0}
; CHECK-DAG: [[SURF_BTIS]] = !{i32 255, i32 255}
; CHECK-DAG: [[SMP_NODE]] = !{ptr @test_bindless_sampler, null, null, null, [[SMP_BTIS:![0-9]+]], i32 0}
; CHECK-DAG: [[SMP_BTIS]] = !{i32 255}

; test_bindless_surfaces kernel
!0 = !{ptr @test_bindless_surfaces, !"test_bindless_surfaces", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 2}
!2 = !{i32 0, i32 0}
!3 = !{!"image2d_t read_only", !"buffer_t read_write"}
!4 = !{ptr @test_bindless_surfaces, null, null, null, null}

; test_bindless_sampler kernel
!5 = !{ptr @test_bindless_sampler, !"test_bindless_sampler", !6, i32 0, i32 0, !7, !8, i32 0}
!6 = !{i32 1}
!7 = !{i32 0}
!8 = !{!"sampler_t"}
!9 = !{ptr @test_bindless_sampler, null, null, null, null}
