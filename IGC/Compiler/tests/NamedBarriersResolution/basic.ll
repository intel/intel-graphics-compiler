;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; run: igc_opt -debugify --igc-named-barriers-resolution -check-debugify -S < %s 2>&1 | FileCheck %s
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-named-barriers-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; NamedBarriersResolution
; ------------------------------------------------
;
; Was reduced from ocl test kernel:
; __kernel void foo(__global unsigned char *in)
; {
;   __local NamedBarrier_t* a;
;   a = named_barrier_init(1);
;   work_group_named_barrier(a, CLK_LOCAL_MEM_FENCE );
;   work_group_named_barrier(a, memory_scope_work_group, CLK_LOCAL_MEM_FENCE );
; }
;
; ------------------------------------------------

; Fails on debug check
; COM: check-not WARNING
; COM: check CheckModuleDebugify: PASS


%struct.__namedBarrier = type { i32, i32, i32 }

; Function Attrs: noinline nounwind
define spir_kernel void @foo(i8 addrspace(1)* %in) #1 {
; CHECK-LABEL: @foo(
; CHECK:  entry:
; CHECK:    [[IN_ADDR:%.*]] = alloca i8 addrspace(1)*, align 8
; CHECK:    [[A:%.*]] = alloca [[STRUCT___NAMEDBARRIER:%.*]] addrspace(3)*, align 8
; CHECK:    store i8 addrspace(1)* [[IN:%.*]], i8 addrspace(1)** [[IN_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = getelementptr [8 x %struct.__namedBarrier], [8 x %struct.__namedBarrier] addrspace(3)* @NamedBarrierArray, i64 0, i32 0
; CHECK:    [[TMP1:%.*]] = bitcast [[STRUCT___NAMEDBARRIER]] addrspace(3)* [[TMP0]] to [[STRUCT___NAMEDBARRIER]] addrspace(3)*
; CHECK:    [[TMP2:%.*]] = call [[STRUCT___NAMEDBARRIER]] addrspace(3)* @__builtin_spirv_OpNamedBarrierInitialize_i32_p3__namedBarrier_p3i32(i32 1, [[STRUCT___NAMEDBARRIER]] addrspace(3)* [[TMP1]], i32 addrspace(3)* @NamedBarrierID)
; CHECK:    store [[STRUCT___NAMEDBARRIER]] addrspace(3)* [[TMP2]], [[STRUCT___NAMEDBARRIER]] addrspace(3)** [[A]], align 8
; CHECK:    [[TMP3:%.*]] = load [[STRUCT___NAMEDBARRIER]] addrspace(3)*, [[STRUCT___NAMEDBARRIER]] addrspace(3)** [[A]], align 8
; CHECK:    call void @__builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32(%struct.__namedBarrier addrspace(3)* [[TMP3]], i32 1)
; CHECK:    [[TMP4:%.*]] = load [[STRUCT___NAMEDBARRIER]] addrspace(3)*, [[STRUCT___NAMEDBARRIER]] addrspace(3)** [[A]], align 8
; CHECK:    call void @__builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32_i32(%struct.__namedBarrier addrspace(3)* [[TMP4]], i32 1, i32 1)
; CHECK:    ret void
;
entry:
  %in.addr = alloca i8 addrspace(1)*, align 8
  %a = alloca %struct.__namedBarrier addrspace(3)*, align 8
  store i8 addrspace(1)* %in, i8 addrspace(1)** %in.addr, align 8
  %call = call spir_func %struct.__namedBarrier addrspace(3)* @_Z18named_barrier_initi(i32 1) #0
  store %struct.__namedBarrier addrspace(3)* %call, %struct.__namedBarrier addrspace(3)** %a, align 8
  %0 = load %struct.__namedBarrier addrspace(3)*, %struct.__namedBarrier addrspace(3)** %a, align 8
  call spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj(%struct.__namedBarrier addrspace(3)* %0, i32 1) #0
  %1 = load %struct.__namedBarrier addrspace(3)*, %struct.__namedBarrier addrspace(3)** %a, align 8
  call spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj12memory_scope(%struct.__namedBarrier addrspace(3)* %1, i32 1, i32 1) #0
  ret void
}

; Function Attrs: nounwind
declare spir_func %struct.__namedBarrier addrspace(3)* @_Z18named_barrier_initi(i32) #0

; Function Attrs: nounwind
declare spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj(%struct.__namedBarrier addrspace(3)*, i32) #0

; Function Attrs: nounwind
declare spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj12memory_scope(%struct.__namedBarrier addrspace(3)*, i32, i32) #0

