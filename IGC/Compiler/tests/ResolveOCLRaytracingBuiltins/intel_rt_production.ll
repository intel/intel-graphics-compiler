;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-resolve-ocl-raytracing-builtins -platformdg2 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-resolve-ocl-raytracing-builtins -platformdg2 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

; CHECK-TYPED: %struct.intel_ray_query_opaque_t = type { %struct.rtfence_t*, %struct.rtglobals_t addrspace(1)*, i8 addrspace(1)*, i32, i32 }
; CHECK-OPAQUE: %struct.intel_ray_query_opaque_t = type { ptr, ptr addrspace(1), ptr addrspace(1), i32, i32 }
%struct.intel_ray_query_opaque_t = type opaque

%struct.rtglobals_t = type opaque
%struct.rtfence_t = type opaque

define spir_kernel void @kernel() {
entry:
  ; CHECK: %[[RAY_QUERY:.*]] = alloca %struct.intel_ray_query_opaque_t
  ; CHECK-TYPED: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK-OPAQUE: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK-TYPED: store %struct.rtfence_t* null, %struct.rtfence_t** %[[RT_FENCE_ADDR]]
  ; CHECK-OPAQUE: store ptr null, ptr %[[RT_FENCE_ADDR]]
  ; CHECK-TYPED: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK-OPAQUE: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK-TYPED: store %struct.rtglobals_t addrspace(1)* null, %struct.rtglobals_t addrspace(1)** %[[RT_GLOBALS_ADDR]]
  ; CHECK-OPAQUE: store ptr addrspace(1) null, ptr %[[RT_GLOBALS_ADDR]]
  ; CHECK-TYPED: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK-OPAQUE: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK-TYPED: store i8 addrspace(1)* null, i8 addrspace(1)** %[[RT_STACK_ADDR]]
  ; CHECK-OPAQUE: store ptr addrspace(1) null, ptr %[[RT_STACK_ADDR]]
  ; CHECK-TYPED: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK-OPAQUE: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK-TYPED: store i32 0, i32* %[[CTRL_ADDR]]
  ; CHECK-OPAQUE: store i32 0, ptr %[[CTRL_ADDR]]
  ; CHECK-TYPED: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK-OPAQUE: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK-TYPED: store i32 0, i32* %[[BVH_LEVEL_ADDR]]
  ; CHECK-OPAQUE: store i32 0, ptr %[[BVH_LEVEL_ADDR]]
  %ray_query = call spir_func %struct.intel_ray_query_opaque_t* @__builtin_IB_intel_init_ray_query(%struct.rtfence_t* null, %struct.rtglobals_t addrspace(1)* null, i8 addrspace(1)* null, i32 0, i32 0)

  ; CHECK-TYPED: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK-OPAQUE: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK-TYPED: load %struct.rtfence_t*, %struct.rtfence_t** %[[RT_FENCE_ADDR]]
  ; CHECK-OPAQUE: load ptr, ptr %[[RT_FENCE_ADDR]]
  %rt_fence = call spir_func %struct.rtfence_t* @__builtin_IB_intel_query_rt_fence(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK-TYPED: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK-OPAQUE: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK-TYPED: load %struct.rtglobals_t addrspace(1)*, %struct.rtglobals_t addrspace(1)** %[[RT_GLOBALS_ADDR]]
  ; CHECK-OPAQUE: load ptr addrspace(1), ptr %[[RT_GLOBALS_ADDR]]
  %rt_globals = call spir_func %struct.rtglobals_t addrspace(1)* @__builtin_IB_intel_query_rt_globals(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK-TYPED: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK-OPAQUE: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK-TYPED: load i8 addrspace(1)*, i8 addrspace(1)** %[[RT_STACK_ADDR]]
  ; CHECK-OPAQUE: load ptr addrspace(1), ptr %[[RT_STACK_ADDR]]
  %rt_stack = call spir_func i8 addrspace(1)* @__builtin_IB_intel_query_rt_stack(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK-TYPED: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK-OPAQUE: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK-TYPED: load i32, i32* %[[CTRL_ADDR]]
  ; CHECK-OPAQUE: load i32, ptr %[[CTRL_ADDR]]
  %ctrl = call spir_func i32 @__builtin_IB_intel_query_ctrl(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK-TYPED: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK-OPAQUE: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK-TYPED: load i32, i32* %[[BVH_LEVEL_ADDR]]
  ; CHECK-OPAQUE: load i32, ptr %[[BVH_LEVEL_ADDR]]
  %bvh_level = call spir_func i32 @__builtin_IB_intel_query_bvh_level(%struct.intel_ray_query_opaque_t* %ray_query)

  ; CHECK-TYPED: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK-OPAQUE: %[[RT_FENCE_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 0
  ; CHECK-TYPED: store %struct.rtfence_t* null, %struct.rtfence_t** %[[RT_FENCE_ADDR]]
  ; CHECK-OPAQUE: store ptr null, ptr %[[RT_FENCE_ADDR]]
  ; CHECK-TYPED: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK-OPAQUE: %[[RT_GLOBALS_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 1
  ; CHECK-TYPED: store %struct.rtglobals_t addrspace(1)* null, %struct.rtglobals_t addrspace(1)** %[[RT_GLOBALS_ADDR]]
  ; CHECK-OPAQUE: store ptr addrspace(1) null, ptr %[[RT_GLOBALS_ADDR]]
  ; CHECK-TYPED: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK-OPAQUE: %[[RT_STACK_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 2
  ; CHECK-TYPED: store i8 addrspace(1)* null, i8 addrspace(1)** %[[RT_STACK_ADDR]]
  ; CHECK-OPAQUE: store ptr addrspace(1) null, ptr %[[RT_STACK_ADDR]]
  ; CHECK-TYPED: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK-OPAQUE: %[[CTRL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 3
  ; CHECK-TYPED: store i32 0, i32* %[[CTRL_ADDR]]
  ; CHECK-OPAQUE: store i32 0, ptr %[[CTRL_ADDR]]
  ; CHECK-TYPED: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, %struct.intel_ray_query_opaque_t* %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK-OPAQUE: %[[BVH_LEVEL_ADDR:.*]] = getelementptr %struct.intel_ray_query_opaque_t, ptr %[[RAY_QUERY]], i32 0, i32 4
  ; CHECK-TYPED: store i32 0, i32* %[[BVH_LEVEL_ADDR]]
  ; CHECK-OPAQUE: store i32 0, ptr %[[BVH_LEVEL_ADDR]]
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
