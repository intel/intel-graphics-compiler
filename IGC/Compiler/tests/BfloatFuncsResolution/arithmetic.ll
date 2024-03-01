;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt %s -S -o - --igc-bfloat-funcs-resolution | FileCheck %s

; This test verifies if bfloat OCL validation functions are lowered correctly.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_addtt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_addft(float, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_addtf(i16 zeroext, float) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_addftt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_addftf(i16 zeroext, float) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_addfft(float, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_subtt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_subft(float, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_subtf(i16 zeroext, float) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_subftt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_subftf(i16 zeroext, float) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_subfft(float, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_multt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_mulft(float, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_multf(i16 zeroext, float) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_mulftt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_mulftf(i16 zeroext, float) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_mulfft(float, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_madttt(i16 zeroext, i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_madftt(float, i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_madttf(i16 zeroext, i16 zeroext, float) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_madfttt(i16 zeroext, i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_madfftt(float, i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func float @_Z19__builtin_bf16_madfttf(i16 zeroext, i16 zeroext, float) #0


; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_add_v1
define spir_kernel void @test_add_v1(i16 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fadd bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_addtt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_add_v2
define spir_kernel void @test_add_v2(i16 addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = fptrunc float %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fadd bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_addft(float %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_add_v3
define spir_kernel void @test_add_v3(i16 addrspace(1)* %out1, i16 zeroext %v1_1, float %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = fptrunc float %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fadd bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_addtf(i16 zeroext %v1_1, float %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_addf_v1(float addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fadd bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = fpext bfloat %[[RES]] to float
  %call = call spir_func float @_Z19__builtin_bf16_addftt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_addf_v2(float addrspace(1)* %out1, i16 zeroext %v1_1, float %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC0F:.*]] = fpext bfloat %[[SRC0BF]] to float
; CHECK: %[[RES:.*]] = fadd float %[[SRC0F]], %v2_1
  %call = call spir_func float @_Z19__builtin_bf16_addftf(i16 zeroext %v1_1, float %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_addf_v3(float addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC1F:.*]] = fpext bfloat %[[SRC1BF]] to float
; CHECK: %[[RES:.*]] = fadd float %v1_1, %[[SRC1F]]
  %call = call spir_func float @_Z19__builtin_bf16_addfft(float %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; FSUB

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_sub_v1
define spir_kernel void @test_sub_v1(i16 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fsub bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_subtt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_sub_v2
define spir_kernel void @test_sub_v2(i16 addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = fptrunc float %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fsub bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_subft(float %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_sub_v3
define spir_kernel void @test_sub_v3(i16 addrspace(1)* %out1, i16 zeroext %v1_1, float %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = fptrunc float %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fsub bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_subtf(i16 zeroext %v1_1, float %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_subf_v1(float addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fsub bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = fpext bfloat %[[RES]] to float
  %call = call spir_func float @_Z19__builtin_bf16_subftt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_subf_v2(float addrspace(1)* %out1, i16 zeroext %v1_1, float %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC0F:.*]] = fpext bfloat %[[SRC0BF]] to float
; CHECK: %[[RES:.*]] = fsub float %[[SRC0F]], %v2_1
  %call = call spir_func float @_Z19__builtin_bf16_subftf(i16 zeroext %v1_1, float %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_subf_v3(float addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC1F:.*]] = fpext bfloat %[[SRC1BF]] to float
; CHECK: %[[RES:.*]] = fsub float %v1_1, %[[SRC1F]]
  %call = call spir_func float @_Z19__builtin_bf16_subfft(float %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; FMUL

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_mul_v1
define spir_kernel void @test_mul_v1(i16 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_multt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_mul_v2
define spir_kernel void @test_mul_v2(i16 addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = fptrunc float %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_mulft(float %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_mul_v3
define spir_kernel void @test_mul_v3(i16 addrspace(1)* %out1, i16 zeroext %v1_1, float %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = fptrunc float %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_multf(i16 zeroext %v1_1, float %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_mulf_v1(float addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[RES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = fpext bfloat %[[RES]] to float
  %call = call spir_func float @_Z19__builtin_bf16_mulftt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_mulf_v2(float addrspace(1)* %out1, i16 zeroext %v1_1, float %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC0F:.*]] = fpext bfloat %[[SRC0BF]] to float
; CHECK: %[[RES:.*]] = fmul float %[[SRC0F]], %v2_1
  %call = call spir_func float @_Z19__builtin_bf16_mulftf(i16 zeroext %v1_1, float %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_mulf_v3(float addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC1F:.*]] = fpext bfloat %[[SRC1BF]] to float
; CHECK: %[[RES:.*]] = fmul float %v1_1, %[[SRC1F]]
  %call = call spir_func float @_Z19__builtin_bf16_mulfft(float %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; MAD

; Function Attrs: convergent nounwind
define spir_kernel void @test_mad_v1(i16 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC2BF:.*]] = bitcast i16 %v3_1 to bfloat
; CHECK: %[[FMULRES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[FADDRES:.*]] = fadd bfloat %[[FMULRES]], %[[SRC2BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[FADDRES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_madttt(i16 zeroext %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_mad_v2(i16 addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = fptrunc float %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC2BF:.*]] = bitcast i16 %v3_1 to bfloat
; CHECK: %[[FMULRES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[FADDRES:.*]] = fadd bfloat %[[FMULRES]], %[[SRC2BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[FADDRES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_madftt(float %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_mad_v3(i16 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1, float %v3_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC2BF:.*]] = fptrunc float %v3_1 to bfloat
; CHECK: %[[FMULRES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[FADDRES:.*]] = fadd bfloat %[[FMULRES]], %[[SRC2BF]]
; CHECK: %{{.*}} = bitcast bfloat %[[FADDRES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_madttf(i16 zeroext %v1_1, i16 zeroext %v2_1, float %v3_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_madf_v1(float addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC2BF:.*]] = bitcast i16 %v3_1 to bfloat
; CHECK: %[[FMULRES:.*]] = fmul bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[FADDRES:.*]] = fadd bfloat %[[FMULRES]], %[[SRC2BF]]
; CHECK: %{{.*}} = fpext bfloat %[[FADDRES]] to float
  %call = call spir_func float @_Z19__builtin_bf16_madfttt(i16 zeroext %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_madf_v2(float addrspace(1)* %out1, float %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #1 {
entry:
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC1F:.*]] = fpext bfloat %[[SRC1BF]] to float
; CHECK: %[[SRC2BF:.*]] = bitcast i16 %v3_1 to bfloat
; CHECK: %[[SRC2F:.*]] = fpext bfloat %[[SRC2BF]] to float
; CHECK: %[[FMULRES:.*]] = fmul float %v1_1, %[[SRC1F]]
; CHECK: %[[FADDRES:.*]] = fadd float %[[FMULRES]], %[[SRC2F]]
  %call = call spir_func float @_Z19__builtin_bf16_madfftt(float %v1_1, i16 zeroext %v2_1, i16 zeroext %v3_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind
define spir_kernel void @test_madf_v3(float addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1, float %v3_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC0F:.*]] = fpext bfloat %[[SRC0BF]] to float
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[SRC1F:.*]] = fpext bfloat %[[SRC1BF]] to float
; CHECK: %[[FMULRES:.*]] = fmul float %[[SRC0F]], %[[SRC1F]]
; CHECK: %[[FADDRES:.*]] = fadd float %[[FMULRES]], %v3_1
  %call = call spir_func float @_Z19__builtin_bf16_madfttf(i16 zeroext %v1_1, i16 zeroext %v2_1, float %v3_1) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %out1, i64 0
  store float %call, float addrspace(1)* %arrayidx, align 4
  ret void
}

attributes #0 = { nounwind "referenced-indirectly" "visaStackCall" }
attributes #1 = { convergent nounwind }
attributes #2 = { nounwind }