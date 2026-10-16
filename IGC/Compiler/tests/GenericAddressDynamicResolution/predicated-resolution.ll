;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers %s -S -o - -platformdg2 -igc-generic-address-dynamic-resolution | FileCheck %s
; RUN: igc_opt --opaque-pointers %s -S -o - -platformtgllp -regkey EnablePredicatedGASDynamicResolution=1 -igc-generic-address-dynamic-resolution | FileCheck %s --check-prefix=NOLSC
; RUN: igc_opt --opaque-pointers %s -S -o - -platformdg2 -regkey EnablePredicatedGASDynamicResolution=0 -igc-generic-address-dynamic-resolution | FileCheck %s --check-prefix=NOLSC

; Without LSC or with the regkey disabled, generic accesses are resolved with branches.
; NOLSC-LABEL: @load_dynamic
; NOLSC: switch i64
; NOLSC-NOT: GenISA.PredicatedLoad
; NOLSC-LABEL: @store_dynamic

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

define spir_kernel void @load_dynamic(ptr addrspace(3) %local_buffer, ptr addrspace(1) %out, i1 %cond) {
  %alloca = alloca i32
  %private_as_generic = addrspacecast ptr %alloca to ptr addrspace(4)
  %local_as_generic = addrspacecast ptr addrspace(3) %local_buffer to ptr addrspace(4)
  %generic_ptr = select i1 %cond, ptr addrspace(4) %private_as_generic, ptr addrspace(4) %local_as_generic

  ; CHECK-LABEL: @load_dynamic
  ; CHECK-NOT: switch
  ; CHECK: %[[PTI:.*]] = ptrtoint ptr addrspace(4) %generic_ptr to i64
  ; CHECK: %[[TAG:.*]] = lshr i64 %[[PTI]], 61
  ; CHECK: %isPrivate = icmp eq i64 %[[TAG]], 1
  ; CHECK: %isLocal = icmp eq i64 %[[TAG]], 2
  ; CHECK: %[[NOT_GLOBAL:.*]] = or i1 %isPrivate, %isLocal
  ; CHECK: %isGlobal = xor i1 %[[NOT_GLOBAL]], true
  ; CHECK: %[[GLOBAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(1)
  ; CHECK: %globalLoad = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %[[GLOBAL_PTR]], i64 4, i1 %isGlobal, i32 poison)
  ; CHECK: %[[LOCAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(3)
  ; CHECK: %localLoad = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p3.i32(ptr addrspace(3) %[[LOCAL_PTR]], i64 4, i1 %isLocal, i32 %globalLoad)
  ; CHECK: %[[PRIV_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr
  ; CHECK: %privateLoad = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0.i32(ptr %[[PRIV_PTR]], i64 4, i1 %isPrivate, i32 %localLoad)
  ; CHECK: store i32 %privateLoad, ptr addrspace(1) %out
  ; CHECK-NOT: load i32
  ; CHECK: ret void
  %val = load i32, ptr addrspace(4) %generic_ptr, align 4
  store i32 %val, ptr addrspace(1) %out, align 4
  ret void
}

define spir_kernel void @store_dynamic(ptr addrspace(3) %local_buffer, i1 %cond, i32 %val) {
  %alloca = alloca i32
  %private_as_generic = addrspacecast ptr %alloca to ptr addrspace(4)
  %local_as_generic = addrspacecast ptr addrspace(3) %local_buffer to ptr addrspace(4)
  %generic_ptr = select i1 %cond, ptr addrspace(4) %private_as_generic, ptr addrspace(4) %local_as_generic

  ; CHECK-LABEL: @store_dynamic
  ; CHECK-NOT: switch
  ; CHECK: %isGlobal = xor i1
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %{{.*}}, i32 %val, i64 4, i1 %isGlobal)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p3.i32(ptr addrspace(3) %{{.*}}, i32 %val, i64 4, i1 %isLocal)
  ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0.i32(ptr %{{.*}}, i32 %val, i64 4, i1 %isPrivate)
  ; CHECK-NOT: store i32
  ; CHECK: ret void
  store i32 %val, ptr addrspace(4) %generic_ptr, align 4
  ret void
}

; The original predicate is combined with each address space predicate.
define spir_kernel void @predicated_load_dynamic(ptr addrspace(3) %local_buffer, ptr addrspace(1) %out, i1 %cond, i1 %pred, i16 %passthru) {
  %alloca = alloca i16
  %private_as_generic = addrspacecast ptr %alloca to ptr addrspace(4)
  %local_as_generic = addrspacecast ptr addrspace(3) %local_buffer to ptr addrspace(4)
  %generic_ptr = select i1 %cond, ptr addrspace(4) %private_as_generic, ptr addrspace(4) %local_as_generic

  ; CHECK-LABEL: @predicated_load_dynamic
  ; CHECK-NOT: switch
  ; CHECK: %[[GLOBAL_PRED:.*]] = and i1 %pred, %isGlobal
  ; CHECK: %globalLoad = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %{{.*}}, i64 2, i1 %[[GLOBAL_PRED]], i16 %passthru)
  ; CHECK: %[[LOCAL_PRED:.*]] = and i1 %pred, %isLocal
  ; CHECK: %localLoad = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p3.i16(ptr addrspace(3) %{{.*}}, i64 2, i1 %[[LOCAL_PRED]], i16 %globalLoad)
  ; CHECK: %[[PRIV_PRED:.*]] = and i1 %pred, %isPrivate
  ; CHECK: %privateLoad = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p0.i16(ptr %{{.*}}, i64 2, i1 %[[PRIV_PRED]], i16 %localLoad)
  ; CHECK: store i16 %privateLoad, ptr addrspace(1) %out
  %res = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p4.i16(ptr addrspace(4) %generic_ptr, i64 2, i1 %pred, i16 %passthru)
  store i16 %res, ptr addrspace(1) %out, align 2
  ret void
}

; Volatile accesses keep the branches.
define spir_kernel void @volatile_load_dynamic(ptr addrspace(3) %local_buffer, ptr addrspace(1) %out, i1 %cond) {
  %alloca = alloca i32
  %private_as_generic = addrspacecast ptr %alloca to ptr addrspace(4)
  %local_as_generic = addrspacecast ptr addrspace(3) %local_buffer to ptr addrspace(4)
  %generic_ptr = select i1 %cond, ptr addrspace(4) %private_as_generic, ptr addrspace(4) %local_as_generic

  ; CHECK-LABEL: @volatile_load_dynamic
  ; CHECK: switch i64
  ; CHECK-NOT: GenISA.PredicatedLoad
  ; CHECK: load volatile i32, ptr addrspace(1)
  %val = load volatile i32, ptr addrspace(4) %generic_ptr, align 4
  store i32 %val, ptr addrspace(1) %out, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.PredicatedLoad.i16.p4.i16(ptr addrspace(4), i64, i1, i16)

!igc.functions = !{!0, !3, !4, !5}
!0 = !{ptr @load_dynamic, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @store_dynamic, !1}
!4 = !{ptr @predicated_load_dynamic, !1}
!5 = !{ptr @volatile_load_dynamic, !1}
