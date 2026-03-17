;; kernel void test(global uint* dst)
;; {
;;     intel_work_group_barrier_arrive(CLK_LOCAL_MEM_FENCE);
;;     intel_work_group_barrier_wait(CLK_LOCAL_MEM_FENCE);
;;     intel_work_group_barrier_arrive(CLK_GLOBAL_MEM_FENCE);
;;     intel_work_group_barrier_wait(CLK_GLOBAL_MEM_FENCE);
;;
;;     intel_work_group_barrier_arrive(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
;;     intel_work_group_barrier_wait(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
;;}

; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_split_barrier -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; ModuleID = 'split_barrier.cl'
source_filename = "split_barrier.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64"

; CHECK-LLVM-LABEL: define spir_kernel void @test
; Function Attrs: convergent norecurse nounwind
define dso_local spir_kernel void @test(i32 addrspace(1)* nocapture noundef readnone align 4 %0) local_unnamed_addr #0 !kernel_arg_addr_space !4 !kernel_arg_access_qual !5 !kernel_arg_type !6 !kernel_arg_base_type !6 !kernel_arg_type_qual !7 {
  tail call spir_func void @_Z31intel_work_group_barrier_arrivej(i32 noundef 1) #2
    ; CHECK-LLVM: call spir_func void @_Z33__spirv_ControlBarrierArriveINTELiii(i32 2, i32 2, i32 260)
  tail call spir_func void @_Z29intel_work_group_barrier_waitj(i32 noundef 1) #2
    ; CHECK-LLVM: call spir_func void @_Z31__spirv_ControlBarrierWaitINTELiii(i32 2, i32 2, i32 258)
  tail call spir_func void @_Z31intel_work_group_barrier_arrivej(i32 noundef 2) #2
    ; CHECK-LLVM: call spir_func void @_Z33__spirv_ControlBarrierArriveINTELiii(i32 2, i32 2, i32 516)
  tail call spir_func void @_Z29intel_work_group_barrier_waitj(i32 noundef 2) #2
    ; CHECK-LLVM: call spir_func void @_Z31__spirv_ControlBarrierWaitINTELiii(i32 2, i32 2, i32 514)
  tail call spir_func void @_Z31intel_work_group_barrier_arrivej(i32 noundef 3) #2
    ; CHECK-LLVM: call spir_func void @_Z33__spirv_ControlBarrierArriveINTELiii(i32 2, i32 2, i32 772)
  tail call spir_func void @_Z29intel_work_group_barrier_waitj(i32 noundef 3) #2
    ; CHECK-LLVM: call spir_func void @_Z31__spirv_ControlBarrierWaitINTELiii(i32 2, i32 2, i32 770)
  ret void
}

; Function Attrs: convergent
declare dso_local spir_func void @_Z31intel_work_group_barrier_arrivej(i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare dso_local spir_func void @_Z29intel_work_group_barrier_waitj(i32 noundef) local_unnamed_addr #1

attributes #0 = { convergent norecurse nounwind "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "uniform-work-group-size"="true" }
attributes #1 = { convergent "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind }

!llvm.module.flags = !{!0, !1}
!opencl.ocl.version = !{!2}
!opencl.spir.version = !{!2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"frame-pointer", i32 2}
!2 = !{i32 1, i32 2}
!3 = !{!"clang version 15.0.0 (https://github.com/llvm/llvm-project 861386dbd6ff0d91636b7c674c2abb2eccd9d3f2)"}
!4 = !{i32 1}
!5 = !{!"none"}
!6 = !{!"uint*"}
!7 = !{!""}
