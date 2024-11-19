;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm_11_or_less
; COM: default behavior: structure is passed by value if possible + size is less then threshold (see vc-max-cmabi-byval-size option).
; COM: all structures of the test are supposed to be less then this threshold.
; RUN: %opt_typed_ptrs %use_old_pass_manager% %pass_pref%CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=SMALL-ON,SMALL-ON-TYPED-PTRS,BIG-ON,BIG-ON-TYPED-PTRS,COMMON,COMMON-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% %pass_pref%CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=SMALL-ON,SMALL-ON-OPAQUE-PTRS,BIG-ON,BIG-ON-OPAQUE-PTRS,COMMON,COMMON-OPAQUE-PTRS

; COM: vc-max-cmabi-byval-size=0: turn off byval generation for CM ABI.
; RUN: %opt_typed_ptrs %use_old_pass_manager% %pass_pref%CMABI -vc-max-cmabi-byval-size=0 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=SMALL-OFF,SMALL-OFF-TYPED-PTRS,BIG-OFF-TYPED-PTRS,COMMON,COMMON-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% %pass_pref%CMABI -vc-max-cmabi-byval-size=0 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=SMALL-OFF,SMALL-OFF-OPAQUE-PTRS,BIG-OFF-OPAQUE-PTRS,COMMON,COMMON-OPAQUE-PTRS

; COM: vc-max-cmabi-byval-size=8: very small threshold for byval generation for CM ABI.
; RUN: %opt_typed_ptrs %use_old_pass_manager% %pass_pref%CMABI -vc-max-cmabi-byval-size=8 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=SMALL-ON,SMALL-ON-TYPED-PTRS,BIG-OFF-TYPED-PTRS,COMMON,COMMON-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% %pass_pref%CMABI -vc-max-cmabi-byval-size=8 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=SMALL-ON,SMALL-ON-OPAQUE-PTRS,BIG-OFF-OPAQUE-PTRS,COMMON,COMMON-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%struct4 = type { float }
%struct24 = type { <2 x float>, <2 x float>, <2 x float>* }
%struct4s = type { %struct4 }

declare <8 x float> @llvm.genx.svm.block.ld.v8f32.i64(i64)

; COM: struct4 tests basic functionality

; COM: internal function:
; COM:   - transform according to CMABI threashold even without byval attribute
; COM:   - copyin
; COMMON-LABEL: define internal spir_func float @store_float_internal
; SMALL-OFF-TYPED-PTRS: (%struct4* %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; SMALL-OFF-OPAQUE-PTRS: (ptr %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; SMALL-ON: (%struct4 %[[INPUT_STR:[^ ]+]], float %[[INPUT_FLOAT:[^ ]+]])
define internal spir_func void @store_float_internal(%struct4* %s_ptr, float* %f_ptr) #0 {
  %s = load %struct4, %struct4* %s_ptr
  %f = extractvalue %struct4 %s, 0
  store float %f, float* %f_ptr
  ret void
; SMALL-ON-NEXT: %s_ptr = alloca %struct4
; SMALL-ON-TYPED-PTRS-NEXT: store %struct4 %[[INPUT_STR]], %struct4* %s_ptr
; SMALL-ON-OPAQUE-PTRS-NEXT: store %struct4 %[[INPUT_STR]], ptr %s_ptr
; COMMON-NEXT: %f_ptr = alloca float
; COMMON-TYPED-PTRS-NEXT: store float %[[INPUT_FLOAT]], float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %s = load %struct4, %struct4* %s_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %[[INPUT_FLOAT]], ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %s = load %struct4, ptr %s_ptr
; COMMON-NEXT: %f = extractvalue %struct4 %s, 0
; COMMON-TYPED-PTRS-NEXT: store float %f, float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, float* %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %f, ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, ptr %f_ptr
; COMMON-NEXT: ret float %[[FLOAT_VAL]]
}

; COM: internal function:
; COM:   - transform according to cmabi threshold
; COM:   - copyin
; COMMON-LABEL: define internal spir_func float @store_float_internal_byval
; SMALL-TYPED-PTRS-OFF: (%struct4* byval(%struct4) %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; SMALL-OPAQUE-PTRS-OFF: (ptr byval(%struct4) %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; SMALL-ON: (%struct4 %[[INPUT_STR:[^ ]+]], float %[[INPUT_FLOAT:[^ ]+]])
define internal spir_func void @store_float_internal_byval(%struct4* byval(%struct4) %s_ptr, float* %f_ptr) #0 {
  %s = load %struct4, %struct4* %s_ptr
  %f = extractvalue %struct4 %s, 0
  store float %f, float* %f_ptr
  ret void
; SMALL-OFF-NEXT: %[[COPY_STR:[^ ]+]] = alloca %struct4, align 8
; SMALL-OFF-TYPED-PTRS-NEXT: %[[MEMCPY_DST:[^ ]+]] = bitcast %struct4* %[[COPY_STR]] to i8*
; SMALL-OFF-TYPED-PTRS-NEXT: %[[MEMCPY_SRC:[^ ]+]] = bitcast %struct4* %s_ptr to i8*
; SMALL-OFF-TYPED-PTRS-NEXT: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %[[MEMCPY_DST]], i8* %[[MEMCPY_SRC]], i64 4, i1 false)
; SMALL-OFF-OPAQUE-PTRS-NEXT: %[[MEMCPY_DST:[^ ]+]] = bitcast ptr %[[COPY_STR]] to ptr
; SMALL-OFF-OPAQUE-PTRS-NEXT: %[[MEMCPY_SRC:[^ ]+]] = bitcast ptr %s_ptr to ptr
; SMALL-OFF-OPAQUE-PTRS-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr %[[MEMCPY_DST]], ptr %[[MEMCPY_SRC]], i64 4, i1 false)
; SMALL-ON-NEXT: %s_ptr = alloca %struct4
; SMALL-ON-TYPED-PTRS-NEXT: store %struct4 %[[INPUT_STR]], %struct4* %s_ptr
; SMALL-ON-OPAQUE-PTRS-NEXT: store %struct4 %[[INPUT_STR]], ptr %s_ptr
; COMMON-NEXT: %f_ptr = alloca float
; COMMON-TYPED-PTRS-NEXT: store float %[[INPUT_FLOAT]], float* %f_ptr
; SMALL-OFF-TYPED-PTRS-NEXT: %s = load %struct4, %struct4* %[[COPY_STR]]
; SMALL-ON-TYPED-PTRS-NEXT: %s = load %struct4, %struct4* %s_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %[[INPUT_FLOAT]], ptr %f_ptr
; SMALL-OFF-OPAQUE-PTRS-NEXT: %s = load %struct4, ptr %[[COPY_STR]]
; SMALL-ON-OPAQUE-PTRS-NEXT: %s = load %struct4, ptr %s_ptr
; COMMON-NEXT: %f = extractvalue %struct4 %s, 0
; COMMON-TYPED-PTRS-NEXT: store float %f, float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, float* %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %f, ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, ptr %f_ptr
; COMMON-NEXT: ret float %[[FLOAT_VAL]]
}

; COM: not internal function with byval attribute:
; COM:   - never transform
; COMMON-LABEL: define spir_func void @store_float
; COMMON-TYPED-PTRS: (%struct4* byval(%struct4) %s_ptr, float* %f_ptr)
; COMMON-OPAQUE-PTRS: (ptr byval(%struct4) %s_ptr, ptr %f_ptr)
define spir_func void @store_float(%struct4* byval(%struct4) %s_ptr, float* %f_ptr) #0 {
  %s = load %struct4, %struct4* %s_ptr
  %f = extractvalue %struct4 %s, 0
  store float %f, float* %f_ptr
  ret void
; COMMON-NEXT: %[[COPY_STR:[^ ]+]] = alloca %struct4, align 8
; COMMON-TYPED-PTRS-NEXT: %[[MEMCPY_DST:[^ ]+]] = bitcast %struct4* %[[COPY_STR]] to i8*
; COMMON-TYPED-PTRS-NEXT: %[[MEMCPY_SRC:[^ ]+]] = bitcast %struct4* %s_ptr to i8*
; COMMON-TYPED-PTRS-NEXT: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %[[MEMCPY_DST]], i8* %[[MEMCPY_SRC]], i64 4, i1 false)
; COMMON-TYPED-PTRS-NEXT: %s = load %struct4, %struct4* %[[COPY_STR]]
; COMMON-OPAQUE-PTRS-NEXT: %[[MEMCPY_DST:[^ ]+]] = bitcast ptr %[[COPY_STR]] to ptr
; COMMON-OPAQUE-PTRS-NEXT: %[[MEMCPY_SRC:[^ ]+]] = bitcast ptr %s_ptr to ptr
; COMMON-OPAQUE-PTRS-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr %[[MEMCPY_DST]], ptr %[[MEMCPY_SRC]], i64 4, i1 false)
; COMMON-OPAQUE-PTRS-NEXT: %s = load %struct4, ptr %[[COPY_STR]]
; COMMON-NEXT: %f = extractvalue %struct4 %s, 0
; COMMON-TYPED-PTRS-NEXT: store float %f, float* %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %f, ptr %f_ptr
; COMMON-NEXT: ret void
}

; COM: internal function with array access on ptr:
; COM:   - never transform
; COMMON-LABEL: define internal spir_func float @store_float_array
; COMMON-TYPED-PTRS: (%struct4* %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; COMMON-OPAQUE-PTRS: (ptr %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
define internal spir_func void @store_float_array(%struct4* %s_ptr, float* %f_ptr) #0 {
  %load_ptr = getelementptr %struct4, %struct4* %s_ptr, i32 1, i32 0
  %f = load float, float* %load_ptr
  store float %f, float* %f_ptr
  ret void
; COMMON-NEXT: %f_ptr = alloca float
; COMMON-TYPED-PTRS-NEXT: store float %[[INPUT_FLOAT]], float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %load_ptr = getelementptr %struct4, %struct4* %s_ptr, i32 1, i32 0
; COMMON-TYPED-PTRS-NEXT: %f = load float, float* %load_ptr
; COMMON-TYPED-PTRS-NEXT: store float %f, float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, float* %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %[[INPUT_FLOAT]], ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %load_ptr = getelementptr %struct4, ptr %s_ptr, i32 1, i32 0
; COMMON-OPAQUE-PTRS-NEXT: %f = load float, ptr %load_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %f, ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, ptr %f_ptr
; COMMON-NEXT: ret float %[[FLOAT_VAL]]
}

; COM: internal function with sideeffect on struct:
; COM:   - only copyin structures supported but copyinout is needed here
; COM:   - never transform
; COMMON-LABEL: define internal spir_func void @store_float_to_struct
; COMMON-TYPED-PTRS: (%struct4* %s_ptr, float %f)
; COMMON-OPAQUE-PTRS: (ptr %s_ptr, float %f)
define internal spir_func void @store_float_to_struct(%struct4* %s_ptr, float %f) #0 {
  %store_ptr = getelementptr %struct4, %struct4* %s_ptr, i32 0, i32 0
  store float %f, float* %store_ptr
  ret void
; COMMON-TYPED-PTRS-NEXT: %store_ptr = getelementptr %struct4, %struct4* %s_ptr, i32 0, i32 0
; COMMON-TYPED-PTRS-NEXT: store float %f, float* %store_ptr
; COMMON-OPAQUE-PTRS-NEXT: %store_ptr = getelementptr %struct4, ptr %s_ptr, i32 0, i32 0
; COMMON-OPAQUE-PTRS-NEXT: store float %f, ptr %store_ptr
; COMMON-NEXT: ret void
}

; COM: struct24 tests thresholds

; COM: internal function:
; COM:   - transform according to cmabi threshold
; COM:   - copyin
; COMMON-LABEL: define internal spir_func void @store_sum_internal
; BIG-OFF-TYPED-PTRS: (%struct24* %s_ptr)
; BIG-OFF-OPAQUE-PTRS: (ptr %s_ptr)
; BIG-ON: (%struct24 %[[INPUT_STR:[^ ]+]])
define internal spir_func void @store_sum_internal(%struct24* %s_ptr) #0 {
  %ptr1 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 0
  %ptr2 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 1
  %ptr3 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 2
  %fv1 = load <2 x float>, <2 x float>* %ptr1
  %fv2 = load <2 x float>, <2 x float>* %ptr2
  %fv_ptr = load <2 x float>*, <2 x float>** %ptr3
  %sum = fadd <2 x float> %fv1, %fv2
  store <2 x float> %sum, <2 x float>* %fv_ptr
  ret void
; BIG-ON-NEXT: %s_ptr = alloca %struct24
; BIG-ON-TYPED-PTRS-NEXT: store %struct24 %[[INPUT_STR]], %struct24* %s_ptr
; COMMON-TYPED-PTRS-NEXT: %ptr1 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 0
; COMMON-TYPED-PTRS-NEXT: %ptr2 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 1
; COMMON-TYPED-PTRS-NEXT: %ptr3 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 2
; COMMON-TYPED-PTRS-NEXT: %fv1 = load <2 x float>, <2 x float>* %ptr1
; COMMON-TYPED-PTRS-NEXT: %fv2 = load <2 x float>, <2 x float>* %ptr2
; COMMON-TYPED-PTRS-NEXT: %fv_ptr = load <2 x float>*, <2 x float>** %ptr3
; BIG-ON-OPAQUE-PTRS-NEXT: store %struct24 %[[INPUT_STR]], ptr %s_ptr
; COMMON-OPAQUE-PTRS-NEXT: %ptr1 = getelementptr %struct24, ptr %s_ptr, i32 0, i32 0
; COMMON-OPAQUE-PTRS-NEXT: %ptr2 = getelementptr %struct24, ptr %s_ptr, i32 0, i32 1
; COMMON-OPAQUE-PTRS-NEXT: %ptr3 = getelementptr %struct24, ptr %s_ptr, i32 0, i32 2
; COMMON-OPAQUE-PTRS-NEXT: %fv1 = load <2 x float>, ptr %ptr1
; COMMON-OPAQUE-PTRS-NEXT: %fv2 = load <2 x float>, ptr %ptr2
; COMMON-OPAQUE-PTRS-NEXT: %fv_ptr = load ptr, ptr %ptr3
; COMMON-NEXT: %sum = fadd <2 x float> %fv1, %fv2
; COMMON-TYPED-PTRS-NEXT: store <2 x float> %sum, <2 x float>* %fv_ptr
; COMMON-OPAQUE-PTRS-NEXT: store <2 x float> %sum, ptr %fv_ptr
; COMMON-NEXT: ret void
}

; COM: not internal function:
; COM:   - never transform
; COMMON-LABEL: define spir_func void @store_sum
; COMMON-TYPED-PTRS: (%struct24* byval(%struct24) %s_ptr)
; COMMON-OPAQUE-PTRS: (ptr byval(%struct24) %s_ptr)
define spir_func void @store_sum(%struct24* byval(%struct24) %s_ptr) #0 {
  %ptr1 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 0
  %ptr2 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 1
  %ptr3 = getelementptr %struct24, %struct24* %s_ptr, i32 0, i32 2
  %fv1 = load <2 x float>, <2 x float>* %ptr1
  %fv2 = load <2 x float>, <2 x float>* %ptr2
  %fv_ptr = load <2 x float>*, <2 x float>** %ptr3
  %sum = fadd <2 x float> %fv1, %fv2
  store <2 x float> %sum, <2 x float>* %fv_ptr
  ret void
; COMMON-NEXT: %[[COPY_STR:[^ ]+]] = alloca %struct24, align 8
; COMMON-TYPED-PTRS-NEXT: %[[MEMCPY_DST:[^ ]+]] = bitcast %struct24* %[[COPY_STR]] to i8*
; COMMON-TYPED-PTRS-NEXT: %[[MEMCPY_SRC:[^ ]+]] = bitcast %struct24* %s_ptr to i8*
; COMMON-TYPED-PTRS-NEXT: call void @llvm.memcpy.p0i8.p0i8.i64(i8* %[[MEMCPY_DST]], i8* %[[MEMCPY_SRC]], i64 24, i1 false)
; COMMON-TYPED-PTRS-NEXT: %ptr1 = getelementptr %struct24, %struct24* %[[COPY_STR]], i32 0, i32 0
; COMMON-TYPED-PTRS-NEXT: %ptr2 = getelementptr %struct24, %struct24* %[[COPY_STR]], i32 0, i32 1
; COMMON-TYPED-PTRS-NEXT: %ptr3 = getelementptr %struct24, %struct24* %[[COPY_STR]], i32 0, i32 2
; COMMON-TYPED-PTRS-NEXT: %fv1 = load <2 x float>, <2 x float>* %ptr1
; COMMON-TYPED-PTRS-NEXT: %fv2 = load <2 x float>, <2 x float>* %ptr2
; COMMON-TYPED-PTRS-NEXT: %fv_ptr = load <2 x float>*, <2 x float>** %ptr3
; COMMON-OPAQUE-PTRS-NEXT: %[[MEMCPY_DST:[^ ]+]] = bitcast ptr %[[COPY_STR]] to ptr
; COMMON-OPAQUE-PTRS-NEXT: %[[MEMCPY_SRC:[^ ]+]] = bitcast ptr %s_ptr to ptr
; COMMON-OPAQUE-PTRS-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr %[[MEMCPY_DST]], ptr %[[MEMCPY_SRC]], i64 24, i1 false)
; COMMON-OPAQUE-PTRS-NEXT: %ptr1 = getelementptr %struct24, ptr %[[COPY_STR]], i32 0, i32 0
; COMMON-OPAQUE-PTRS-NEXT: %ptr2 = getelementptr %struct24, ptr %[[COPY_STR]], i32 0, i32 1
; COMMON-OPAQUE-PTRS-NEXT: %ptr3 = getelementptr %struct24, ptr %[[COPY_STR]], i32 0, i32 2
; COMMON-OPAQUE-PTRS-NEXT: %fv1 = load <2 x float>, ptr %ptr1
; COMMON-OPAQUE-PTRS-NEXT: %fv2 = load <2 x float>, ptr %ptr2
; COMMON-OPAQUE-PTRS-NEXT: %fv_ptr = load ptr, ptr %ptr3
; COMMON-NEXT: %sum = fadd <2 x float> %fv1, %fv2
; COMMON-TYPED-PTRS-NEXT: store <2 x float> %sum, <2 x float>* %fv_ptr
; COMMON-OPAQUE-PTRS-NEXT: store <2 x float> %sum, ptr %fv_ptr
; COMMON-NEXT: ret void
}

; COM: struct4s tests structure with structure inside.
; COM: currently not supported for cmabi.

; COM: internal function:
; COM:   - never transform: structure's type is not supported
; COMMON-LABEL: define internal spir_func float @store_float_str_internal
; COMMON-TYPED-PTRS: (%struct4s* %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; COMMON-OPAQUE-PTRS: (ptr %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
define internal spir_func void @store_float_str_internal(%struct4s* %s_ptr, float* %f_ptr) #0 {
  %s_s = load %struct4s, %struct4s* %s_ptr
  %s = extractvalue %struct4s %s_s, 0
  %f = extractvalue %struct4 %s, 0
  store float %f, float* %f_ptr
  ret void
; COMMON-NEXT: %f_ptr = alloca float
; COMMON-TYPED-PTRS-NEXT: store float %[[INPUT_FLOAT]], float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %s_s = load %struct4s, %struct4s* %s_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %[[INPUT_FLOAT]], ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %s_s = load %struct4s, ptr %s_ptr
; COMMON-NEXT: %s = extractvalue %struct4s %s_s, 0
; COMMON-NEXT: %f = extractvalue %struct4 %s, 0
; COMMON-TYPED-PTRS-NEXT: store float %f, float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, float* %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %f, ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %[[FLOAT_VAL:[^ ]+]] = load float, ptr %f_ptr
; COMMON-NEXT: ret float %[[FLOAT_VAL]]
}

; COM: test sret with copyout functionality

; COM: internal function with sret attribute:
; COM:   - transform according to cmabi threshold
; COM:   - copyout
; SMALL-OFF-LABEL: define internal spir_func void @get_str_internal
; SMALL-OFF-TYPED-PTRS: (%struct4* %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; SMALL-OFF-OPAQUE-PTRS: (ptr %s_ptr, float %[[INPUT_FLOAT:[^ ]+]])
; SMALL-ON-LABEL: define internal spir_func %struct4 @get_str_internal
; SMALL-ON: (float %[[INPUT_FLOAT:[^ ]+]])
define internal spir_func void @get_str_internal(%struct4* sret(%struct4) %s_ptr, float* %f_ptr) #0 {
  %f = load float, float* %f_ptr
  %s = insertvalue %struct4 undef, float %f, 0
  store %struct4 %s, %struct4* %s_ptr
  ret void
; SMALL-ON-NEXT: %s_ptr = alloca %struct4
; COMMON-NEXT: %f_ptr = alloca float
; COMMON-TYPED-PTRS-NEXT: store float %[[INPUT_FLOAT]], float* %f_ptr
; COMMON-TYPED-PTRS-NEXT: %f = load float, float* %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: store float %[[INPUT_FLOAT]], ptr %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %f = load float, ptr %f_ptr
; COMMON-NEXT: %s = insertvalue %struct4 undef, float %f, 0
; COMMON-TYPED-PTRS-NEXT: store %struct4 %s, %struct4* %s_ptr
; SMALL-ON-TYPED-PTRS-NEXT: %[[UPD_STR:[^ ]+]] = load %struct4, %struct4* %s_ptr
; COMMON-OPAQUE-PTRS-NEXT: store %struct4 %s, ptr %s_ptr
; SMALL-ON-OPAQUE-PTRS-NEXT: %[[UPD_STR:[^ ]+]] = load %struct4, ptr %s_ptr
; SMALL-ON-NEXT: ret %struct4 %[[UPD_STR]]
; SMALL-OFF-NEXT: ret void
}

; COM: not internal function with sret attribute:
; COM:   - never transform
; COM:   - copyout
; COMMON-LABEL: define spir_func void @get_str
; COMMON-TYPED-PTRS: (%struct4* sret(%struct4) %s_ptr, float* %f_ptr)
; COMMON-OPAQUE-PTRS: (ptr sret(%struct4) %s_ptr, ptr %f_ptr)
define spir_func void @get_str(%struct4* sret(%struct4) %s_ptr, float* %f_ptr) #0 {
  %f = load float, float* %f_ptr
  %s = insertvalue %struct4 undef, float %f, 0
  store %struct4 %s, %struct4* %s_ptr
  ret void
; COMMON-TYPED-PTRS-NEXT: %f = load float, float* %f_ptr
; COMMON-OPAQUE-PTRS-NEXT: %f = load float, ptr %f_ptr
; COMMON-NEXT: %s = insertvalue %struct4 undef, float %f, 0
; COMMON-TYPED-PTRS-NEXT: store %struct4 %s, %struct4* %s_ptr
; COMMON-OPAQUE-PTRS-NEXT: store %struct4 %s, ptr %s_ptr
; COMMON-NEXT: ret void
}

; COM: kernel
; COMMON-LABEL: kern
define spir_kernel void @kern(float* %RET, float* %aFOO, i64 %privBase) #1 {

; COM: allocas
  %str4_ptr = alloca %struct4, i32 2
  %str24_ptr = alloca %struct24
  %str4s_ptr = alloca %struct4s
  %f_value = load float, float* %aFOO

; COM: store_float_internal
  call spir_func void @store_float_internal(%struct4* %str4_ptr, float* %RET)
; SMALL-OFF-TYPED-PTRS: %[[RET_LOAD1:[^ ]+]] = load float, float* %RET
; SMALL-OFF-TYPED-PTRS-NEXT: %[[RET1:[^ ]+]] = call spir_func float @store_float_internal(%struct4* %str4_ptr, float %[[RET_LOAD1]])
; SMALL-ON-TYPED-PTRS: %[[STR_VAL1:[^ ]+]] = load %struct4, %struct4* %str4_ptr
; SMALL-ON-TYPED-PTRS-NEXT: %[[RET_LOAD1:[^ ]+]] = load float, float* %RET
; SMALL-OFF-OPAQUE-PTRS: %[[RET_LOAD1:[^ ]+]] = load float, ptr %RET
; SMALL-OFF-OPAQUE-PTRS-NEXT: %[[RET1:[^ ]+]] = call spir_func float @store_float_internal(ptr %str4_ptr, float %[[RET_LOAD1]])
; SMALL-ON-OPAQUE-PTRS: %[[STR_VAL1:[^ ]+]] = load %struct4, ptr %str4_ptr
; SMALL-ON-OPAQUE-PTRS-NEXT: %[[RET_LOAD1:[^ ]+]] = load float, ptr %RET
; SMALL-ON-NEXT: %[[RET1:[^ ]+]] = call spir_func float @store_float_internal(%struct4 %[[STR_VAL1]], float %[[RET_LOAD1]])
; COMMON-TYPED-PTRS-NEXT: store float %[[RET1]], float* %RET
; COMMON-OPAQUE-PTRS-NEXT: store float %[[RET1]], ptr %RET

; COM: store_float_internal_byval
  call spir_func void @store_float_internal_byval(%struct4* byval(%struct4) %str4_ptr, float* %RET)
; SMALL-OFF-TYPED-PTRS: %[[RET_LOAD2:[^ ]+]] = load float, float* %RET
; SMALL-OFF-TYPED-PTRS-NEXT: %[[RET2:[^ ]+]] = call spir_func float @store_float_internal_byval(%struct4* byval(%struct4) %str4_ptr, float %[[RET_LOAD2]])
; SMALL-ON-TYPED-PTRS: %[[STR_VAL2:[^ ]+]] = load %struct4, %struct4* %str4_ptr
; SMALL-ON-TYPED-PTRS-NEXT: %[[RET_LOAD2:[^ ]+]] = load float, float* %RET
; SMALL-OFF-OPAQUE-PTRS: %[[RET_LOAD2:[^ ]+]] = load float, ptr %RET
; SMALL-OFF-OPAQUE-PTRS-NEXT: %[[RET2:[^ ]+]] = call spir_func float @store_float_internal_byval(ptr byval(%struct4) %str4_ptr, float %[[RET_LOAD2]])
; SMALL-ON-OPAQUE-PTRS: %[[STR_VAL2:[^ ]+]] = load %struct4, ptr %str4_ptr
; SMALL-ON-OPAQUE-PTRS-NEXT: %[[RET_LOAD2:[^ ]+]] = load float, ptr %RET
; SMALL-ON-NEXT: %[[RET2:[^ ]+]] = call spir_func float @store_float_internal_byval(%struct4 %[[STR_VAL2]], float %[[RET_LOAD2]])
; COMMON-TYPED-PTRS-NEXT: store float %[[RET2]], float* %RET
; COMMON-OPAQUE-PTRS-NEXT: store float %[[RET2]], ptr %RET

; COM: store_float
  call spir_func void @store_float(%struct4* byval(%struct4) %str4_ptr, float* %RET)
; COMMON-TYPED-PTRS: call spir_func void @store_float(%struct4* byval(%struct4) %str4_ptr, float* %RET)
; COMMON-OPAQUE-PTRS: call spir_func void @store_float(ptr byval(%struct4) %str4_ptr, ptr %RET)

; COM: store_float_array
  call spir_func void @store_float_array(%struct4* %str4_ptr, float* %RET)
; COMMON-TYPED-PTRS: %[[RET_LOAD5:[^ ]+]] = load float, float* %RET
; COMMON-TYPED-PTRS-NEXT: %[[RET5:[^ ]+]] = call spir_func float @store_float_array(%struct4* %str4_ptr, float %[[RET_LOAD5]])
; COMMON-TYPED-PTRS-NEXT: store float %[[RET5]], float* %RET
; COMMON-OPAQUE-PTRS: %[[RET_LOAD5:[^ ]+]] = load float, ptr %RET
; COMMON-OPAQUE-PTRS-NEXT: %[[RET5:[^ ]+]] = call spir_func float @store_float_array(ptr %str4_ptr, float %[[RET_LOAD5]])
; COMMON-OPAQUE-PTRS-NEXT: store float %[[RET5]], ptr %RET

; COM: store_float_to_struct
  call spir_func void @store_float_to_struct(%struct4* %str4_ptr, float %f_value)
; COMMON-TYPED-PTRS: call spir_func void @store_float_to_struct(%struct4* %str4_ptr, float %f_value)
; COMMON-OPAQUE-PTRS: call spir_func void @store_float_to_struct(ptr %str4_ptr, float %f_value)

; COM: store_sum_internal
  call spir_func void @store_sum_internal(%struct24* %str24_ptr)
; BIG-OFF-TYPED-PTRS: call spir_func void @store_sum_internal(%struct24* %str24_ptr)
; BIG-ON-TYPED-PTRS: %[[STR_VAL7:[^ ]+]] = load %struct24, %struct24* %str24_ptr
; BIG-OFF-OPAQUE-PTRS: call spir_func void @store_sum_internal(ptr %str24_ptr)
; BIG-ON-OPAQUE-PTRS: %[[STR_VAL7:[^ ]+]] = load %struct24, ptr %str24_ptr
; BIG-ON-NEXT: call spir_func void @store_sum_internal(%struct24 %[[STR_VAL7]])

; COM: store_sum
  call spir_func void @store_sum(%struct24* byval(%struct24) %str24_ptr)
; COMMON-TYPED-PTRS: call spir_func void @store_sum(%struct24* byval(%struct24) %str24_ptr)
; COMMON-OPAQUE-PTRS: call spir_func void @store_sum(ptr byval(%struct24) %str24_ptr)

; COM: store_float_str_internal
  call spir_func void @store_float_str_internal(%struct4s* %str4s_ptr, float* %RET)
; COMMON-TYPED-PTRS: %[[RET_LOAD10:[^ ]+]] = load float, float* %RET
; COMMON-TYPED-PTRS-NEXT: %[[RET10:[^ ]+]] = call spir_func float @store_float_str_internal(%struct4s* %str4s_ptr, float %[[RET_LOAD10]])
; COMMON-TYPED-PTRS-NEXT: store float %[[RET10]], float* %RET
; COMMON-OPAQUE-PTRS: %[[RET_LOAD10:[^ ]+]] = load float, ptr %RET
; COMMON-OPAQUE-PTRS-NEXT: %[[RET10:[^ ]+]] = call spir_func float @store_float_str_internal(ptr %str4s_ptr, float %[[RET_LOAD10]])
; COMMON-OPAQUE-PTRS-NEXT: store float %[[RET10]], ptr %RET

; COM: get_str_internal
  call spir_func void @get_str_internal(%struct4* sret(%struct4) %str4_ptr, float* %RET)
; COMMON-TYPED-PTRS: %[[RET_LOAD11:[^ ]+]] = load float, float* %RET
; SMALL-OFF-TYPED-PTRS-NEXT: call spir_func void @get_str_internal(%struct4* %str4_ptr, float %[[RET_LOAD11]])
; COMMON-OPAQUE-PTRS: %[[RET_LOAD11:[^ ]+]] = load float, ptr %RET
; SMALL-OFF-OPAQUE-PTRS-NEXT: call spir_func void @get_str_internal(ptr %str4_ptr, float %[[RET_LOAD11]])
; SMALL-ON-NEXT: %[[RET11:[^ ]+]] = call spir_func %struct4 @get_str_internal(float %[[RET_LOAD11]])
; SMALL-ON-TYPED-PTRS-NEXT: store %struct4 %[[RET11]], %struct4* %str4_ptr
; SMALL-ON-OPAQUE-PTRS-NEXT: store %struct4 %[[RET11]], ptr %str4_ptr

; COM: get_str
  call spir_func void @get_str(%struct4* sret(%struct4) %str4_ptr, float* %RET)
; COMMON-TYPED-PTRS: call spir_func void @get_str(%struct4* sret(%struct4) %str4_ptr, float* %RET)
; COMMON-OPAQUE-PTRS: call spir_func void @get_str(ptr sret(%struct4) %str4_ptr, ptr %RET)

  ret void
}

attributes #0 = { noinline nounwind "CMStackCall" }
attributes #1 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}

!0 = !{i32 1, !"genx.useGlobalMem", i32 1}
!1 = !{i32 0, i32 0}
!genx.kernels = !{!2}
!genx.kernel.internal = !{!7}
!2 = !{void (float*, float*, i64)* @kern, !"kern", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 0, i32 0, i32 96}
!4 = !{i32 72, i32 80, i32 64}
!5 = !{i32 0, i32 0}
!6 = !{!"", !""}
!7 = !{void (float*, float*, i64)* @kern, !8, !9, !10, null}
!8 = !{i32 0, i32 0, i32 0}
!9 = !{i32 0, i32 1, i32 2}
!10 = !{}
