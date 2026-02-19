;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --platformdg2 --igc-sub-group-func-resolution -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------
; This test checks that SubGroupFuncsResolution pass resolves mismatch
; between bfloat16 type passed from SYCL and built-ins accepting i16 type
; ------------------------------------------------

%"class.sycl::_V1::ext::oneapi::bfloat16" = type { i16 }

define spir_kernel void @test_bfloat16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)* %dst, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) #0 {
; CHECK-LABEL: @test_bfloat16(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = bitcast %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[SRC:%.*]] to i16 addrspace(1)*
; CHECK-NEXT:    [[TMP1:%.*]] = call <2 x i16> @llvm.genx.GenISA.simdBlockRead.v2i16.p1i16(i16 addrspace(1)* [[TMP0]])
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)* [[DST:%.*]] to i16 addrspace(3)*
; CHECK-NEXT:    call void @llvm.genx.GenISA.simdBlockWrite.p3i16.v2i16(i16 addrspace(3)* [[TMP2]], <2 x i16> [[TMP1]])
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[SRC]] to i16 addrspace(1)*
; CHECK-NEXT:    [[TMP4:%.*]] = call <16 x i16> @llvm.genx.GenISA.simdBlockRead.v16i16.p1i16(i16 addrspace(1)* [[TMP3]])
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)* [[DST]] to i16 addrspace(3)*
; CHECK-NEXT:    call void @llvm.genx.GenISA.simdBlockWrite.p3i16.v16i16(i16 addrspace(3)* [[TMP5]], <16 x i16> [[TMP4]])
; CHECK-NEXT:    ret void
;
entry:
  %0 = call spir_func <2 x i16> @__builtin_IB_simd_block_read_2_global_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) #0
  call spir_func void @__builtin_IB_simd_block_write_2_local_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)* %dst, <2 x i16> %0) #0
  %1 = call spir_func <16 x i16> @__builtin_IB_simd_block_read_16_global_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %src) #0
  call spir_func void @__builtin_IB_simd_block_write_16_local_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)* %dst, <16 x i16> %1) #0
  ret void
}

declare spir_func <2 x i16> @__builtin_IB_simd_block_read_2_global_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*) #0
declare spir_func void @__builtin_IB_simd_block_write_2_local_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)*, <2 x i16>) #0
declare spir_func <16 x i16> @__builtin_IB_simd_block_read_16_global_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*) #0
declare spir_func void @__builtin_IB_simd_block_write_16_local_h(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)*, <16 x i16>) #0

attributes #0 = { convergent noinline nounwind optnone }

!igc.functions = !{!3}

!3 = !{void (%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(3)*, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*)* @test_bfloat16, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"sub_group_size", i32 8}
