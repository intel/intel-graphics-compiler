;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -enable-debugify --igc-resolve-ocl-raytracing-builtins -platformdg2 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ResolveOCLRaytracingBuiltins
; ------------------------------------------------

; Test checks builtins lowering

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

%struct.rtglobals_t = type opaque
%struct.rtfence_t = type opaque

declare spir_func ptr addrspace(4) @__builtin_IB_intel_get_rt_stack(ptr addrspace(1))

define spir_func ptr addrspace(4) @test_intel_get_rt_stack(ptr addrspace(1) %rtglobals) {
; CHECK-LABEL: @test_intel_get_rt_stack
; CHECK: {{%.*}} = load i64, ptr addrspace(1) {{%.*}}
; CHECK: {{%.*}} = call i16 @llvm.genx.GenISA.simdLaneIdReplicate()
; CHECK: {{%.*}} = load i32, ptr addrspace(1) {{%.*}}
  %ptr = call spir_func ptr addrspace(4) @__builtin_IB_intel_get_rt_stack(ptr addrspace(1) %rtglobals)
  ret ptr addrspace(4) %ptr
}

declare spir_func ptr addrspace(4) @__builtin_IB_intel_get_thread_btd_stack(ptr addrspace(1)) local_unnamed_addr

define spir_func ptr addrspace(4) @test_intel_get_thread_btd_stack(ptr addrspace(1) %rtglobals) {
; CHECK-LABEL: @test_intel_get_thread_btd_stack
; CHECK: {{%.*}} = load i64, ptr addrspace(1) {{%.*}}
; CHECK: {{%.*}} = load i32, ptr addrspace(1) {{%.*}}
; CHECK: {{%.*}} = load i32, ptr addrspace(1) {{%.*}}
; CHECK: {{%.*}} = call i32 @llvm.genx.GenISA.dual.subslice.id()
; CHECK: {{%.*}} = call i16 @llvm.genx.GenISA.AsyncStackID()
  %ptr = call spir_func ptr addrspace(4) @__builtin_IB_intel_get_thread_btd_stack(ptr addrspace(1) %rtglobals)
  ret ptr addrspace(4) %ptr
}


declare spir_func ptr addrspace(4) @__builtin_IB_intel_get_global_btd_stack(ptr addrspace(1)) local_unnamed_addr

define spir_func ptr addrspace(4) @test_intel_get_global_btd_stack(ptr addrspace(1) %rtglobals) {
; CHECK-LABEL: @test_intel_get_global_btd_stack
; CHECK: {{%.*}} = load i64, ptr addrspace(1) {{%.*}}
; CHECK: {{%.*}} = load i32, ptr addrspace(1) {{%.*}}
; CHECK: {{%.*}} = load i32, ptr addrspace(1) {{%.*}}
; CHECK: {{%.*}} = call i32 @llvm.genx.GenISA.dual.subslice.id()
; CHECK-NOT: @llvm.genx.GenISA.AsyncStackID()
  %ptr = call spir_func ptr addrspace(4) @__builtin_IB_intel_get_global_btd_stack(ptr addrspace(1) %rtglobals)
  ret ptr addrspace(4) %ptr
}

declare spir_func ptr @__builtin_IB_intel_dispatch_trace_ray_query(ptr addrspace(1), i32, i32)

define spir_func ptr @test_intel_dispatch_trace_ray_query(ptr addrspace(1) %rtglobals, i32 %bvhlevel, i32 %ctrl) {
; CHECK-LABEL: @test_intel_dispatch_trace_ray_query
; CHECK: call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
; CHECK: {{%.*}} = and i32 {{%.*}}, 7
; CHECK: {{%.*}} = and i32 {{%.*}}, 3
; CHECK: {{%.*}} = shl i32 {{%.*}}, 8
; CHECK: {{%.*}} = call i32 @llvm.genx.GenISA.TraceRaySync
  %ptr = call spir_func ptr @__builtin_IB_intel_dispatch_trace_ray_query(ptr addrspace(1) %rtglobals, i32 %bvhlevel, i32 %ctrl)
  ret ptr %ptr
}

declare spir_func void @__builtin_IB_intel_rt_sync(ptr)

define spir_func void @test_intel_rt_sync(ptr %fence) {
; CHECK-LABEL: @test_intel_rt_sync
; CHECK: call void @llvm.genx.GenISA.ReadTraceRaySync
  call spir_func void @__builtin_IB_intel_rt_sync(ptr %fence)
  ret void
}

declare spir_func ptr addrspace(1) @__builtin_IB_intel_get_rt_global_buffer()

define spir_func ptr addrspace(1) @test_intel_get_rt_global_buffer() {
; CHECK-LABEL: @test_intel_get_rt_global_buffer
; CHECK: {{%.*}} = call align 256 ptr addrspace(1) @llvm.genx.GenISA.GlobalBufferPointer.p1()
  %ptr = call spir_func ptr addrspace(1) @__builtin_IB_intel_get_rt_global_buffer()
  ret ptr addrspace(1) %ptr
}
