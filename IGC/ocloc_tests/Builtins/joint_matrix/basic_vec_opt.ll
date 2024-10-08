;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: pvc-supported

; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-cl-intel-enable-auto-large-GRF-mode -igc_opts 'PrintToConsole=1,PrintAfter=Layout,JointMatrixLoadStoreOpt=2'" 2>&1 | FileCheck %s

target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::ext::oneapi::bfloat16" = type { i16 }

define spir_kernel void @test(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_a, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_b, float addrspace(1)* %src_c) !intel_reqd_sub_group_size !100 {
entry:
  %ma1x32 = alloca <2 x i16>
  %a1 = bitcast <2 x i16>* %ma1x32 to i8*
  %ma32x32 = alloca <64 x i16>
  %a = bitcast <64 x i16>* %ma32x32 to i8*
  %mb = alloca <64 x i32>
  %b = bitcast <64 x i32>* %mb to i8*
  %mc1x64 = alloca <4 x float>
  %c1 = bitcast <4 x float>* %mc1x64 to i8*
  %mc32x64 = alloca { <64 x float>, <64 x float> }
  %c = bitcast { <64 x float>, <64 x float> }* %mc32x64 to i8*

; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_v8i8_pi32_i32(i8* %a1, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_a, i64 32, i32 0)

; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_a, i8* %a1, i64 32, i32 0)

; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
; CHECK: call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)*
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_v8i8_pi32_i32(i8* %a, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_a, i64 32, i32 0)

; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v2i16(i16 addrspace(1)* %{{[0-9]+}}, <2 x i16> %{{[0-9]+}})
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_a, i8* %a, i64 32, i32 0)

; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8* %b, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_b, i64 128, i32 0)

; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_b, i8* %b, i64 128, i32 0)

; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
; CHECK: call <4 x i16> @llvm.genx.GenISA.simdBlockRead.v4i16.p1i16(i16 addrspace(1)*
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8* %b, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_b, i64 64, i32 0)

; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i16.v4i16(i16 addrspace(1)* %{{[0-9]+}}, <4 x i16> %{{[0-9]+}})
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src_b, i8* %b, i64 64, i32 0)

; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)*
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_v8i8_pi32_i32(i8* %c, float addrspace(1)* %src_c, i64 64, i32 0)

; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_pi64_v8i8(float addrspace(1)* %src_c, i8* %c, i64 64, i32 0)

; CHECK: call <4 x i32> @llvm.genx.GenISA.simdBlockRead.v4i32.p1i32(i32 addrspace(1)* %{{[0-9]+}})
  call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_v8i8_pi32_i32(i8* %c1, float addrspace(1)* %src_c, i64 64, i32 0)

; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1i32.v4i32(i32 addrspace(1)* %{{[0-9]+}}, <4 x i32> %{{[0-9]+}})
  call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_pi64_v8i8(float addrspace(1)* %src_c, i8* %c1, i64 64, i32 0)

  ret void
}

declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_v8i8_pi32_i32(i8*, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_v8i8_pi32_i32(i8*, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8*, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8*, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_v8i8_pi32_i32(i8*, float addrspace(1)*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_v8i8_pi32_i32(i8*, float addrspace(1)*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_1x32_i16_2_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i8*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_32x32_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i8*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_32x64_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i8*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_RowMajor_SG16_32x64_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i8*, i64, i32)

declare void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_pi64_v8i8(float addrspace(1)*, i8*, i64, i32)
declare void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_pi64_v8i8(float addrspace(1)*, i8*, i64, i32)

!100 = !{i32 16}
