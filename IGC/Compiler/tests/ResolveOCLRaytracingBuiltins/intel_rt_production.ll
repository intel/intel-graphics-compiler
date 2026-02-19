;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-resolve-ocl-raytracing-builtins -platformdg2 | FileCheck %s

; CHECK: %struct.intel_ray_query_opaque_t = type { %struct.rtfence_t*, %struct.rtglobals_t addrspace(1)*, i8 addrspace(1)*, i32, i32 }
%struct.intel_ray_query_opaque_t = type opaque

%struct.rtglobals_t = type opaque
%struct.rtfence_t = type opaque

define spir_kernel void @kernel() {
entry:
  ; CHECK: %[[RAY_QUERY:.*]] = alloca %struct.intel_ray_query_opaque_t
  ; CHECK: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK: store %struct.rtfence_t* null, %struct.rtfence_t** %[[RT_FENCE_ADDR]]
  ; CHECK: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK: store %struct.rtglobals_t addrspace(1)* null, %struct.rtglobals_t addrspace(1)** %[[RT_GLOBALS_ADDR]]
  ; CHECK: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK: store i8 addrspace(1)* null, i8 addrspace(1)** %[[RT_STACK_ADDR]]
  ; CHECK: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK: store i32 0, i32* %[[CTRL_ADDR]]
  ; CHECK: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK: store i32 0, i32* %[[BVH_LEVEL_ADDR]]
  %ray_query = call spir_func %struct.intel_ray_query_opaque_t* @__builtin_IB_intel_init_ray_query(%struct.rtfence_t* null, %struct.rtglobals_t addrspace(1)* null, i8 addrspace(1)* null, i32 0, i32 0)

  ; CHECK: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK: load %struct.rtfence_t*, %struct.rtfence_t** %[[RT_FENCE_ADDR]]
  %rt_fence = call spir_func %struct.rtfence_t* @__builtin_IB_intel_query_rt_fence(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK: load %struct.rtglobals_t addrspace(1)*, %struct.rtglobals_t addrspace(1)** %[[RT_GLOBALS_ADDR]]
  %rt_globals = call spir_func %struct.rtglobals_t addrspace(1)* @__builtin_IB_intel_query_rt_globals(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK: load i8 addrspace(1)*, i8 addrspace(1)** %[[RT_STACK_ADDR]]
  %rt_stack = call spir_func i8 addrspace(1)* @__builtin_IB_intel_query_rt_stack(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK: load i32, i32* %[[CTRL_ADDR]]
  %ctrl = call spir_func i32 @__builtin_IB_intel_query_ctrl(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK: load i32, i32* %[[BVH_LEVEL_ADDR]]
  %bvh_level = call spir_func i32 @__builtin_IB_intel_query_bvh_level(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK: store %struct.rtfence_t* null, %struct.rtfence_t** %[[RT_FENCE_ADDR]]
  ; CHECK: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK: store %struct.rtglobals_t addrspace(1)* null, %struct.rtglobals_t addrspace(1)** %[[RT_GLOBALS_ADDR]]
  ; CHECK: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK: store i8 addrspace(1)* null, i8 addrspace(1)** %[[RT_STACK_ADDR]]
  ; CHECK: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK: store i32 0, i32* %[[CTRL_ADDR]]
  ; CHECK: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK: store i32 0, i32* %[[BVH_LEVEL_ADDR]]
  call spir_func void @__builtin_IB_intel_update_ray_query(%struct.intel_ray_query_opaque_t* %ray_query, %struct.rtfence_t* null, %struct.rtglobals_t addrspace(1)* null, i8 addrspace(1)* null, i32 0, i32 0)
  ret void
}

declare spir_func %struct.intel_ray_query_opaque_t* @__builtin_IB_intel_init_ray_query(%struct.rtfence_t*, %struct.rtglobals_t addrspace(1)*, i8 addrspace(1)*, i32, i32)
declare spir_func %struct.rtfence_t* @__builtin_IB_intel_query_rt_fence(%struct.intel_ray_query_opaque_t*)
declare spir_func %struct.rtglobals_t addrspace(1)* @__builtin_IB_intel_query_rt_globals(%struct.intel_ray_query_opaque_t*)
declare spir_func i8 addrspace(1)* @__builtin_IB_intel_query_rt_stack(%struct.intel_ray_query_opaque_t*)
declare spir_func i32 @__builtin_IB_intel_query_ctrl(%struct.intel_ray_query_opaque_t*)
declare spir_func i32 @__builtin_IB_intel_query_bvh_level(%struct.intel_ray_query_opaque_t*)
declare spir_func void @__builtin_IB_intel_update_ray_query(%struct.intel_ray_query_opaque_t*, %struct.rtfence_t*, %struct.rtglobals_t addrspace(1)*, i8 addrspace(1)*, i32, i32)
