;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -serialize-igc-metadata -igc-dpas-scan -platformpvc | FileCheck %s
; CHECK: !{!"DisableEUFusion", i1 false}

; .cl source
;
;float  __builtin_IB_sub_group_fdpas_hf_hf_8_1 (float  acc, int  a, int8 b) __attribute__((const));
;
;__kernel void SimpleArg(float acc, int src1, int8 src2, __global float *dst) {
;    for (int i = 0; i < 3; i++) {
;        dst[i] =  __builtin_IB_sub_group_fdpas_hf_hf_8_1(acc, src1, src2);
;    }
;}


declare spir_func float @__builtin_IB_sub_group_fdpas_hf_hf_8_1(float, i32, <8 x i32>) #0

define spir_kernel void @SimpleArg(float %acc, i32 %src1, <8 x i32> %src2, float addrspace(1)* %dst) #1 {
entry:
  %dpas = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %acc, i32 %src1, <8 x i32> %src2, i32 12, i32 12, i32 8, i32 1, i1 false)
  store float %dpas, float addrspace(1)* %dst, align 4
  %dpas.1 = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %acc, i32 %src1, <8 x i32> %src2, i32 12, i32 12, i32 8, i32 1, i1 false)
  %0 = inttoptr i32 4 to float addrspace(131072)*
  store float %dpas.1, float addrspace(131072)* %0, align 4
  %dpas.2 = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %acc, i32 %src1, <8 x i32> %src2, i32 12, i32 12, i32 8, i32 1, i1 false)
  %1 = inttoptr i32 8 to float addrspace(131072)*
  store float %dpas.2, float addrspace(131072)* %1, align 8
  ret void
}

declare float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float, i32, <8 x i32>, i32, i32, i32, i32, i1) #1

!68 = !{!"DisableEUFusion", i1 false}
!igc.functions = !{!332}
!332 = !{void (float, i32, <8 x i32>, float addrspace(1)*)* @SimpleArg, !333}
!333 = !{!334}
!334 = !{!"function_type", i32 0}
