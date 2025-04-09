;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check 64-bit integer is loaded from unsampled_image_handle and then casted to bindless image handle.

; RUN: igc_opt --typed-pointers -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

%"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" = type { i64 }

define spir_kernel void @image_read(%"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle"* %_arg_imgHandle1) {
entry:
; CHECK:      [[BC:%[0-9]+]] = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle"* %_arg_imgHandle1 to i64*
; CHECK-NEXT: [[LOAD:%[0-9]+]] = load i64, i64* [[BC]], align 8
; CHECK:      %bindless_img = inttoptr i32 %2 to float addrspace(393468)*
; CHECK-NEXT: call <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p196860f32.p393468f32(i32 %CoordX, i32 %CoordY, i32 0, i32 0, float addrspace(196860)* undef, float addrspace(393468)* %bindless_img, i32 0, i32 0, i32 0)

  %0 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle"* %_arg_imgHandle1 to i64*
  %1 = load i64, i64* %0, align 8
  %vecinit3.i.i = insertelement <2 x i32> zeroinitializer, i32 0, i32 0
  %2 = trunc i64 %1 to i32
  %call.i = call spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32 %2, <2 x i32> %vecinit3.i.i, i32 0)
  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32, <2 x i32>, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{void (%"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle"*)* @image_read, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !12}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (%"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle"*)* @image_read}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 4}
!10 = !{!"extensionType", i32 0}
!11 = !{!"indexType", i32 3}
!12 = !{!"UseBindlessImage", i1 true}
