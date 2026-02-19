;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: system-windows, llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-clone-address-arithmetic --regkey=RematChainLimit=10 --regkey=RematFlowThreshold=100 --regkey=RematRPELimit=0 --dce | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

%"struct.ONEAPIKernelContext::LightSample" = type { %struct.packed_float3, %struct.packed_float3, %struct.packed_float3, float, float, float, float, float, float, i32, i32, i32, i32, i32, i32 }
%struct.packed_float3 = type { float, float, float }

declare spir_func void @target_func(%"struct.ONEAPIKernelContext::LightSample" addrspace(4)*)
declare spir_func void @target_func_bitcast_no_ptr(double)

define spir_kernel void @main() {
entry:
  %ls.i = alloca %"struct.ONEAPIKernelContext::LightSample", i32 0, align 4
  br label %if.end46

if.end46:
  %ls.ascast.i = addrspacecast %"struct.ONEAPIKernelContext::LightSample"* %ls.i to %"struct.ONEAPIKernelContext::LightSample" addrspace(4)*
  br label %cleanup.cont.i
; CHECK-LABEL: cleanup.cont.i:
; CHECK-NEXT: [[REMAT:%.*]] = alloca %"struct.ONEAPIKernelContext::LightSample", i32 0, align 4
; CHECK-NEXT: [[CAST:%.*]] = addrspacecast %"struct.ONEAPIKernelContext::LightSample"* [[REMAT]] to %"struct.ONEAPIKernelContext::LightSample" addrspace(4)*
; CHECK-NEXT: call spir_func void @target_func(%"struct.ONEAPIKernelContext::LightSample" addrspace(4)* [[CAST]])
cleanup.cont.i:                                   ; preds = %if.end46
  call spir_func void @target_func(%"struct.ONEAPIKernelContext::LightSample" addrspace(4)* %ls.ascast.i)
  ret void
}

define spir_func void @bar(i64 %arg1) {
entry:
  %val2 = mul i64 %arg1, 2
  %val3 = mul i64 %arg1, 3
  %cast_val2 = bitcast i64 %val2 to double
  %cast_val3 = bitcast i64 %val3 to double
  ; CHECK: [[VAL2:%.*]] = mul i64 [[ARG1:%.*]], 2
  ; CHECK: [[VAL3:%.*]] = mul i64 [[ARG1]], 3
  ; CHECK: bitcast i64 [[VAL2]] to double
  ; CHECK: bitcast i64 [[VAL3]] to double
  ; CHECK: br label %if.end46
  %ls.i = alloca %"struct.ONEAPIKernelContext::LightSample", i32 0, align 4
  br label %if.end46

if.end46:
  %ls.ascast.i = addrspacecast %"struct.ONEAPIKernelContext::LightSample"* %ls.i to %"struct.ONEAPIKernelContext::LightSample" addrspace(4)*
  br label %cleanup.cont.i
; CHECK-LABEL: cleanup.cont.i:
; CHECK-NEXT: [[REMAT:%.*]] = alloca %"struct.ONEAPIKernelContext::LightSample", i32 0, align 4
; CHECK-NEXT: [[CAST:%.*]] = addrspacecast %"struct.ONEAPIKernelContext::LightSample"* [[REMAT]] to %"struct.ONEAPIKernelContext::LightSample" addrspace(4)*
; CHECK-NEXT: call spir_func void @target_func(%"struct.ONEAPIKernelContext::LightSample" addrspace(4)* [[CAST]])
cleanup.cont.i:                                   ; preds = %if.end46
  call spir_func void @target_func(%"struct.ONEAPIKernelContext::LightSample" addrspace(4)* %ls.ascast.i)
  call spir_func void @target_func_bitcast_no_ptr(double %cast_val2)
  call spir_func void @target_func_bitcast_no_ptr(double %cast_val3)
  ret void
}
