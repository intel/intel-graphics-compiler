;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -serialize-igc-metadata -igc-dpas-scan -platformpvc | FileCheck %s
; CHECK: !{!"DisableEUFusion", i1 true}

; .cl source
;
; float  __builtin_IB_sub_group_fdpas_hf_hf_8_1 (float  acc, int  a, int8 b) __attribute__((const));
;
;__kernel void SimpleArg(float acc, int src1, int8 src2, __global float *dst) {
;    for (int i = 0; i < 3; i++) {
;        dst[i] =  __builtin_IB_sub_group_fdpas_hf_hf_8_1(acc, src1, src2);
;        if (dst[i] < 0.f) break;
;    }
;}


declare spir_func float @__builtin_IB_sub_group_fdpas_hf_hf_8_1(float, i32, <8 x i32>) #0

define spir_kernel void @SimpleArg(float %acc, i32 %src1, <8 x i32> %src2, float addrspace(1)* %dst) #1 {
entry:
  %dpas = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %acc, i32 %src1, <8 x i32> %src2, i32 12, i32 12, i32 8, i32 1, i1 false)
  store float %dpas, float addrspace(1)* %dst, align 4
  %cmp3 = fcmp olt float %dpas, 0.000000e+00
  br i1 %cmp3, label %entry.for.end_crit_edge, label %for.cond

for.cond:                                         ; preds = %entry
  %dpas.1 = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %acc, i32 %src1, <8 x i32> %src2, i32 12, i32 12, i32 8, i32 1, i1 false)
  %0 = inttoptr i32 4 to float addrspace(131072)*
  store float %dpas.1, float addrspace(131072)* %0, align 4
  %cmp3.1 = fcmp olt float %dpas.1, 0.000000e+00
  br i1 %cmp3.1, label %for.cond.for.end_crit_edge, label %for.cond.1

for.cond.1:                                       ; preds = %for.cond
  %dpas.2 = call float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float %acc, i32 %src1, <8 x i32> %src2, i32 12, i32 12, i32 8, i32 1, i1 false)
  %1 = inttoptr i32 8 to float addrspace(131072)*
  store float %dpas.2, float addrspace(131072)* %1, align 8
  br label %UnifiedReturnBlock

for.cond.for.end_crit_edge:                       ; preds = %for.cond
  br label %for.end

entry.for.end_crit_edge:                          ; preds = %entry
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry.for.end_crit_edge
  br label %UnifiedReturnBlock

UnifiedReturnBlock:                               ; preds = %for.cond.1, %for.end
  ret void
}

declare float @llvm.genx.GenISA.sub.group.dpas.f32.f32.i32.v8i32(float, i32, <8 x i32>, i32, i32, i32, i32, i1) #1

!68 = !{!"DisableEUFusion", i1 false}
!igc.functions = !{!332}
!332 = !{void (float, i32, <8 x i32>, float addrspace(1)*)* @SimpleArg, !333}
!333 = !{!334}
!334 = !{!"function_type", i32 0}
