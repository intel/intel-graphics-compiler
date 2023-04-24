;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGASDynamicResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

declare <8 x float> @llvm.masked.gather.v8f32.v8p4f32(<8 x float addrspace(4)*>, i32 immarg, <8 x i1>, <8 x float>)
declare void @llvm.masked.scatter.v8f32.v8p4f32(<8 x float>, <8 x float addrspace(4)*>, i32 immarg, <8 x i1>)

define spir_kernel void @kernelA(<8 x float addrspace(3)*> %local_v_ptrs) #0 {
  ; CHECK: %[[V8P3_2_V8I64:[^ ]+]] = ptrtoint <8 x float addrspace(3)*> %local_v_ptrs to <8 x i64>
  ; CHECK: %[[V8P3_2_V16I32:[^ ]+]] = bitcast <8 x i64> %[[V8P3_2_V8I64:[^ ]+]] to <16 x i32>
  ; CHECK: %[[TAGGED_V16I32:[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %[[V8P3_2_V16I32:[^ ]+]], <8 x i32> <i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824>, i32 2, i32 1, i32 0, i16 4, i32 undef, i1 true)
  ; CHECK: %[[V16I32_2_V8I64:[^ ]+]] = bitcast <16 x i32> %[[TAGGED_V16I32:[^ ]+]] to <8 x i64>
  ; CHECK: %[[V8I64_2_V8P3:[^ ]+]] = inttoptr <8 x i64> %[[V16I32_2_V8I64:[^ ]+]] to <8 x float addrspace(3)*>
  ; CHECK: %generic_v_ptrs = addrspacecast <8 x float addrspace(3)*> %[[V8I64_2_V8P3:[^ ]+]] to <8 x float addrspace(4)*>
  %generic_v_ptrs = addrspacecast <8 x float addrspace(3)*> %local_v_ptrs to <8 x float addrspace(4)*>

  ; CHECK: %[[GT_V8P4__2_V8I64:[^ ]+]] = ptrtoint <8 x float addrspace(4)*> %generic_v_ptrs to <8 x i64>
  ; CHECK: %[[GT_V8P4_2_V16I32:[^ ]+]] = bitcast <8 x i64> %[[GT_V8P4__2_V8I64:[^ ]+]] to <16 x i32>
  ; CHECK: %[[GT_V8P4_HIGH32:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %[[GT_V8P4_2_V16I32:[^ ]+]], i32 2, i32 1, i32 0, i16 4, i32 undef)
  ; CHECK: %[[GT_ISLOCAL:[^ ]+]] = icmp eq <8 x i32> %[[GT_V8P4_HIGH32:[^ ]+]], <i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824>
  ; CHECK: %[[GT_ISGLOBAL:[^ ]+]] = xor <8 x i1> %[[GT_ISLOCAL:[^ ]+]], <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>

  ; CHECK: %[[GT_LOCAL_MASK:[^ ]+]] = and <8 x i1> %[[GT_ISLOCAL:[^ ]+]], <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  ; CHECK: %[[GT_V8P4_2_V8P3:[^ ]+]] = addrspacecast <8 x float addrspace(4)*> %generic_v_ptrs to <8 x float addrspace(3)*>
  ; CHECK: %val.local = call <8 x float> @llvm.masked.gather.v8f32.v8p3f32(<8 x float addrspace(3)*> %[[GT_V8P4_2_V8P3:[^ ]+]], i32 4, <8 x i1> %[[GT_LOCAL_MASK:[^ ]+]], <8 x float> undef)

  ; CHECK: %[[GT_GLOBAL_MASK:[^ ]+]] = and <8 x i1> %[[GT_ISGLOBAL:[^ ]+]], <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  ; CHECK: %[[GT_V8P4_2_V8P1:[^ ]+]] = addrspacecast <8 x float addrspace(4)*> %generic_v_ptrs to <8 x float addrspace(1)*>
  ; CHECK: %val.global = call <8 x float> @llvm.masked.gather.v8f32.v8p1f32(<8 x float addrspace(1)*> %[[GT_V8P4_2_V8P1:[^ ]+]], i32 4, <8 x i1> %[[GT_GLOBAL_MASK:[^ ]+]], <8 x float> undef)

  ; CHECK: %[[GATHER_RESULT:[^ ]+]] = select <8 x i1> %[[GT_ISLOCAL:[^ ]+]], <8 x float>  %val.local, <8 x float> %val.global
  %val = call <8 x float> @llvm.masked.gather.v8f32.v8p4f32(<8 x float addrspace(4)*> %generic_v_ptrs, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <8 x float> undef)

  ; CHECK: %[[SC_V8P4_2_V8I64:[^ ]+]] = ptrtoint <8 x float addrspace(4)*> %generic_v_ptrs to <8 x i64>
  ; CHECK: %[[SC_V8P4_2_V16I32:[^ ]+]] = bitcast <8 x i64> %[[SC_V8P4_2_V8I64:[^ ]+]] to <16 x i32>
  ; CHECK: %[[SC_V8P4_HIGH32:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %[[SC_V8P4_2_V16I32:[^ ]+]], i32 2, i32 1, i32 0, i16 4, i32 undef)
  ; CHECK: %[[SC_ISLOCAL:[^ ]+]] = icmp eq <8 x i32> %[[SC_V8P4_HIGH32:[^ ]+]], <i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824, i32 1073741824>
  ; CHECK: %[[SC_ISGLOBAL:[^ ]+]] = xor <8 x i1> %[[SC_ISLOCAL:[^ ]+]], <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>

  ; CHECK: %[[SCATTER_LOCAL_MASK:[^ ]+]] = and <8 x i1> %[[SC_ISLOCAL:[^ ]+]], <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  ; CHECK: %[[SC_V8P4_2_V8P3:[^ ]+]] = addrspacecast <8 x float addrspace(4)*> %generic_v_ptrs to <8 x float addrspace(3)*>
  ; CHECK: call void @llvm.masked.scatter.v8f32.v8p3f32(<8 x float> %[[GATHER_RESULT:[^ ]+]], <8 x float addrspace(3)*> %[[SC_V8P4_2_V8P3:[^ ]+]], i32 4, <8 x i1> %[[SCATTER_LOCAL_MASK:[^ ]+]])

  ; CHECK: %[[SC_GLOBAL_MASK:[^ ]+]] = and <8 x i1> %[[SC_ISGLOBAL:[^ ]+]], <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  ; CHECK: %[[SC_V8P4_2_V8P1:[^ ]+]] = addrspacecast <8 x float addrspace(4)*> %generic_v_ptrs to <8 x float addrspace(1)*>
  ; CHECK: call void @llvm.masked.scatter.v8f32.v8p1f32(<8 x float> %[[GATHER_RESULT:[^ ]+]], <8 x float addrspace(1)*> %[[SC_V8P4_2_V8P1:[^ ]+]], i32 4, <8 x i1> %[[SC_GLOBAL_MASK:[^ ]+]])
  call void @llvm.masked.scatter.v8f32.v8p4f32(<8 x float> %val, <8 x float addrspace(4)*> %generic_v_ptrs, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  ret void
}

define dllexport spir_kernel void @kernelB(<8 x float addrspace(1)*> %global_v_ptrs) #0 {
  %generic_v_ptrs = addrspacecast <8 x float addrspace(1)*> %global_v_ptrs to <8 x float addrspace(4)*>

  ; CHECK: %[[GLOBAL_V_PTR_GATHER:[^ ]+]] = addrspacecast <8 x float addrspace(4)*> %generic_v_ptrs to <8 x float addrspace(1)*>
  ; CHECK: %[[VAL:[^ ]+]] = call <8 x float> @llvm.masked.gather.v8f32.v8p1f32(<8 x float addrspace(1)*> %[[GLOBAL_V_PTR_GATHER:[^ ]+]], i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <8 x float> undef)
  %val = call <8 x float> @llvm.masked.gather.v8f32.v8p4f32(<8 x float addrspace(4)*> %generic_v_ptrs, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <8 x float> undef)

  ; CHECK: %[[GLOBAL_V_PTR_SCATTER:[^ ]+]] = addrspacecast <8 x float addrspace(4)*> %generic_v_ptrs to <8 x float addrspace(1)*>
  ; CHECK: call void @llvm.masked.scatter.v8f32.v8p1f32(<8 x float> %[[VAL:[^ ]+]], <8 x float addrspace(1)*> %[[GLOBAL_V_PTR_SCATTER:[^ ]+]], i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  call void @llvm.masked.scatter.v8f32.v8p4f32(<8 x float> %val, <8 x float addrspace(4)*> %generic_v_ptrs, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }
