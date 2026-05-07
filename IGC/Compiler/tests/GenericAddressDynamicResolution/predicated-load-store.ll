;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-generic-address-dynamic-resolution | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

; Test 1: Predicated load with dynamic resolution (private + local + global branches)
define spir_kernel void @predicated_load_dynamic(ptr addrspace(3) %local_buffer, i1 %pred, i16 %passthru) {
  %alloca = alloca i32
  %call = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
  %gid = extractelement <3 x i64> %call, i32 0
  %cond = icmp ne i64 %gid, 0
  br i1 %cond, label %br0, label %br1
br0:
  %private_as_generic = addrspacecast ptr %alloca to ptr addrspace(4)
  br label %return
br1:
  %local_as_generic = addrspacecast ptr addrspace(3) %local_buffer to ptr addrspace(4)
  br label %return
return:
  %generic_ptr = phi ptr addrspace(4) [ %private_as_generic, %br0 ], [ %local_as_generic, %br1 ]

  ; CHECK-LABEL: return:
  ; CHECK: %[[PTI:.*]] = ptrtoint ptr addrspace(4) %generic_ptr to i64
  ; CHECK: %[[TAG:.*]] = lshr i64 %[[PTI]], 61
  ; CHECK: switch i64 %[[TAG]], label %GlobalBlock [
  ; CHECK:   i64 1, label %PrivateBlock
  ; CHECK:   i64 2, label %LocalBlock
  ; CHECK: ]

  ; CHECK: PrivateBlock:
  ; CHECK:   %[[PRIV_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr
  ; CHECK:   %[[PRIV_LOAD:.*]] = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p0.i16(ptr %[[PRIV_PTR]], i64 1, i1 %pred, i16 %passthru)

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(3)
  ; CHECK:   %[[LOCAL_LOAD:.*]] = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p3.i16(ptr addrspace(3) %[[LOCAL_PTR]], i64 1, i1 %pred, i16 %passthru)

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(1)
  ; CHECK:   %[[GLOBAL_LOAD:.*]] = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %[[GLOBAL_PTR]], i64 1, i1 %pred, i16 %passthru)

  %res = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p4.i16(ptr addrspace(4) %generic_ptr, i64 1, i1 %pred, i16 %passthru)
  ret void
}

; Test 2: Predicated store with dynamic resolution
define spir_kernel void @predicated_store_dynamic(ptr addrspace(3) %local_buffer, i32 %val, i1 %pred) {
  %alloca = alloca i32
  %call = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
  %gid = extractelement <3 x i64> %call, i32 0
  %cond = icmp ne i64 %gid, 0
  br i1 %cond, label %br0, label %br1
br0:
  %private_as_generic = addrspacecast ptr %alloca to ptr addrspace(4)
  br label %return
br1:
  %local_as_generic = addrspacecast ptr addrspace(3) %local_buffer to ptr addrspace(4)
  br label %return
return:
  %generic_ptr = phi ptr addrspace(4) [ %private_as_generic, %br0 ], [ %local_as_generic, %br1 ]

  ; CHECK-LABEL: @predicated_store_dynamic
  ; CHECK: %[[PTI:.*]] = ptrtoint ptr addrspace(4) %generic_ptr to i64
  ; CHECK: %[[TAG:.*]] = lshr i64 %[[PTI]], 61
  ; CHECK: switch i64 %[[TAG]], label %GlobalBlock [

  ; CHECK: PrivateBlock:
  ; CHECK:   %[[PRIV_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr
  ; CHECK:   call void @llvm.genx.GenISA.PredicatedStore.p0.i32(ptr %[[PRIV_PTR]], i32 %val, i64 1, i1 %pred)

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(3)
  ; CHECK:   call void @llvm.genx.GenISA.PredicatedStore.p3.i32(ptr addrspace(3) %[[LOCAL_PTR]], i32 %val, i64 1, i1 %pred)

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(1)
  ; CHECK:   call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %[[GLOBAL_PTR]], i32 %val, i64 1, i1 %pred)

  call void @llvm.genx.GenISA.PredicatedStore.p4.i32(ptr addrspace(4) %generic_ptr, i32 %val, i64 1, i1 %pred)
  ret void
}

; Test 3: Predicated load without branches (only global resolution)
define spir_kernel void @predicated_load_global_only(ptr addrspace(1) %global_buffer, i1 %pred, i16 %passthru) {
  %generic_ptr = addrspacecast ptr addrspace(1) %global_buffer to ptr addrspace(4)

  ; CHECK-LABEL: @predicated_load_global_only
  ; CHECK: %[[GLOBAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(1)
  ; CHECK: %[[RES:.*]] = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %[[GLOBAL_PTR]], i64 1, i1 %pred, i16 %passthru)

  %res = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p4.i16(ptr addrspace(4) %generic_ptr, i64 1, i1 %pred, i16 %passthru)
  ret void
}

declare spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
declare i16 @llvm.genx.GenISA.PredicatedLoad.i16.p4.i16(ptr addrspace(4), i64, i1, i16)
declare void @llvm.genx.GenISA.PredicatedStore.p4.i32(ptr addrspace(4), i32, i64, i1)

!igc.functions = !{!0, !3, !4}
!0 = !{ptr @predicated_load_dynamic, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @predicated_store_dynamic, !1}
!4 = !{ptr @predicated_load_global_only, !1}
