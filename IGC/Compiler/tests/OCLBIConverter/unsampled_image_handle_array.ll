;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check 64-bit integer is loaded from unsampled_image_handle array and then casted to bindless image handle.

; RUN: igc_opt --typed-pointers -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

%"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" = type { i64 }
%"class.sycl::_V1::id" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }

define spir_kernel void @image_array(i64 %_arg_numImages, %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* %_arg_imgHandleAcc, %"class.sycl::_V1::id"* byval(%"class.sycl::_V1::id") align 8 %_arg_imgHandleAcc3) {
entry:
; CHECK:      [[GEP:%.*]] = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle", %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* %add.ptr.i, i64 %conv.i
; CHECK-NEXT: [[CAST:%[0-9]+]] = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* [[GEP]] to i64 addrspace(1)*
; CHECK-NEXT: [[LOAD:%[0-9]+]] = load i64, i64 addrspace(1)* [[CAST]], align 8
; CHECK:       %bindless_img = inttoptr i32 %4 to float addrspace(393468)*
; CHECK-NEXT: call <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p196860f32.p393468f32(i32 %CoordX, i32 %CoordY, i32 0, i32 0, float addrspace(196860)* undef, float addrspace(393468)* %bindless_img, i32 0, i32 0, i32 0)

  %0 = bitcast %"class.sycl::_V1::id"* %_arg_imgHandleAcc3 to i64*
  %1 = load i64, i64* %0, align 8
  %add.ptr.i = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle", %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* %_arg_imgHandleAcc, i64 %1
  %vecinit = insertelement <2 x i32> zeroinitializer, i32 0, i32 0
  br label %for.cond

for.cond:
  %i = phi i32 [ 0, %entry ], [ %inc.i, %for.body ]
  %conv.i = zext i32 %i to i64
  %cmp.i = icmp ult i64 %conv.i, %_arg_numImages
  br i1 %cmp.i, label %for.body, label %exit

for.body:
  %arrayidx.i38 = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle", %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* %add.ptr.i, i64 %conv.i
  %2 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* %arrayidx.i38 to i64 addrspace(1)*
  %3 = load i64, i64 addrspace(1)* %2, align 8
  %4 = trunc i64 %3 to i32
  %call.i = call spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32 %4, <2 x i32> %vecinit, i32 0)
  %inc.i = add nuw nsw i32 %i, 1
  br label %for.cond

exit:
  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32, <2 x i32>, i32)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{void (i64, %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)*, %"class.sycl::_V1::id"*)* @image_array, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !16}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (i64, %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)*, %"class.sycl::_V1::id"*)* @image_array}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !12, !15}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"indexType", i32 -1}
!12 = !{!"argAllocMDListVec[1]", !13, !10, !14}
!13 = !{!"type", i32 1}
!14 = !{!"indexType", i32 3}
!15 = !{!"argAllocMDListVec[2]", !9, !10, !11}
!16 = !{!"UseBindlessImage", i1 true}
