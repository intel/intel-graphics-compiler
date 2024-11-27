;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; The test checks if the FixAlignmentPass is able to correctly set argument alignment
; for the OpenCL kernel arguments. They should be set based on kernel_arg_type metadata
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-fix-alignment -S %s -o %t
; RUN: FileCheck %s --input-file=%t
; ------------------------------------------------

target datalayout = "e-p:32:32-p1:64:64-p2:64:64-p3:32:32-p4:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
; CHECK-LABEL: @test_opencl_types
define spir_kernel void @test_opencl_types(
    ptr addrspace(1) %structparam,
    i8 signext %c,
    i8 zeroext %uc,
    i16 signext %s,
    i16 zeroext %us,
    i32 %i,
    i32 %ui,
    i64 %l,
    i64 %ul,
    float %f,
    double %d,
    half %h,
    ptr addrspace(1) %c_ptr,
    ptr addrspace(1) %uc_ptr,
    ptr addrspace(1) %s_ptr,
    ptr addrspace(1) %us_ptr,
    ptr addrspace(1) %i_ptr,
    ptr addrspace(1) %ui_ptr,
    ptr addrspace(1) %l_ptr,
    ptr addrspace(1) %ul_ptr,
    ptr addrspace(1) %f_ptr,
    ptr addrspace(1) %d_ptr,
    ptr addrspace(1) %c2_ptr,
    ptr addrspace(1) %uc2_ptr,
    ptr addrspace(1) %s2_ptr,
    ptr addrspace(1) %us2_ptr,
    ptr addrspace(1) %i2_ptr,
    ptr addrspace(1) %ui2_ptr,
    ptr addrspace(1) %l2_ptr,
    ptr addrspace(1) %ul2_ptr,
    ptr addrspace(1) %f2_ptr,
    ptr addrspace(1) %d2_ptr,
    ptr addrspace(1) %c3_ptr,
    ptr addrspace(1) %uc3_ptr,
    ptr addrspace(1) %s3_ptr,
    ptr addrspace(1) %us3_ptr,
    ptr addrspace(1) %i3_ptr,
    ptr addrspace(1) %ui3_ptr,
    ptr addrspace(1) %l3_ptr,
    ptr addrspace(1) %ul3_ptr,
    ptr addrspace(1) %f3_ptr,
    ptr addrspace(1) %d3_ptr,
    ptr addrspace(1) %c4_ptr,
    ptr addrspace(1) %uc4_ptr,
    ptr addrspace(1) %s4_ptr,
    ptr addrspace(1) %us4_ptr,
    ptr addrspace(1) %i4_ptr,
    ptr addrspace(1) %ui4_ptr,
    ptr addrspace(1) %l4_ptr,
    ptr addrspace(1) %ul4_ptr,
    ptr addrspace(1) %f4_ptr,
    ptr addrspace(1) %d4_ptr,
    ptr addrspace(1) %c8_ptr,
    ptr addrspace(1) %uc8_ptr,
    ptr addrspace(1) %s8_ptr,
    ptr addrspace(1) %us8_ptr,
    ptr addrspace(1) %i8_ptr,
    ptr addrspace(1) %ui8_ptr,
    ptr addrspace(1) %l8_ptr,
    ptr addrspace(1) %ul8_ptr,
    ptr addrspace(1) %f8_ptr,
    ptr addrspace(1) %d8_ptr,
    ptr addrspace(1) %c16_ptr,
    ptr addrspace(1) %uc16_ptr,
    ptr addrspace(1) %s16_ptr,
    ptr addrspace(1) %us16_ptr,
    ptr addrspace(1) %i16_ptr,
    ptr addrspace(1) %ui16_ptr,
    ptr addrspace(1) %l16_ptr,
    ptr addrspace(1) %ul16_ptr,
    ptr addrspace(1) %f16_ptr,
    ptr addrspace(1) %d16_ptr,
    ptr addrspace(1) %h_ptr,
    ptr addrspace(1) %h2_ptr,
    ptr addrspace(1) %h3_ptr,
    ptr addrspace(1) %h4_ptr,
    ptr addrspace(1) %h8_ptr,
    ptr addrspace(1) %h16_ptr
; CHECK:  ptr addrspace(1) align 1 %c_ptr
; CHECK:  ptr addrspace(1) align 1 %uc_ptr
; CHECK:  ptr addrspace(1) align 2 %s_ptr
; CHECK:  ptr addrspace(1) align 2 %us_ptr
; CHECK:  ptr addrspace(1) align 4 %i_ptr
; CHECK:  ptr addrspace(1) align 4 %ui_ptr
; CHECK:  ptr addrspace(1) align 8 %l_ptr
; CHECK:  ptr addrspace(1) align 8 %ul_ptr
; CHECK:  ptr addrspace(1) align 4 %f_ptr
; CHECK:  ptr addrspace(1) align 8 %d_ptr
; CHECK:  ptr addrspace(1) align 2 %c2_ptr
; CHECK:  ptr addrspace(1) align 2 %uc2_ptr
; CHECK:  ptr addrspace(1) align 4 %s2_ptr
; CHECK:  ptr addrspace(1) align 4 %us2_ptr
; CHECK:  ptr addrspace(1) align 8 %i2_ptr
; CHECK:  ptr addrspace(1) align 8 %ui2_ptr
; CHECK:  ptr addrspace(1) align 16 %l2_ptr
; CHECK:  ptr addrspace(1) align 16 %ul2_ptr
; CHECK:  ptr addrspace(1) align 8 %f2_ptr
; CHECK:  ptr addrspace(1) align 16 %d2_ptr
; CHECK:  ptr addrspace(1) align 4 %c3_ptr
; CHECK:  ptr addrspace(1) align 4 %uc3_ptr
; CHECK:  ptr addrspace(1) align 8 %s3_ptr
; CHECK:  ptr addrspace(1) align 8 %us3_ptr
; CHECK:  ptr addrspace(1) align 16 %i3_ptr
; CHECK:  ptr addrspace(1) align 16 %ui3_ptr
; CHECK:  ptr addrspace(1) align 32 %l3_ptr
; CHECK:  ptr addrspace(1) align 32 %ul3_ptr
; CHECK:  ptr addrspace(1) align 16 %f3_ptr
; CHECK:  ptr addrspace(1) align 32 %d3_ptr
; CHECK:  ptr addrspace(1) align 4 %c4_ptr
; CHECK:  ptr addrspace(1) align 4 %uc4_ptr
; CHECK:  ptr addrspace(1) align 8 %s4_ptr
; CHECK:  ptr addrspace(1) align 8 %us4_ptr
; CHECK:  ptr addrspace(1) align 16 %i4_ptr
; CHECK:  ptr addrspace(1) align 16 %ui4_ptr
; CHECK:  ptr addrspace(1) align 32 %l4_ptr
; CHECK:  ptr addrspace(1) align 32 %ul4_ptr
; CHECK:  ptr addrspace(1) align 16 %f4_ptr
; CHECK:  ptr addrspace(1) align 32 %d4_ptr
; CHECK:  ptr addrspace(1) align 8 %c8_ptr
; CHECK:  ptr addrspace(1) align 8 %uc8_ptr
; CHECK:  ptr addrspace(1) align 16 %s8_ptr
; CHECK:  ptr addrspace(1) align 16 %us8_ptr
; CHECK:  ptr addrspace(1) align 32 %i8_ptr
; CHECK:  ptr addrspace(1) align 32 %ui8_ptr
; CHECK:  ptr addrspace(1) align 64 %l8_ptr
; CHECK:  ptr addrspace(1) align 64 %ul8_ptr
; CHECK:  ptr addrspace(1) align 32 %f8_ptr
; CHECK:  ptr addrspace(1) align 64 %d8_ptr
; CHECK:  ptr addrspace(1) align 16 %c16_ptr
; CHECK:  ptr addrspace(1) align 16 %uc16_ptr
; CHECK:  ptr addrspace(1) align 32 %s16_ptr
; CHECK:  ptr addrspace(1) align 32 %us16_ptr
; CHECK:  ptr addrspace(1) align 64 %i16_ptr
; CHECK:  ptr addrspace(1) align 64 %ui16_ptr
; CHECK:  ptr addrspace(1) align 128 %l16_ptr
; CHECK:  ptr addrspace(1) align 128 %ul16_ptr
; CHECK:  ptr addrspace(1) align 64 %f16_ptr
; CHECK:  ptr addrspace(1) align 128 %d16_ptr
; CHECK:  ptr addrspace(1) align 2 %h_ptr
; CHECK:  ptr addrspace(1) align 4 %h2_ptr
; CHECK:  ptr addrspace(1) align 8 %h3_ptr
; CHECK:  ptr addrspace(1) align 8 %h4_ptr
; CHECK:  ptr addrspace(1) align 16 %h8_ptr
; CHECK:  ptr addrspace(1) align 32 %h16_ptr
    ) #0 !kernel_arg_addr_space !6 !kernel_arg_access_qual !7 !kernel_arg_type !8 !kernel_arg_type_qual !9 !kernel_arg_base_type !10 !kernel_arg_name !11 !spirv.ParameterDecorations !12 {
entry:
  ret void
}

; The test checks if the pass does not override the alignment that is already set.
; CHECK-LABEL: @test_no_override
define spir_kernel void @test_no_override(
    ptr addrspace(1) %structparam,
    i8 signext %c,
    i8 zeroext %uc,
    i16 signext %s,
    i16 zeroext %us,
    i32 %i,
    i32 %ui,
    i64 %l,
    i64 %ul,
    float %f,
    double %d,
    half %h,
    ptr addrspace(1) align 16  %c_ptr,
    ptr addrspace(1) align 16  %uc_ptr,
    ptr addrspace(1) align 16  %s_ptr,
    ptr addrspace(1) align 16  %us_ptr,
    ptr addrspace(1) align 16  %i_ptr,
    ptr addrspace(1) align 16  %ui_ptr,
    ptr addrspace(1) align 16  %l_ptr,
    ptr addrspace(1) align 16  %ul_ptr,
    ptr addrspace(1) align 16  %f_ptr,
    ptr addrspace(1) align 16  %d_ptr,
    ptr addrspace(1) align 16  %c2_ptr,
    ptr addrspace(1) align 16  %uc2_ptr,
    ptr addrspace(1) align 16  %s2_ptr,
    ptr addrspace(1) align 16  %us2_ptr,
    ptr addrspace(1) align 16  %i2_ptr,
    ptr addrspace(1) align 16  %ui2_ptr,
    ptr addrspace(1) align 16  %l2_ptr,
    ptr addrspace(1) align 16  %ul2_ptr,
    ptr addrspace(1) align 16  %f2_ptr,
    ptr addrspace(1) align 16  %d2_ptr,
    ptr addrspace(1) align 16  %c3_ptr,
    ptr addrspace(1) align 16  %uc3_ptr,
    ptr addrspace(1) align 16  %s3_ptr,
    ptr addrspace(1) align 16  %us3_ptr,
    ptr addrspace(1) align 16  %i3_ptr,
    ptr addrspace(1) align 16  %ui3_ptr,
    ptr addrspace(1) align 16  %l3_ptr,
    ptr addrspace(1) align 16  %ul3_ptr,
    ptr addrspace(1) align 16  %f3_ptr,
    ptr addrspace(1) align 16  %d3_ptr,
    ptr addrspace(1) align 16  %c4_ptr,
    ptr addrspace(1) align 16  %uc4_ptr,
    ptr addrspace(1) align 16  %s4_ptr,
    ptr addrspace(1) align 16  %us4_ptr,
    ptr addrspace(1) align 16  %i4_ptr,
    ptr addrspace(1) align 16  %ui4_ptr,
    ptr addrspace(1) align 16  %l4_ptr,
    ptr addrspace(1) align 16  %ul4_ptr,
    ptr addrspace(1) align 16  %f4_ptr,
    ptr addrspace(1) align 16  %d4_ptr,
    ptr addrspace(1) align 16  %c8_ptr,
    ptr addrspace(1) align 16  %uc8_ptr,
    ptr addrspace(1) align 16  %s8_ptr,
    ptr addrspace(1) align 16  %us8_ptr,
    ptr addrspace(1) align 16  %i8_ptr,
    ptr addrspace(1) align 16  %ui8_ptr,
    ptr addrspace(1) align 16  %l8_ptr,
    ptr addrspace(1) align 16  %ul8_ptr,
    ptr addrspace(1) align 16  %f8_ptr,
    ptr addrspace(1) align 16  %d8_ptr,
    ptr addrspace(1) align 16  %c16_ptr,
    ptr addrspace(1) align 16  %uc16_ptr,
    ptr addrspace(1) align 16  %s16_ptr,
    ptr addrspace(1) align 16  %us16_ptr,
    ptr addrspace(1) align 16  %i16_ptr,
    ptr addrspace(1) align 16  %ui16_ptr,
    ptr addrspace(1) align 16  %l16_ptr,
    ptr addrspace(1) align 16  %ul16_ptr,
    ptr addrspace(1) align 16  %f16_ptr,
    ptr addrspace(1) align 16  %d16_ptr,
    ptr addrspace(1) align 16  %h_ptr,
    ptr addrspace(1) align 16  %h2_ptr,
    ptr addrspace(1) align 16  %h3_ptr,
    ptr addrspace(1) align 16  %h4_ptr,
    ptr addrspace(1) align 16  %h8_ptr,
    ptr addrspace(1) align 16  %h16_ptr
; CHECK:  ptr addrspace(1) align 16 %c_ptr
; CHECK:  ptr addrspace(1) align 16 %uc_ptr
; CHECK:  ptr addrspace(1) align 16 %s_ptr
; CHECK:  ptr addrspace(1) align 16 %us_ptr
; CHECK:  ptr addrspace(1) align 16 %i_ptr
; CHECK:  ptr addrspace(1) align 16 %ui_ptr
; CHECK:  ptr addrspace(1) align 16 %l_ptr
; CHECK:  ptr addrspace(1) align 16 %ul_ptr
; CHECK:  ptr addrspace(1) align 16 %f_ptr
; CHECK:  ptr addrspace(1) align 16 %d_ptr
; CHECK:  ptr addrspace(1) align 16 %c2_ptr
; CHECK:  ptr addrspace(1) align 16 %uc2_ptr
; CHECK:  ptr addrspace(1) align 16 %s2_ptr
; CHECK:  ptr addrspace(1) align 16 %us2_ptr
; CHECK:  ptr addrspace(1) align 16 %i2_ptr
; CHECK:  ptr addrspace(1) align 16 %ui2_ptr
; CHECK:  ptr addrspace(1) align 16 %l2_ptr
; CHECK:  ptr addrspace(1) align 16 %ul2_ptr
; CHECK:  ptr addrspace(1) align 16 %f2_ptr
; CHECK:  ptr addrspace(1) align 16 %d2_ptr
; CHECK:  ptr addrspace(1) align 16 %c3_ptr
; CHECK:  ptr addrspace(1) align 16 %uc3_ptr
; CHECK:  ptr addrspace(1) align 16 %s3_ptr
; CHECK:  ptr addrspace(1) align 16 %us3_ptr
; CHECK:  ptr addrspace(1) align 16 %i3_ptr
; CHECK:  ptr addrspace(1) align 16 %ui3_ptr
; CHECK:  ptr addrspace(1) align 16 %l3_ptr
; CHECK:  ptr addrspace(1) align 16 %ul3_ptr
; CHECK:  ptr addrspace(1) align 16 %f3_ptr
; CHECK:  ptr addrspace(1) align 16 %d3_ptr
; CHECK:  ptr addrspace(1) align 16 %c4_ptr
; CHECK:  ptr addrspace(1) align 16 %uc4_ptr
; CHECK:  ptr addrspace(1) align 16 %s4_ptr
; CHECK:  ptr addrspace(1) align 16 %us4_ptr
; CHECK:  ptr addrspace(1) align 16 %i4_ptr
; CHECK:  ptr addrspace(1) align 16 %ui4_ptr
; CHECK:  ptr addrspace(1) align 16 %l4_ptr
; CHECK:  ptr addrspace(1) align 16 %ul4_ptr
; CHECK:  ptr addrspace(1) align 16 %f4_ptr
; CHECK:  ptr addrspace(1) align 16 %d4_ptr
; CHECK:  ptr addrspace(1) align 16 %c8_ptr
; CHECK:  ptr addrspace(1) align 16 %uc8_ptr
; CHECK:  ptr addrspace(1) align 16 %s8_ptr
; CHECK:  ptr addrspace(1) align 16 %us8_ptr
; CHECK:  ptr addrspace(1) align 16 %i8_ptr
; CHECK:  ptr addrspace(1) align 16 %ui8_ptr
; CHECK:  ptr addrspace(1) align 16 %l8_ptr
; CHECK:  ptr addrspace(1) align 16 %ul8_ptr
; CHECK:  ptr addrspace(1) align 16 %f8_ptr
; CHECK:  ptr addrspace(1) align 16 %d8_ptr
; CHECK:  ptr addrspace(1) align 16 %c16_ptr
; CHECK:  ptr addrspace(1) align 16 %uc16_ptr
; CHECK:  ptr addrspace(1) align 16 %s16_ptr
; CHECK:  ptr addrspace(1) align 16 %us16_ptr
; CHECK:  ptr addrspace(1) align 16 %i16_ptr
; CHECK:  ptr addrspace(1) align 16 %ui16_ptr
; CHECK:  ptr addrspace(1) align 16 %l16_ptr
; CHECK:  ptr addrspace(1) align 16 %ul16_ptr
; CHECK:  ptr addrspace(1) align 16 %f16_ptr
; CHECK:  ptr addrspace(1) align 16 %d16_ptr
; CHECK:  ptr addrspace(1) align 16 %h_ptr
; CHECK:  ptr addrspace(1) align 16 %h2_ptr
; CHECK:  ptr addrspace(1) align 16 %h3_ptr
; CHECK:  ptr addrspace(1) align 16 %h4_ptr
; CHECK:  ptr addrspace(1) align 16 %h8_ptr
; CHECK:  ptr addrspace(1) align 16 %h16_ptr
    ) #0 !kernel_arg_addr_space !6 !kernel_arg_access_qual !7 !kernel_arg_type !8 !kernel_arg_type_qual !9 !kernel_arg_base_type !10 !kernel_arg_name !11 !spirv.ParameterDecorations !12
{
  entry:
    ret void
}
; This case checks if the pass ignores that the argument with ptr byval does not have "*" in kernel_arg_type metadata and does not modify the alignment.
; CHECK-LABEL: @test_dpcpp_no_asterisk
; CHECK: ptr addrspace(1) align 1 %0
; CHECK: ptr byval(%"class.sycl::_V1::id") align 8 %1
%"class.sycl::_V1::id" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }
define spir_kernel void @test_dpcpp_no_asterisk(ptr addrspace(1) align 1 %0, ptr byval(%"class.sycl::_V1::id") align 8 %1) #0 !kernel_arg_type !17 {
entry:
  %2 = bitcast ptr %1 to ptr
  %3 = load i64, ptr %2, align 8
  %4 = getelementptr inbounds i8, ptr addrspace(1) %0, i64 %3
  store i8 1, ptr addrspace(1) %4, align 1
  ret void
}

attributes #0 = { nounwind }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 102000}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{!"cl_doubles"}
!5 = !{i16 6, i16 14}
!6 = !{i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1}
!7 = !{!"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!8 = !{!"struct test_struct*", !"char", !"uchar", !"short", !"ushort", !"int", !"uint", !"long", !"ulong", !"float", !"double", !"half", !"char*", !"uchar*", !"short*", !"ushort*", !"int*", !"uint*", !"long*", !"ulong*", !"float*", !"double*", !"char2*", !"uchar2*", !"short2*", !"ushort2*", !"int2*", !"uint2*", !"long2*", !"ulong2*", !"float2*", !"double2*", !"char3*", !"uchar3*", !"short3*", !"ushort3*", !"int3*", !"uint3*", !"long3*", !"ulong3*", !"float3*", !"double3*", !"char4*", !"uchar4*", !"short4*", !"ushort4*", !"int4*", !"uint4*", !"long4*", !"ulong4*", !"float4*", !"double4*", !"char8*", !"uchar8*", !"short8*", !"ushort8*", !"int8*", !"uint8*", !"long8*", !"ulong8*", !"float8*", !"double8*", !"char16*", !"uchar16*", !"short16*", !"ushort16*", !"int16*", !"uint16*", !"long16*", !"ulong16*", !"float16*", !"double16*", !"half*", !"half2*", !"half3*", !"half4*", !"half8*", !"half16*"}
!9 = !{!"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !"", !""}
!10 = !{!"struct test_struct*", !"char", !"uchar", !"short", !"ushort", !"int", !"int", !"long", !"long", !"float", !"double", !"half", !"char*", !"char*", !"short*", !"short*", !"int*", !"int*", !"long*", !"long*", !"float*", !"double*", !"char2*", !"char2*", !"short2*", !"short2*", !"int2*", !"int2*", !"long2*", !"long2*", !"float2*", !"double2*", !"char3*", !"char3*", !"short3*", !"short3*", !"int3*", !"int3*", !"long3*", !"long3*", !"float3*", !"double3*", !"char4*", !"char4*", !"short4*", !"short4*", !"int4*", !"int4*", !"long4*", !"long4*", !"float4*", !"double4*", !"char8*", !"char8*", !"short8*", !"short8*", !"int8*", !"int8*", !"long8*", !"long8*", !"float8*", !"double8*", !"char16*", !"char16*", !"short16*", !"short16*", !"int16*", !"int16*", !"long16*", !"long16*", !"float16*", !"double16*", !"half*", !"half2*", !"half3*", !"half4*", !"half8*", !"half16*"}
!11 = !{!"structparam", !"c", !"uc", !"s", !"us", !"i", !"ui", !"l", !"ul", !"f", !"d", !"h", !"c_ptr", !"uc_ptr", !"s_ptr", !"us_ptr", !"i_ptr", !"ui_ptr", !"l_ptr", !"ul_ptr", !"f_ptr", !"d_ptr", !"c2_ptr", !"uc2_ptr", !"s2_ptr", !"us2_ptr", !"i2_ptr", !"ui2_ptr", !"l2_ptr", !"ul2_ptr", !"f2_ptr", !"d2_ptr", !"c3_ptr", !"uc3_ptr", !"s3_ptr", !"us3_ptr", !"i3_ptr", !"ui3_ptr", !"l3_ptr", !"ul3_ptr", !"f3_ptr", !"d3_ptr", !"c4_ptr", !"uc4_ptr", !"s4_ptr", !"us4_ptr", !"i4_ptr", !"ui4_ptr", !"l4_ptr", !"ul4_ptr", !"f4_ptr", !"d4_ptr", !"c8_ptr", !"uc8_ptr", !"s8_ptr", !"us8_ptr", !"i8_ptr", !"ui8_ptr", !"l8_ptr", !"ul8_ptr", !"f8_ptr", !"d8_ptr", !"c16_ptr", !"uc16_ptr", !"s16_ptr", !"us16_ptr", !"i16_ptr", !"ui16_ptr", !"l16_ptr", !"ul16_ptr", !"f16_ptr", !"d16_ptr", !"h_ptr", !"h2_ptr", !"h3_ptr", !"h4_ptr", !"h8_ptr", !"h16_ptr"}
!12 = !{!3, !13, !15, !13, !15, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!13 = !{!14}
!14 = !{i32 38, i32 1}
!15 = !{!16}
!16 = !{i32 38, i32 0}
!17 = !{!"char*", !"class.sycl::_V1::id"}
