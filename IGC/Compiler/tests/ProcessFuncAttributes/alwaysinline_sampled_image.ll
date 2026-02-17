;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; This software and the related documents are Intel copyrighted materials,
; and your use of them is governed by the express license under which they were
; provided to you ("License"). Unless the License provides otherwise,
; you may not use, modify, copy, publish, distribute, disclose or transmit this
; software or the related documents without Intel's prior written permission.
;
; This software and the related documents are provided as is, with no express or
; implied warranties, other than those that are expressly stated in the License.
;
;============================ end_copyright_notice =============================

; Check alwaysinline attribute is added to following functions:
; 1. function that is returning an image type and its' users, e.g. _ZN4sycl3_V13ext6oneapi12experimental6detail31convert_handle_to_sampled_imageI14ocl_image3d_roNS3_17spirv_handle_typeEEEDaT0_

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-process-func-attributes -S %s -o - | FileCheck %s

; CHECK: define internal spir_func void @_ZZZ4mainENKUlRN4sycl3_V17handlerEE_clES2_ENKUlNS0_7nd_itemILi3EEEE_clES5_() [[MD0:#[0-9]+]]
; CHECK: define internal spir_func void @_ZN4sycl3_V13ext6oneapi12experimental10read_imageINS0_3vecIfLi4EEES6_S6_EET_RKNS3_20sampled_image_handleERKT1_({{.*}}) [[MD0]]
; CHECK: define internal spir_func {{.*}} @_ZN4sycl3_V13ext6oneapi12experimental6detail31convert_handle_to_sampled_imageI14ocl_image3d_roNS3_17spirv_handle_typeEEEDaT0_({{.*}}) [[MD1:#[0-9]+]]
; CHECK: define internal spir_func void @_ZL19__invoke__ImageReadIN4sycl3_V13vecIfLi4EEE32__spirv_SampledImage__image3d_roS3_ET_T0_T1_({{.*}}) [[MD1]]
; CHECK: define internal spir_func {{.*}} @_Z25__spirv_ImageRead_Rfloat4PU3AS140__spirv_SampledImage__void_2_0_0_0_0_0_0Dv4_f({{.*}}) [[MD2:#[0-9]+]]
; CHECK: attributes [[MD0]] = {{.*}} noinline
; CHECK: attributes [[MD1]] = {{.*}} alwaysinline
; CHECK: attributes [[MD2]] = {{.*}} alwaysinline

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::vec" = type { <4 x float> }
%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" = type { %"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type" }
%"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type" = type { i64, i64 }

; Function Attrs: noinline nounwind optnone
define internal spir_func void @_ZZZ4mainENKUlRN4sycl3_V17handlerEE_clES2_ENKUlNS0_7nd_itemILi3EEEE_clES5_() #0 {
entry:
  call spir_func void @_ZN4sycl3_V13ext6oneapi12experimental10read_imageINS0_3vecIfLi4EEES6_S6_EET_RKNS3_20sampled_image_handleERKT1_(%"class.sycl::_V1::vec" addrspace(4)* noalias align 16 null, %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" addrspace(4)* align 8 null, %"class.sycl::_V1::vec" addrspace(4)* align 16 null)
  ret void
}

; Function Attrs: noinline nounwind optnone
define weak_odr spir_func void @_ZN4sycl3_V13ext6oneapi12experimental10read_imageINS0_3vecIfLi4EEES6_S6_EET_RKNS3_20sampled_image_handleERKT1_(%"class.sycl::_V1::vec" addrspace(4)* noalias align 16 %agg.result, %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" addrspace(4)* align 8 dereferenceable(16) %imageHandle, %"class.sycl::_V1::vec" addrspace(4)* align 16 dereferenceable(16) %coords) #0 {
entry:
  %call = call spir_func target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) @_ZN4sycl3_V13ext6oneapi12experimental6detail31convert_handle_to_sampled_imageI14ocl_image3d_roNS3_17spirv_handle_typeEEEDaT0_(%"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type"* byval(%"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type") align 8 null) #0
  call spir_func void @_ZL19__invoke__ImageReadIN4sycl3_V13vecIfLi4EEE32__spirv_SampledImage__image3d_roS3_ET_T0_T1_(%"class.sycl::_V1::vec" addrspace(4)* noalias align 16 %agg.result, target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) %call, %"class.sycl::_V1::vec"* byval(%"class.sycl::_V1::vec") align 16 null) #0
  ret void
}

; Function Attrs: noinline nounwind optnone
define linkonce_odr spir_func target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) @_ZN4sycl3_V13ext6oneapi12experimental6detail31convert_handle_to_sampled_imageI14ocl_image3d_roNS3_17spirv_handle_typeEEEDaT0_(%"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type"* byval(%"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type") align 8 %raw_handle) #0 {
entry:
  %retval = alloca target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0), align 8
  %retval.ascast = addrspacecast target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0)* %retval to target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) addrspace(4)*
  %raw_handle.ascast = addrspacecast %"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type"* %raw_handle to %"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type" addrspace(4)*
  %image = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type", %"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type" addrspace(4)* %raw_handle.ascast, i32 0, i32 0
  %0 = load i64, i64 addrspace(4)* %image, align 8
  %call = call spir_func target("spirv.Image", void, 2, 0, 0, 0, 0, 0, 0) @_Z76__spirv_ConvertHandleToImageINTEL_RPU3AS133__spirv_Image__void_2_0_0_0_0_0_0m(i64 %0)
  %sampler = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type", %"struct.sycl::_V1::ext::oneapi::experimental::spirv_handle_type" addrspace(4)* %raw_handle.ascast, i32 0, i32 1
  %1 = load i64, i64 addrspace(4)* %sampler, align 8
  %call1 = call spir_func target("spirv.Sampler") @_Z35__spirv_ConvertHandleToSamplerINTELm(i64 %1)
  %call2 = call spir_func target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_2_0_0_0_0_0_0PU3AS215__spirv_Sampler(target("spirv.Image", void, 2, 0, 0, 0, 0, 0, 0) %call, target("spirv.Sampler") %call1)
  ret target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) %call2
}

; Function Attrs: noinline nounwind optnone
define hidden spir_func void @_ZL19__invoke__ImageReadIN4sycl3_V13vecIfLi4EEE32__spirv_SampledImage__image3d_roS3_ET_T0_T1_(%"class.sycl::_V1::vec" addrspace(4)* noalias align 16 %agg.result, target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) %Img, %"class.sycl::_V1::vec"* byval(%"class.sycl::_V1::vec") align 16 %Coords) #0 {
entry:
  %call1 = call spir_func <4 x float> @_Z25__spirv_ImageRead_Rfloat4PU3AS140__spirv_SampledImage__void_2_0_0_0_0_0_0Dv4_f(target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) %Img, <4 x float> zeroinitializer)
  ret void
}

; Function Attrs: convergent
define dso_local spir_func <4 x float> @_Z25__spirv_ImageRead_Rfloat4PU3AS140__spirv_SampledImage__void_2_0_0_0_0_0_0Dv4_f(target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) %Image, <4 x float> %Coordinate) #1 {
entry:
  %call.i.i = tail call spir_func i64 @__builtin_IB_get_image(target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) %Image)
  %call1.i.i = tail call spir_func i64 @__builtin_IB_get_sampler(target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) %Image)
  %conv2.i.i = trunc i64 %call1.i.i to i32
  %call3.i.i = tail call spir_func i32 @__builtin_IB_get_snap_wa_reqd(i32 %conv2.i.i)
  %call19.i.i = tail call spir_func <4 x float> @__builtin_IB_OCL_3d_sample_l(i32 0, i32 %conv2.i.i, <4 x float> zeroinitializer, float 0.000000e+00)
  ret <4 x float> %call19.i.i
}

declare spir_func target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0) @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_2_0_0_0_0_0_0PU3AS215__spirv_Sampler(target("spirv.Image", void, 2, 0, 0, 0, 0, 0, 0), target("spirv.Sampler"))

declare spir_func target("spirv.Sampler") @_Z35__spirv_ConvertHandleToSamplerINTELm(i64)

declare spir_func target("spirv.Image", void, 2, 0, 0, 0, 0, 0, 0) @_Z76__spirv_ConvertHandleToImageINTEL_RPU3AS133__spirv_Image__void_2_0_0_0_0_0_0m(i64)

declare spir_func i64 @__builtin_IB_get_image(target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0))

declare spir_func i64 @__builtin_IB_get_sampler(target("spirv.SampledImage", void, 2, 0, 0, 0, 0, 0, 0))

declare spir_func i32 @__builtin_IB_get_snap_wa_reqd(i32)

declare spir_func <4 x float> @__builtin_IB_OCL_3d_sample_l(i32, i32, <4 x float>, float)

attributes #0 = { noinline nounwind optnone }
attributes #1 = { convergent }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}
!IGCMetadata = !{!4}
!opencl.ocl.version = !{!5}
!opencl.spir.version = !{!5}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = distinct !{null, null}
!4 = !{!"ModuleMD"}
!5 = !{i32 2, i32 0}
